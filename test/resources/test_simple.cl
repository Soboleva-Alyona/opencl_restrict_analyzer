__kernel void vecadd(__global int* __restrict a, __global int* __restrict b, __global int* c) {
    //const int idx = get_global_id(0);
    //const int* restrict p = &idx;
    //const int pidx = *p;
    //c[idx] = (a + 1)[pidx] + b[idx];
    //c[0] = (a + 1)[0] + b[0];
    b = c;
    a[0] = c[0];
    b[0] = a[0];
}
