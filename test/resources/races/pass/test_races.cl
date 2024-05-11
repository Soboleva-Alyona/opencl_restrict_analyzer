__kernel void test_benign_write_zero(__global int* a, __global int* b, __global int* c) {
  a[0] = 0;
}

__kernel void test_benign_write_zero_local(__local int* a, __global int* b, __global int* c) {
  a[0] = 0;
}

__kernel void test_index_local_id(__global int* a, __global int* b, __global int* c) {
  a[get_local_id(0)] = get_local_id(0);
}

__kernel void test_private_var(__global int* a, __global int* b, __global int* c) {
  int d = 0;
  d = get_local_id(0) % 2;
}

__kernel void test_private_arr(__global int* a, __global int* b, __global int* c) {
  int* d[1024];
  d[0] = 1;
}

__kernel void test_index_local_id_combination_pass(__global int* a, __global int* b, __global int* c) {
  a[get_local_id(0) % get_local_size(0)] = get_local_id(0);
}

__kernel void test_assign_right_part_read_racy(__global int* a, __global int* b, __global int* c) {
  a[get_local_id(0)] = b[get_local_id(0) % 2];
}

__kernel void test_assign_same_addr_same_value(__global int* a, __global int* b,  __global int* c) {
  __local int* d[1024];

  d[0] = b[0];
}

__kernel void test_assign_same_value_with_local_id(__global int* a, __global int* b, __global int* c) {
  __local int* d[1024];

  d[0] = get_local_id(0) % 1;
}

__kernel void test_barrier_between_writes(__global int* a, __global int* b, __global int* c) {
    a[get_local_id(0)] = 4;
    barrier(CLK_GLOBAL_MEM_FENCE);
    b[get_local_id(0)] = a[(get_local_id(0) + 1) % get_local_size(0)];
}

__kernel void test_barrier_for_local(__global int* a, __global int* b,  __global int* c) {
    __local int* d[1024];
    __local int* f[1024];

    d[get_local_id(0)] = 4;
    barrier(CLK_LOCAL_MEM_FENCE);
    f[get_local_id(0)] = d[(get_local_id(0) + 1) % get_local_size(0)];
}

