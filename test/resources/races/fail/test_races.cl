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

__kernel void test_index_local_id_mod_local_size(__global int* a, __global int* b, __global int* c) {
  a[get_local_id(0) % (get_local_size(0) - 1)] = get_local_id(0);
}

__kernel void test_assign_same_addr_diff_value(__global int* a, __global int* b,  __global int* c) {
  __local int* d[1024];

  d[0] = get_local_id(0);
}

__kernel void test_no_barrier_between_writes(__global int* a, __global int* b, __global int* c) {
    a[get_local_id(0)] = 4;
    b[get_local_id(0)] = a[(get_local_id(0) + 1) % get_local_size(0)];
}

__kernel void test_wrong_barrier_between_writes(__global int* a, __global int* b, __global int* c) {
    a[get_local_id(0)] = 4;
    barrier(CLK_LOCAL_MEM_FENCE);
    b[get_local_id(0)] = a[(get_local_id(0) + 1) % get_local_size(0)];
}

__kernel void test_no_barrier_for_local(__global int* a, __global int* b,  __global int* c) {
    __local int* d[1024];
    __local int* f[1024];

    d[get_local_id(0)] = 4;
    f[get_local_id(0)] = d[(get_local_id(0) + 1) % get_local_size(0)];
}

__kernel void test_race_one_instruction(__global int* a, __global int* b, __global int* c) {
    a[get_local_id(0)] = a[(get_local_id(0) + 1) % get_local_size(0)];
}
