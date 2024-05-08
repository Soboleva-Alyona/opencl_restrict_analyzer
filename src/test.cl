__kernel void test_violation_trivial(__global int* a, int t) {
    a[get_global_id(0) % 2] = 1;
    barrier(CLK_LOCAL_MEM_FENCE);
}

__kernel void test_assign_right_part_read_racy(__global int* a, __global int* b) {
  // a[get_local_id(0)] = b[get_local_id(0) % 2];
  __local int* c[1024];

  c[0] = get_local_id(0) % 1;
}