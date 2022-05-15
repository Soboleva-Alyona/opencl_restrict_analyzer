__kernel void test_no_violation_trivial(__global int* a, __global int* b, __global int* c) {
    a[0] = b[0];
}

__kernel void test_violation_trivial(__global int* a, __global int* b, __global int* c) {
    a[-1] = b[-1];
}

__kernel void test_no_violation_relative(__global int* a, __global int* b, __global int* c) {
    a[get_global_id(0)] = b[get_global_id(0)];
}

__kernel void test_violation_relative(__global int* a, __global int* b, __global int* c) {
    a[get_global_size(0)] = b[get_global_size(0)];
}

__kernel void test_violation_inside_loop(__global int* a, __global int* b, __global int* c) {
    int n = get_global_size(0);
    for (int i = 0; i <= n; ++i) {
        a[i];
    }
}

__kernel void test_no_violation_inside_loop(__global int* a, __global int* b, __global int* c) {
    int n = get_global_size(0);
    for (int i = 0; i < n; ++i) {
        a[i];
    }
}

__kernel void test_no_violation_unreachable_statement(__global int* a, __global int* b, __global int* c) {
    if (true) {
        for (int i = 0; i < 3; ++i) {
            return;
        }
    }
    a[-1] = b[-1];
}

__kernel void test_violation_inside_loop_inside_loop(__global int* a, __global int* b, __global int* c) {
    int n = get_global_size(0);
    for (int i = 0; i < n; ++i) {
        for (int j = i; j < i + 2; ++j) {
            a[j];
        }
    }
}

__kernel void test_violation_conditional(__global int* a, __global int* b, __global int* c) {
    int i = 0;
    if (a[0] != 0) {
        --i;
    }
    a[i];
}

__kernel void test_violation_pointer_arithmetic(__global int* a, __global int* b, __global int* c) {
    __global int* d = a + get_global_size(0);
    *d;
}

__kernel void test_no_violation_pointer_arithmetic(__global int* a, __global int* b, __global int* c) {
    __global int* d = a + get_global_size(0);
    *(--d);
}

__kernel void test_no_violation_complex_conditional_return(__global int* a, __global int* b, __global int* c) {
    int i = get_global_id(0);
    while (true) {
        if (i > (get_global_size(0) - 1) / 2) {
            return;
        }
        break;
    }
    a[i + i] = b[i + i];
}

__kernel void test_violation_complex_conditional_return(__global int* a, __global int* b, __global int* c) {
    int i = get_global_id(0);
    while (true) {
        if (i > get_global_size(0) / 2) {
            return;
        }
        break;
    }
    a[i + i] = b[i + i];
}
