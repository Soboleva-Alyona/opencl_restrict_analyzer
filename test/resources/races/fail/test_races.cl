__kernel void test_write_zero(__global int* A) {
  A[0] = 0;
}

__kernel void test_write_zero_local(__local int* A) {
  A[0] = 0;
}

__kernel void test_index_local_id_combination(__global int* A) {
  A[get_local_id(0) % 2] = get_local_id(0);
}

__kernel void test_index_parameter(__global int* A, int i) {
  A[i] = B[j];
}

__kernel void test_local_arr_declaration(__global int* A) {
    __global int* b[1024];
    b[0] = get_local_id(0);
}



