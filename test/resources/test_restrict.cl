void assign_from_b_to_a(int* restrict a, int* restrict b) {
    a[0] = b[0];
}

__kernel void test_no_violation_trivial(__global int* restrict a, __global int* restrict b, __global int* restrict c) {
    a[0] = b[0];
    c[0] = a[0];
    b[0] = c[0];
}

__kernel void test_violation_trivial(__global int* restrict a, __global int* restrict b, __global int* restrict c) {
    int* restrict d = a;
    a[0] = d[0];
}

__kernel void test_violation_conditional(__global int* restrict a, __global int* restrict b, __global int* restrict c) {
    if (a[0] != 0) {
        b = a;
    }
    a[0] = b[0];
}

__kernel void test_no_violation_unreachable_condition(__global int* restrict a, __global int* restrict b, __global int* restrict c) {
    if (false) {
        b = a;
    }
    a[0] = b[0];
}

__kernel void test_no_violation_unreachable_statement(__global int* restrict a, __global int* restrict b, __global int* restrict c) {
    __global int* restrict d = a;
    if (true) {
        for (int i = 0; i < 3; ++i) {
            return;
        }
    }
    a[0] = d[0];
}

__kernel void test_violation_incorrect_function_call(__global int* restrict a, __global int* restrict b, __global int* restrict c) {
    assign_from_b_to_a(a, a);
}

__kernel void test_no_violation_correct_function_call(__global int* restrict a, __global int* restrict b, __global int* restrict c) {
    assign_from_b_to_a(a, b);
}

__kernel void test_violation_inside_loop(__global int* restrict a, __global int* restrict b, __global int* restrict c) {
    int* restrict d = a;
    for (int i = 0; i < 3; ++i) {
        d[i] = a[i];
    }
}

__kernel void test_no_violation_inside_loop_unreachable(__global int* restrict a, __global int* restrict b, __global int* restrict c) {
    int* restrict d = a;
    for (int i = 0; i < i; ++i) {
        d[i] = a[i];
    }
}

__kernel void test_violation_dereference(__global int* restrict a, __global int* restrict b, __global int* restrict c) {
    *a = *(b = a);
}
