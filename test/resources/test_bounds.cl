__kernel void test_no_violation_trivial(int* a, int* b, int* c) {
    a[0] = b[0];
}

__kernel void test_violation_trivial(int* a, int* b, int* c) {
    a[-1] = b[-1];
}

__kernel void test_no_violation_relative(int* a, int* b, int* c) {
    a[get_global_id(0)] = b[get_global_id(0)];
}

__kernel void test_violation_relative(int* a, int* b, int* c) {
    a[get_global_size(0)] = b[get_global_size(0)];
}
