int myfunc(int a) {
    return a;
}

__kernel void vecadd(__global int* __restrict a, __global int* __restrict b, __global int* c) {
    //const int idx = get_global_id(0);
    //const int* restrict p = &idx;
    //const int pidx = *p;
    //c[idx] = (a + 1)[pidx] + b[idx];
    //c[0] = (a + 1)[0] + b[0];
    b = c;
    int i = get_global_id(1);
    while (i < 15) {
        b[i] = c[i + 3];
        i = i + 1;
    }
    a[0] = c[myfunc(0)];
    if (false) {
        if (true) {
            return;
        }
    }
    if (true) {
        b[0] = a[0];
    }
    if (a[0] == b[0]) {
        b = c;
        b[0] = a[0];
        return;
    } else {
        a = c;
        b[0] = a[0];
    }
    a[0] = c[0];
    b[0] = a[0];
    a[0] = c[0];
    b[myfunc(0)] = a[0];
    a[0] = c[0];
    b[0] = a[0];
    a[0] = c[0];
}
