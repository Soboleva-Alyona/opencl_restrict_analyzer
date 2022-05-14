void assign_from_b_to_a(int* __restrict a, int* __restrict b) {
    a[0] = b[0];
}

__kernel void test_no_violation_trivial(int* __restrict a, int* __restrict b, int* __restrict c) {
    a[0] = b[0];
    c[0] = a[0];
    b[0] = c[0];
}

__kernel void test_violation_trivial(int* __restrict a, int* __restrict b, int* __restrict c) {
    int* __restrict d = a;
    a[0] = d[0];
}

__kernel void test_violation_conditional(int* __restrict a, int* __restrict b, int* __restrict c) {
    if (a[0] != 0) {
        b = a;
    }
    a[0] = b[0];
}

__kernel void test_no_violation_unreachable_condition(int* __restrict a, int* __restrict b, int* __restrict c) {
    if (false) {
        b = a;
    }
    a[0] = b[0];
}

__kernel void test_no_violation_unreachable_statement(int* __restrict a, int* __restrict b, int* __restrict c) {
    int* __restrict d = a;
    if (true) {
        for (int i = 0; i < 3; ++i) {
            return;
        }
    }
    a[0] = d[0];
}

__kernel void test_violation_incorrect_function_call(int* __restrict a, int* __restrict b, int* __restrict c) {
    assign_from_b_to_a(a, a);
}

__kernel void test_no_violation_correct_function_call(int* __restrict a, int* __restrict b, int* __restrict c) {
    assign_from_b_to_a(a, b);
}

__kernel void test_violation_inside_loop(int* __restrict a, int* __restrict b, int* __restrict c) {
    int* __restrict d = a;
    for (int i = 0; i < 3; ++i) {
        d[i] = a[i];
    }
}

__kernel void test_no_violation_inside_loop_unreachable(int* __restrict a, int* __restrict b, int* __restrict c) {
    int* __restrict d = a;
    for (int i = 0; i < i; ++i) {
        d[i] = a[i];
    }
}
