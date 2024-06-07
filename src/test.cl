// Here is written from scratch OpenCL kernels for dsu-based trimming for CuckARoo29
// Part of nanominer.

// Python Cog code generation is used
// to re-generate, run
//   python -m cogapp -r dsu.cl
// see https://nedbatchelder.com/code/cog/ for details

//[[[cog
//  import cuckoo
//  import sys
//  sys.path.append('../common_generator')
//  import commongen
//  commongen.debug_print=True
//]]]
//[[[end]]]

#define GRAPH_SIZE      64

//[[[cog
//cuckoo.gen_constants()
//]]]
static __constant uint N = 1u << 29;
static __constant uint NODE_MASK = (1u << 29) - 1;
//static __constant uint NODE_MASK = 63;
static __constant uint EDGE_ALIVE_MASK = 1u << 29;
static __constant uint NODE_STATUS_MASK = 0xC0000000u;
static __constant uint NODE_STATUS_GOOD = 0x80000000u;
static __constant uint EDGE_BLOCK_SIZE = 64;
//[[[end]]]

#define HAS_CYCLES_MASK 0x8000 //for 2 byte rank/size
#define MAX_SET_SIZE  0x7FFF

//[[[cog
//cuckoo.gen_sipround()
//cuckoo.gen_hash24()
//cuckoo.gen_xor_lanes()
//]]]
static ulong4 sipround(ulong4 state)
{
  state.s0 += state.s1;
  state.s2 += state.s3;
  state.s1 = rotate(state.s1, 13ul);
  state.s3 = rotate(state.s3, 16ul);
  state.s1 ^= state.s0;
  state.s3 ^= state.s2;
  state.s0 = rotate(state.s0, 32ul);
  state.s2 += state.s1;
  state.s0 += state.s3;
  state.s1 = rotate(state.s1, 17ul);
  state.s3 = rotate(state.s3, 21ul);
  state.s1 ^= state.s2;
  state.s3 ^= state.s0;
  state.s2 = rotate(state.s2, 32ul);
  return state;
}
static ulong4 hash24(ulong4 state, ulong nonce)
{
  state.s3 ^= nonce;
  state = sipround(state);
  state = sipround(state);
  state.s0 ^= nonce;
  state.s2 ^= 0xff;
  state = sipround(state);
  state = sipround(state);
  state = sipround(state);
  state = sipround(state);
  return state;
}
static ulong xorLanes(ulong4 state)
{
  return (state.s0 ^ state.s1) ^ (state.s2  ^ state.s3);
}
//[[[end]]]

//~~~~~~~~~~~~~single thr DSU~~~~~~~~~~~~~~
static void swap(uint *left, uint *right)
{
  uint tmp = *left;
  *left = *right;
  *right = tmp;
}

uint find_set(uint v, __global uint * parent)
{
  uint r = v;
  while (r != parent[r]) {
    r = parent[r];
  }
  parent[v] = r;
  return r;
}

void ranked_union_sets(uint a, uint b, __global uint * parent, __global ushort * rank) //version with ranks
{
  uint ap = find_set(a, parent);
  uint bp = find_set(b, parent);
  uint gIdx = get_global_id(0) - get_global_offset(0);

  if (gIdx == 0) {
    //printf("a: %u -> %u:%u     b : %u -> %u:%u\n", a, ap, parent[ap], b, bp, parent[bp]);


    if (ap != bp) {
      if (rank[ap] < rank[bp]) {
        if (gIdx == 0) {
          //printf("swap ap, bp\n");
        }
        swap(&ap, &bp);
      }
      parent[bp] = ap;
        if (gIdx == 0) {
          //printf("parent[%u] <- %u (real val: %u)\n", bp, ap, parent[bp]);
        }
      if (rank[ap] == rank[bp]) {
        ++rank[ap];
      }
    }
  }
}

void union_sets_s(uint a, uint b, __global uint * parent, __global ushort * size) //version with sizes without satiation
{
  uint ap = find_set(a, parent);
  uint bp = find_set(b, parent);

  if (ap != bp) {
    if (size[ap] < size[bp]) {
      swap(&ap, &bp);
    }
    parent[bp] = ap;
    //size[ap] += size[bp] & (~HAS_CYCLES_MASK); //TODO: Add with saturation and take into account "has cycles" bit.
		ushort sizeAp = size[ap] & (~HAS_CYCLES_MASK);
		ushort sizeBp = size[bp] & (~HAS_CYCLES_MASK);
		ushort r = min(sizeAp + sizeBp, MAX_SET_SIZE);
		//size[ap] = (size[ap] & HAS_CYCLES_MASK) | r;
  }
  else if ((a != ap) && (b != bp)){
    size[ap] |= HAS_CYCLES_MASK;
  }
}
//~~~~~~~~~~~~~end of DSU~~~~~~~~~~~~~~~~~~

#define GRAPH_ELEMS			3
// Single-threaded DSU for small graph. Debug version.
__kernel __attribute__((reqd_work_group_size(64, 1, 1)))
void dsuPrecalcedSingle(
  __global uint *graph, //edge, left, right
	uint graphSize,
  __global uint * parent,
  __global ushort * rank
  )
{
	uint gIdx = get_global_id(0) - get_global_offset(0);

	if (gIdx <= graphSize) {
		parent[gIdx] = gIdx;
		rank[gIdx] = 1;
	}
	for (uint i = 0; i < GRAPH_ELEMS * graphSize; i+=GRAPH_ELEMS) {
		uint left = graph[i + 1];
		uint right = graph[i + 2];
		if (gIdx == 0) {
      union_sets_s(left, right, parent, rank);
    }
	}
}
