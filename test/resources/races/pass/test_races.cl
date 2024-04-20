__kernel void test_index_local_id(__local int* A) {
  A[get_local_id(0)] = get_local_id(0);
}

__kernel void test_private_var(__local int* A) {
  int a = 0;
}

__kernel void test_private_arr(__local int* A) {
  int* a[1024];
  a[0] = 1;
}


