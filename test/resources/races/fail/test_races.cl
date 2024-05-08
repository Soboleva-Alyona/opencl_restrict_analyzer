__kernel void test_index_local_id_combination(__global int* a, __global int* b, __global int* c) {
  a[get_local_id(0) % 2] = get_local_id(0);
}

__kernel void test_index_global_id_combination(__global int* a, __global int* b, __global int* c) {
  a[get_global_id(0) % 2] = get_local_id(0);
}

__kernel void test_local_arr_declaration_to_int_index(__global int* a, __global int* b, __global int* c) {
    __global int* d[1024];
    d[0] = get_local_id(0);
}

__kernel void test_local_arr_declaration(__global int* a, __global int* b, __global int* c) {
    __global int* d[1024];
    d[get_local_id(0) % 2] = get_local_id(0);
}

__kernel void test_barrier(__global int* a, __global int* b, __global int* c) {
    a[get_global_id(0) % 2] = get_;
    barrier(CLK_LOCAL_MEM_FENCE);
}

__kernel void test_index_local_id_mod_local_size(__global int* a, __global int* b, __global int* c) {
  a[get_local_id(0) % (get_local_size(0) - 1)] = get_local_id(0);
}

__kernel void test_assign_same_addr_diff_value(__global int* a, __global int* b,  __global int* c) {
  // a[get_local_id(0)] = b[get_local_id(0) % 2];
  __local int* d[1024];

  d[0] = get_local_id(0);
}

