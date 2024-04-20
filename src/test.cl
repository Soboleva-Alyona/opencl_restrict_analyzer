__kernel void test_violation_trivial(__global int* a, int t) {
    a[get_local_id(0)] = 1;
}

