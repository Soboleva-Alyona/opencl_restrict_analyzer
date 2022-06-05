// https://github.com/oneapi-src/oneDNN

typedef unsigned int uint;

#undef cl_future_bf16_cvt

#define DT_UNDEF 1
#ifndef GPU_OCL_OCL_TYPES_H
#define GPU_OCL_OCL_TYPES_H

#ifndef GPU_OCL_OCL_MATH_UTILS_H
#define GPU_OCL_OCL_MATH_UTILS_H

#pragma OPENCL EXTENSION cl_khr_fp16 : enable

#if DT_BF16 || SRC_DT_BF16 || WEI_DT_BF16 || DST_DT_BF16 || BIA_DT_BF16 \
        || A_DT_BF16 || B_DT_BF16 || C_DT_BF16 || SUM_DT_BF16 \
        || POST_OP_USING_BF16
#define MATH_UTILS_DECLARE_BF16 1
#endif

ulong8 __builtin_IB_simd_block_read_8_global_l(const __global ulong *);
ushort16 __builtin_IB_simd_block_read_16_global_h(const __global ushort *);

void __builtin_IB_simd_block_write_8_global_l(__global ulong *, ulong8);
void __builtin_IB_simd_block_write_16_global_h(__global ushort *, ushort16);

#if MATH_UTILS_DECLARE_BF16
#ifdef cl_future_bf16_cvt
// f32 -> bf16 conversion builtins (rte rounding mode)
short __builtin_IB_ftobf_1(float a) __attribute__((const));
short2 __builtin_IB_ftobf_2(float2 a) __attribute__((const));
short4 __builtin_IB_ftobf_4(float4 a) __attribute__((const));
short8 __builtin_IB_ftobf_8(float8 a) __attribute__((const));
short16 __builtin_IB_ftobf_16(float16 a) __attribute__((const));

// bf16 -> f32 conversion builtins (precise conversion)
float __builtin_IB_bftof_1(short a) __attribute__((const));
float2 __builtin_IB_bftof_2(short2 a) __attribute__((const));
float4 __builtin_IB_bftof_4(short4 a) __attribute__((const));
float8 __builtin_IB_bftof_8(short8 a) __attribute__((const));
float16 __builtin_IB_bftof_16(short16 a) __attribute__((const));

// clang-format off
ushort   __attribute__((overloadable)) cvt_f32_to_bf16(float   a) { return as_ushort  (__builtin_IB_ftobf_1 (a)); }
ushort2  __attribute__((overloadable)) cvt_f32_to_bf16(float2  a) { return as_ushort2 (__builtin_IB_ftobf_2 (a)); }
ushort4  __attribute__((overloadable)) cvt_f32_to_bf16(float4  a) { return as_ushort4 (__builtin_IB_ftobf_4 (a)); }
ushort8  __attribute__((overloadable)) cvt_f32_to_bf16(float8  a) { return as_ushort8 (__builtin_IB_ftobf_8 (a)); }
ushort16 __attribute__((overloadable)) cvt_f32_to_bf16(float16 a) { return as_ushort16(__builtin_IB_ftobf_16(a)); }

float   __attribute__((overloadable)) cvt_bf16_to_f32(ushort   a) { return __builtin_IB_bftof_1 (as_short  (a)); }
float2  __attribute__((overloadable)) cvt_bf16_to_f32(ushort2  a) { return __builtin_IB_bftof_2 (as_short2 (a)); }
float4  __attribute__((overloadable)) cvt_bf16_to_f32(ushort4  a) { return __builtin_IB_bftof_4 (as_short4 (a)); }
float8  __attribute__((overloadable)) cvt_bf16_to_f32(ushort8  a) { return __builtin_IB_bftof_8 (as_short8 (a)); }
float16 __attribute__((overloadable)) cvt_bf16_to_f32(ushort16 a) { return __builtin_IB_bftof_16(as_short16(a)); }
// clang-format on

#else

// Emulation functions for bf16 <-> f32 conversion.
ushort __attribute__((overloadable)) cvt_f32_to_bf16(float f) {
    uint i = as_uint(f);
    i += 0x00007FFF + ((i & 0x10000) >> 16);
    ushort2 r = as_ushort2(i);
    return r[1];
}

ushort2 __attribute__((overloadable)) cvt_f32_to_bf16(float2 f) {
    ushort2 r;
    for (int i = 0; i < 2; i++) {
        r[i] = cvt_f32_to_bf16(f[i]);
    }
    return r;
}

ushort4 __attribute__((overloadable)) cvt_f32_to_bf16(float4 f) {
    ushort4 r;
    for (int i = 0; i < 4; i++) {
        r[i] = cvt_f32_to_bf16(f[i]);
    }
    return r;
}

ushort8 __attribute__((overloadable)) cvt_f32_to_bf16(float8 f) {
    ushort8 r;
    for (int i = 0; i < 8; i++) {
        r[i] = cvt_f32_to_bf16(f[i]);
    }
    return r;
}

ushort16 __attribute__((overloadable)) cvt_f32_to_bf16(float16 f) {
    ushort16 r;
    for (int i = 0; i < 16; i++) {
        r[i] = cvt_f32_to_bf16(f[i]);
    }
    return r;
}

float __attribute__((overloadable)) cvt_bf16_to_f32(ushort b) {
    ushort2 r = {0, b};
    float f = as_float(r);
    return f;
}

float2 __attribute__((overloadable)) cvt_bf16_to_f32(ushort2 b) {
    float2 f;
    for (int i = 0; i < 2; i++) {
        f[i] = cvt_bf16_to_f32(b[i]);
    }
    return f;
}

float4 __attribute__((overloadable)) cvt_bf16_to_f32(ushort4 b) {
    float4 f;
    for (int i = 0; i < 4; i++) {
        f[i] = cvt_bf16_to_f32(b[i]);
    }
    return f;
}

float8 __attribute__((overloadable)) cvt_bf16_to_f32(ushort8 b) {
    float8 f;
    for (int i = 0; i < 8; i++) {
        f[i] = cvt_bf16_to_f32(b[i]);
    }
    return f;
}

float16 __attribute__((overloadable)) cvt_bf16_to_f32(ushort16 b) {
    float16 f;
    for (int i = 0; i < 16; i++) {
        f[i] = cvt_bf16_to_f32(b[i]);
    }
    return f;
}
#endif
#endif

int __attribute__((overloadable)) idot4(char4 a, char4 b, int c) {
    c += a[0] * b[0];
    c += a[1] * b[1];
    c += a[2] * b[2];
    c += a[3] * b[3];
    return c;
}

int __attribute__((overloadable)) idot4(uchar4 a, uchar4 b, int c) {
    c += a[0] * b[0];
    c += a[1] * b[1];
    c += a[2] * b[2];
    c += a[3] * b[3];
    return c;
}

int __attribute__((overloadable)) idot4(char4 a, uchar4 b, int c) {
    c += a[0] * b[0];
    c += a[1] * b[1];
    c += a[2] * b[2];
    c += a[3] * b[3];
    return c;
}

int __attribute__((overloadable)) idot4(uchar4 a, char4 b, int c) {
    c += a[0] * b[0];
    c += a[1] * b[1];
    c += a[2] * b[2];
    c += a[3] * b[3];
    return c;
}

int __attribute__((overloadable)) idot4(int a, int b, int c) {
    return idot4(as_char4(a), as_char4(b), c);
}

int __attribute__((overloadable)) idot4(uint a, int b, int c) {
    return idot4(as_uchar4(a), as_char4(b), c);
}

float __attribute__((overloadable)) f16_dot2(int a, int b, float c) {
    half2 _a = as_half2(a);
    half2 _b = as_half2(b);
    return c + _a[0] * _b[0] + _a[1] * _b[1];
}

#if MATH_UTILS_DECLARE_BF16
float __attribute__((overloadable)) bf16_dot2(int a, int b, float c) {
    ushort2 _a = as_ushort2(a);
    ushort2 _b = as_ushort2(b);
    c += cvt_bf16_to_f32(_a[0]) * cvt_bf16_to_f32(_b[0]);
    c += cvt_bf16_to_f32(_a[1]) * cvt_bf16_to_f32(_b[1]);
    return c;
}
#endif

#define DECLARE_BLOCK_READ(suffix, func, data_type, addr_space, p_type) \
    data_type __attribute__((overloadable)) \
            block_read##suffix(const addr_space p_type *p) { \
        return func(p); \
    }

#define DECLARE_BLOCK_READ_EMU(suffix, data_type, addr_space, p_type) \
    data_type __attribute__((overloadable)) \
            block_read##suffix##_emu(const addr_space p_type *p) { \
        data_type ret; \
        uint idx = get_sub_group_local_id(); \
        for (int i = 0; i < sizeof(data_type) / sizeof(p_type); i++) { \
            ((p_type *)&ret)[i] = p[idx]; \
            idx += get_max_sub_group_size(); \
        } \
        return ret; \
    }

#define DECLARE_BLOCK_WRITE(suffix, func, data_type, addr_space, p_type) \
    void __attribute__((overloadable)) \
            block_write##suffix(addr_space p_type *p, data_type data) { \
        func(p, data); \
    }

#define DECLARE_BLOCK_WRITE_EMU(suffix, data_type, addr_space, p_type) \
    void __attribute__((overloadable)) \
            block_write##suffix##_emu(addr_space p_type *p, data_type data) { \
        uint idx = get_sub_group_local_id(); \
        for (int i = 0; i < sizeof(data_type) / sizeof(p_type); i++) { \
            p[idx] = ((p_type *)&data)[i]; \
            p += get_max_sub_group_size(); \
        } \
    }

DECLARE_BLOCK_READ(, intel_sub_group_block_read, uint, __global, uint)
DECLARE_BLOCK_READ(2, intel_sub_group_block_read2, uint2, __global, uint)
DECLARE_BLOCK_READ(4, intel_sub_group_block_read4, uint4, __global, uint)
DECLARE_BLOCK_READ(8, intel_sub_group_block_read8, uint8, __global, uint)

DECLARE_BLOCK_WRITE(, intel_sub_group_block_write, uint, __global, uint)
DECLARE_BLOCK_WRITE(2, intel_sub_group_block_write2, uint2, __global, uint)
DECLARE_BLOCK_WRITE(4, intel_sub_group_block_write4, uint4, __global, uint)
DECLARE_BLOCK_WRITE(8, intel_sub_group_block_write8, uint8, __global, uint)

#ifdef cl_intel_subgroups_char
void __attribute__((overloadable))
intel_sub_group_block_write_uc16(__global uchar *p, uchar16 data);

uchar16 __attribute__((overloadable))
intel_sub_group_block_read_uc16(const __global uchar *p);
#endif

// Emulation for cl_intel_subgroup_local_block_io. These functions are not
// defined under ifndef/endif because some kernels rely on the emulation
// functions in case when pointers are not properly aligned for the native
// extensions.
DECLARE_BLOCK_READ_EMU(, uint, __local, uint)
DECLARE_BLOCK_READ_EMU(2, uint2, __local, uint)
DECLARE_BLOCK_READ_EMU(4, uint4, __local, uint)
DECLARE_BLOCK_READ_EMU(8, uint8, __local, uint)

DECLARE_BLOCK_WRITE_EMU(, uint, __local, uint)
DECLARE_BLOCK_WRITE_EMU(2, uint2, __local, uint)
DECLARE_BLOCK_WRITE_EMU(4, uint4, __local, uint)
DECLARE_BLOCK_WRITE_EMU(8, uint8, __local, uint)

DECLARE_BLOCK_WRITE_EMU(_us, ushort, __local, ushort)
DECLARE_BLOCK_WRITE_EMU(_us2, ushort2, __local, ushort)
DECLARE_BLOCK_WRITE_EMU(_us4, ushort4, __local, ushort)
DECLARE_BLOCK_WRITE_EMU(_us8, ushort8, __local, ushort)
#ifdef cl_intel_subgroup_local_block_io

DECLARE_BLOCK_READ(, intel_sub_group_block_read, uint, __local, uint)
DECLARE_BLOCK_READ(2, intel_sub_group_block_read2, uint2, __local, uint)
DECLARE_BLOCK_READ(4, intel_sub_group_block_read4, uint4, __local, uint)
DECLARE_BLOCK_READ(8, intel_sub_group_block_read8, uint8, __local, uint)

DECLARE_BLOCK_WRITE(, intel_sub_group_block_write, uint, __local, uint)
DECLARE_BLOCK_WRITE(2, intel_sub_group_block_write2, uint2, __local, uint)
DECLARE_BLOCK_WRITE(4, intel_sub_group_block_write4, uint4, __local, uint)
DECLARE_BLOCK_WRITE(8, intel_sub_group_block_write8, uint8, __local, uint)

DECLARE_BLOCK_WRITE(
        _us, intel_sub_group_block_write_us, ushort, __local, ushort)

#else

DECLARE_BLOCK_READ(, block_read_emu, uint, __local, uint)
DECLARE_BLOCK_READ(2, block_read2_emu, uint2, __local, uint)
DECLARE_BLOCK_READ(4, block_read4_emu, uint4, __local, uint)
DECLARE_BLOCK_READ(8, block_read8_emu, uint8, __local, uint)

DECLARE_BLOCK_WRITE(, block_write_emu, uint, __local, uint)
DECLARE_BLOCK_WRITE(2, block_write2_emu, uint2, __local, uint)
DECLARE_BLOCK_WRITE(4, block_write4_emu, uint4, __local, uint)
DECLARE_BLOCK_WRITE(8, block_write8_emu, uint8, __local, uint)

DECLARE_BLOCK_WRITE(_us, block_write_us_emu, ushort, __local, ushort)

#endif

// Matrix-matrix multiplication: ACC += A * B
//
// A is (m x (E * K))
// B is ((E * K) x sub_group_size)
// where E is 4 for s8/u8 elements and 2 for f16/bf16 elements.
#define DECLARE_MMAD_EMU(name, dot, K, m, a_type, b_type, acc_type) \
    acc_type __attribute__((overloadable)) \
            name(a_type A_vectors, b_type B_vectors, acc_type acc) { \
        for (uint i = 0; i < (m); ++i) { \
            for (uint j = 0; j < (K); ++j) \
                acc[i] = dot(sub_group_broadcast(A_vectors[i], j), \
                        B_vectors[j], acc[i]); \
        } \
        return acc; \
    }

#if defined(cl_intel_subgroup_matrix_multiply_accumulate) && !DISABLE_DPAS

int8 __attribute__((overloadable)) mmad8x8(uint8 a, int8 b, int8 acc) {
    return intel_sub_group_u8_i8_matrix_mad_k32(a, b, acc);
}

int8 __attribute__((overloadable)) mmad8x8(int8 a, int8 b, int8 acc) {
    return intel_sub_group_i8_i8_matrix_mad_k32(a, b, acc);
}

int4 __attribute__((overloadable)) mmad8x4(uint4 a, int8 b, int4 acc) {
    return intel_sub_group_u8_i8_matrix_mad_k32(a, b, acc);
}

int4 __attribute__((overloadable)) mmad8x4(int4 a, int8 b, int4 acc) {
    return intel_sub_group_i8_i8_matrix_mad_k32(a, b, acc);
}

int4 __attribute__((overloadable)) mmad8x4(ushort4 a, int8 b, int4 acc) {
    return intel_sub_group_u8_i8_matrix_mad_k32(a, b, acc);
}

int4 __attribute__((overloadable)) mmad8x4(short4 a, int8 b, int4 acc) {
    return intel_sub_group_i8_i8_matrix_mad_k32(a, b, acc);
}

int4 __attribute__((overloadable)) mmad8x4(ushort4 a, uint8 b, int4 acc) {
    return intel_sub_group_u8_u8_matrix_mad_k32(a, b, acc);
}

int4 __attribute__((overloadable)) mmad8x4(short4 a, uint8 b, int4 acc) {
    return intel_sub_group_i8_u8_matrix_mad_k32(a, b, acc);
}

int8 __attribute__((overloadable)) mmad8x8(ushort8 a, int8 b, int8 acc) {
    return intel_sub_group_u8_i8_matrix_mad_k32(a, b, acc);
}

int8 __attribute__((overloadable)) mmad8x8(short8 a, int8 b, int8 acc) {
    return intel_sub_group_i8_i8_matrix_mad_k32(a, b, acc);
}

float8 __attribute__((overloadable)) mmad8x8_f16(uint8 a, int8 b, float8 acc) {
    return intel_sub_group_f16_f16_matrix_mad_k16(as_int8(a), b, acc);
}

float4 __attribute__((overloadable)) mmad8x4_f16(uint4 a, int8 b, float4 acc) {
    return intel_sub_group_f16_f16_matrix_mad_k16(as_int4(a), b, acc);
}

float4 __attribute__((overloadable))
mmad8x4_f16(ushort4 a, int8 b, float4 acc) {
    return intel_sub_group_f16_f16_matrix_mad_k16(as_short4(a), b, acc);
}

float8 __attribute__((overloadable))
mmad8x8_f16(ushort8 a, int8 b, float8 acc) {
    return intel_sub_group_f16_f16_matrix_mad_k16(as_short8(a), b, acc);
}

#if MATH_UTILS_DECLARE_BF16
float8 __attribute__((overloadable)) mmad8x8_bf16(uint8 a, int8 b, float8 acc) {
    return intel_sub_group_bf16_bf16_matrix_mad_k16(as_int8(a), b, acc);
}

float8 __attribute__((overloadable))
mmad8x8_bf16(ushort8 a, int8 b, float8 acc) {
    return intel_sub_group_bf16_bf16_matrix_mad_k16(as_short8(a), b, acc);
}

float8 __attribute__((overloadable))
mmad8x8_bf16(short8 a, int8 b, float8 acc) {
    return intel_sub_group_bf16_bf16_matrix_mad_k16(a, b, acc);
}

float4 __attribute__((overloadable))
mmad8x4_bf16(ushort4 a, int8 b, float4 acc) {
    return intel_sub_group_bf16_bf16_matrix_mad_k16(as_short4(a), b, acc);
}
#ifdef cl_intel_subgroup_split_matrix_multiply_accumulate

float8 mmad8x8_bf16_split(uint4 a, int8 b, float8 acc) {
    return intel_sub_group_f16_f16_split_matrix_mad_k16(as_int4(a), b, acc);
}

#endif //cl_intel_subgroup_split_matrix_multiply_accumulate
#endif //cl_intel_subgroup_matrix_multiply_accumulate

#else
DECLARE_MMAD_EMU(mmad8x4, idot4, 8, 4, uint4, int8, int4)
DECLARE_MMAD_EMU(mmad8x4, idot4, 8, 4, int4, int8, int4)
DECLARE_MMAD_EMU(mmad8x8, idot4, 8, 8, uint8, int8, int8)
DECLARE_MMAD_EMU(mmad8x8, idot4, 8, 8, int8, int8, int8)
DECLARE_MMAD_EMU(mmad8x8, idot4, 8, 8, ushort8, int8, int8)
DECLARE_MMAD_EMU(mmad8x8, idot4, 8, 8, short8, int8, int8)
DECLARE_MMAD_EMU(mmad8x4_f16, f16_dot2, 8, 4, uint4, int8, float4)
DECLARE_MMAD_EMU(mmad8x4_f16, f16_dot2, 8, 4, short4, int8, float4)
DECLARE_MMAD_EMU(mmad8x8_f16, f16_dot2, 8, 8, uint8, int8, float8)
DECLARE_MMAD_EMU(mmad8x8_f16, f16_dot2, 8, 8, short8, int8, float8)
#if MATH_UTILS_DECLARE_BF16
DECLARE_MMAD_EMU(mmad8x4_bf16, bf16_dot2, 8, 4, uint4, int8, float4)
DECLARE_MMAD_EMU(mmad8x8_bf16, bf16_dot2, 8, 8, uint8, int8, float8)
DECLARE_MMAD_EMU(mmad8x4_bf16, bf16_dot2, 8, 4, ushort4, int8, float4)
DECLARE_MMAD_EMU(mmad8x8_bf16, bf16_dot2, 8, 8, ushort8, int8, float8)
DECLARE_MMAD_EMU(mmad8x8_bf16, bf16_dot2, 8, 8, short8, int8, float8)
#endif

#endif

// Atomics
#if __OPENCL_C_VERSION__ >= 200
#ifdef cl_intel_global_float_atomics
inline void atomic_add_global(
        volatile global atomic_float *source, float operand) {
    atomic_fetch_add_explicit(source, operand, memory_order_relaxed);
}

#else // float atomics
inline void atomic_add_global(
        volatile __global atomic_float *source, float operand) {
    float old_val = atomic_load_explicit(
            source, memory_order_relaxed, memory_scope_device);
    bool success = false;
    do {
        float new_val = old_val + operand;
        success = atomic_compare_exchange_strong_explicit(source, &old_val,
                new_val, memory_order_acq_rel, memory_order_relaxed,
                memory_scope_device);
    } while (!success);
}
#endif
#endif

#endif

#define unroll_for __attribute__((opencl_unroll_hint)) for

#define for_ for

#define CONCAt2(a, b) a##b
#define CONCAT2(a, b) CONCAt2(a, b)
#define CONCAT3(a, b, c) CONCAT2(CONCAT2(a, b), c)

#if defined(DT_F16) || defined(SRC_DT_F16) || defined(SRC0_DT_F16) \
        || defined(SRC1_DT_F16) || defined(DST_DT_F16) || defined(WEI_DT_F16) \
        || defined(BIA_DT_F16) || defined(ACC_DT_F16)
#pragma OPENCL EXTENSION cl_khr_fp16 : enable
#endif

#if DT_F64
#pragma OPENCL EXTENSION cl_khr_fp64 : enable
#endif

#if DT_F32 == 1
#define DATA_T float
#define DATA2_T float2
#define DATA4_T float4
#define DATA8_T float8
#define DATA16_T float16
#define DATA_MAX FLT_MAX
#define DATA_MIN -DATA_MAX
#define DATA_ZERO 0.0f
#define DATA_ONE 1.0f
#define DEF_ACC_DATA_T float
#define DEF_ACC_DATA2_T float2
#define DEF_ACC_DATA4_T float4
#define DEF_ACC_DATA8_T float8
#define POST_OP_DATA_T float
#define TO_DATA_T(v) (float)(v)
#define TO_DEF_ACC_DATA_T(v) (float)(v)
#define DATA_TO_REF convert_float
#define CONVERT_DATA_T convert_float
#define CONVERT_DATA2_T convert_float2
#define CONVERT_DATA4_T convert_float4
#define CONVERT_DATA8_T convert_float8
#define CONVERT_FLOAT_T convert_float
#define CONVERT_FLOAT2_T convert_float2
#define CONVERT_FLOAT4_T convert_float4
#define CONVERT_FLOAT8_T convert_float8

#define BLOCK_READ intel_sub_group_block_read
#define BLOCK_WRITE intel_sub_group_block_write
#define BLOCK_READ2 intel_sub_group_block_read2
#define BLOCK_READ4 intel_sub_group_block_read4
#define BLOCK_READ8 intel_sub_group_block_read8
#define BLOCK_WRITE2 intel_sub_group_block_write2
#define BLOCK_WRITE4 intel_sub_group_block_write4
#define BLOCK_WRITE8 intel_sub_group_block_write8

#define AS_DATA_T as_float
#define AS_DATA2_T as_float2
#define AS_DATA4_T as_float4
#define AS_DATA8_T as_float8

#define AS_UINT_T as_uint
#define AS_UINT2_T as_uint2
#define AS_UINT4_T as_uint4
#define AS_UINT8_T as_uint8

#define BLOCK_DATA_T uint
#define BLOCK_DATA2_T uint2
#define BLOCK_DATA4_T uint4
#define BLOCK_DATA8_T uint8
#define AS_BLOCK_DATA_T as_uint
#define AS_BLOCK_DATA2_T as_uint2
#define AS_BLOCK_DATA4_T as_uint4
#define AS_BLOCK_DATA8_T as_uint8

#define MMAD_DATA_T uint
#define MMAD_DATA4_T uint4
#define MMAD_DATA8_T uint8
#define MMAD_ACC_DATA4_T float4
#define MMAD_ACC_DATA8_T float8

#elif DT_F64 == 1
#define DATA_T double
#define DATA2_T double2
#define DATA4_T double4
#define DATA8_T double8
#define DATA16_T double16
#define DATA_MAX DBL_MAX
#define DATA_MIN -DATA_MAX
#define DATA_ZERO 0.0d
#define DATA_ONE 1.0d
#define DEF_ACC_DATA_T double
#define DEF_ACC_DATA2_T double2
#define DEF_ACC_DATA4_T double4
#define DEF_ACC_DATA8_T double8
#define POST_OP_DATA_T double
#define TO_DATA_T(v) (double)(v)
#define TO_DEF_ACC_DATA_T(v) (double)(v)
#define DATA_TO_REF convert_float
#define CONVERT_DATA_T convert_double
#define CONVERT_DATA2_T convert_double2
#define CONVERT_DATA4_T convert_double4
#define CONVERT_DATA8_T convert_double8
#define CONVERT_FLOAT_T convert_float
#define CONVERT_FLOAT2_T convert_float2
#define CONVERT_FLOAT4_T convert_float4
#define CONVERT_FLOAT8_T convert_float8

#define AS_DATA_T as_double
#define AS_DATA2_T as_double2
#define AS_DATA4_T as_double4
#define AS_DATA8_T as_double8

#elif DT_F16 == 1

#define DATA_T half
#define DATA2_T half2
#define DATA4_T half4
#define DATA8_T half8
#define DATA16_T half16
#define AS_DATA2_T as_half2
#define DATA_MAX HALF_MAX
#define DATA_MIN -DATA_MAX
#define DATA_ZERO 0.0h
#define DATA_ONE 1.0h
#define DEF_ACC_DATA_T half
#define DEF_ACC_DATA2_T half2
#define DEF_ACC_DATA4_T half4
#define DEF_ACC_DATA8_T half8
#define POST_OP_DATA_T half
#define TO_DATA_T(v) (half)(v)
#define TO_DEF_ACC_DATA_T(v) (half)(v)
#define DATA_TO_REF convert_half
#define CONVERT_DATA_T convert_half
#define CONVERT_DATA2_T convert_half2
#define CONVERT_DATA4_T convert_half4
#define CONVERT_DATA8_T convert_half8
#define CONVERT_FLOAT_T convert_float
#define CONVERT_FLOAT2_T convert_float2
#define CONVERT_FLOAT4_T convert_float4
#define CONVERT_FLOAT8_T convert_float8

#define BLOCK_READ intel_sub_group_block_read_us
#define BLOCK_WRITE intel_sub_group_block_write_us
#define BLOCK_READ2 intel_sub_group_block_read_us2
#define BLOCK_READ4 intel_sub_group_block_read_us4
#define BLOCK_READ8 intel_sub_group_block_read_us8
#define BLOCK_WRITE2 intel_sub_group_block_write_us2
#define BLOCK_WRITE4 intel_sub_group_block_write_us4
#define BLOCK_WRITE8 intel_sub_group_block_write_us8
#define AS_DATA_T as_half
#define AS_DATA2_T as_half2
#define AS_DATA4_T as_half4
#define AS_DATA8_T as_half8

#define AS_UINT_T as_ushort
#define AS_UINT2_T as_ushort2
#define AS_UINT4_T as_ushort4
#define AS_UINT8_T as_ushort8

#define BLOCK_DATA_T ushort
#define BLOCK_DATA2_T ushort2
#define BLOCK_DATA4_T ushort4
#define BLOCK_DATA8_T ushort8
#define AS_BLOCK_DATA_T as_ushort
#define AS_BLOCK_DATA2_T as_ushort2
#define AS_BLOCK_DATA4_T as_ushort4
#define AS_BLOCK_DATA8_T as_ushort8

#define MMAD_DATA_T uint
#define MMAD_DATA4_T uint4
#define MMAD_DATA8_T uint8
#define MMAD_ACC_DATA4_T float4
#define MMAD_ACC_DATA8_T float8
#elif DT_BF16 == 1
#define DATA_T ushort
#define DATA2_T ushort2
#define POST_OP_DATA_T float
#define DATA2_T ushort2
#define DATA4_T ushort4
#define DATA8_T ushort8
#define DATA16_T ushort16
#define DATA_MAX as_float(0x7f7f0000)
#define DATA_MIN (-DATA_MAX)
#define DATA_ZERO 0.0f
#define DATA_ONE 1.0f
#define DEF_ACC_DATA_T float
#define DEF_ACC_DATA2_T float2
#define DEF_ACC_DATA4_T float4
#define DEF_ACC_DATA8_T float8
#define TO_DATA_T(v) cvt_f32_to_bf16(v)
#define TO_DEF_ACC_DATA_T(v) cvt_bf16_to_f32(v)
#define DATA_TO_REF cvt_bf16_to_f32
#define CONVERT_DATA_T cvt_f32_to_bf16
#define CONVERT_DATA2_T cvt_f32_to_bf16
#define CONVERT_DATA4_T cvt_f32_to_bf16
#define CONVERT_DATA8_T cvt_f32_to_bf16
#define CONVERT_FLOAT_T cvt_bf16_to_f32
#define CONVERT_FLOAT2_T cvt_bf16_to_f32
#define CONVERT_FLOAT4_T cvt_bf16_to_f32
#define CONVERT_FLOAT8_T cvt_bf16_to_f32

#define BLOCK_READ intel_sub_group_block_read_us
#define BLOCK_WRITE intel_sub_group_block_write_us
#define BLOCK_READ2 intel_sub_group_block_read_us2
#define BLOCK_READ4 intel_sub_group_block_read_us4
#define BLOCK_READ8 intel_sub_group_block_read_us8
#define BLOCK_WRITE2 intel_sub_group_block_write_us2
#define BLOCK_WRITE4 intel_sub_group_block_write_us4
#define BLOCK_WRITE8 intel_sub_group_block_write_us8
#define AS_DATA_T as_ushort
#define AS_DATA2_T as_ushort2
#define AS_DATA4_T as_ushort4
#define AS_DATA8_T as_ushort8

#define AS_UINT_T as_ushort
#define AS_UINT2_T as_ushort2
#define AS_UINT4_T as_ushort4
#define AS_UINT8_T as_ushort8

#define BLOCK_DATA_T ushort
#define BLOCK_DATA2_T ushort2
#define BLOCK_DATA4_T ushort4
#define BLOCK_DATA8_T ushort8
#define AS_BLOCK_DATA_T as_ushort
#define AS_BLOCK_DATA2_T as_ushort2
#define AS_BLOCK_DATA4_T as_ushort4
#define AS_BLOCK_DATA8_T as_ushort8

#define MMAD_DATA_T uint
#define MMAD_DATA4_T uint4
#define MMAD_DATA8_T uint8
#define MMAD_ACC_DATA4_T float4
#define MMAD_ACC_DATA8_T float8
#elif DT_S8 == 1
#define DATA_T char
#define DATA2_T char2
#define DATA4_T char4
#define DATA8_T char8
#define DATA16_T char16
#define DATA_MAX CHAR_MAX
#define DATA_MIN CHAR_MIN
#define DATA_ZERO 0
#define DATA_ONE 1
#define INT8_T int8
#define DEF_ACC_DATA_T int
#define DEF_ACC_DATA2_T int2
#define DEF_ACC_DATA4_T int4
#define DEF_ACC_DATA8_T int8
#define POST_OP_DATA_T float
#define TO_DATA_T(v) convert_char_sat_rte(v)
#define TO_DEF_ACC_DATA_T(v) (float)(v)
#define DATA_TO_REF convert_float
#define CONVERT_DATA_T convert_char_sat_rte
#define CONVERT_DATA2_T convert_char2_sat_rte
#define CONVERT_DATA4_T convert_char4_sat_rte
#define CONVERT_DATA8_T convert_char8_sat_rte
#define CONVERT_FLOAT_T convert_float
#define CONVERT_FLOAT2_T convert_float2
#define CONVERT_FLOAT4_T convert_float4
#define CONVERT_FLOAT8_T convert_float8

#define BLOCK_READ intel_sub_group_block_read_uc
#define BLOCK_WRITE intel_sub_group_block_write_uc
#define BLOCK_READ2 intel_sub_group_block_read_uc2
#define BLOCK_READ4 intel_sub_group_block_read_uc4
#define BLOCK_READ8 intel_sub_group_block_read_uc8
#define BLOCK_WRITE2 intel_sub_group_block_write_uc2
#define BLOCK_WRITE4 intel_sub_group_block_write_uc4
#define BLOCK_WRITE8 intel_sub_group_block_write_uc8
#define AS_DATA_T as_char
#define AS_DATA2_T as_char2
#define AS_DATA4_T as_char4
#define AS_DATA8_T as_char8
#define AS_DATA16_T as_char16

#define AS_UINT_T as_uchar
#define AS_UINT2_T as_uchar2
#define AS_UINT4_T as_uchar4
#define AS_UINT8_T as_uchar8
#define AS_INT8_T as_int8

#define BLOCK_DATA_T uchar
#define BLOCK_DATA2_T uchar2
#define BLOCK_DATA4_T uchar4
#define BLOCK_DATA8_T uchar8
#define AS_BLOCK_DATA_T as_uchar
#define AS_BLOCK_DATA2_T as_uchar2
#define AS_BLOCK_DATA4_T as_uchar4
#define AS_BLOCK_DATA8_T as_uchar8

#define MMAD_DATA_T int
#define MMAD_DATA4_T int4
#define MMAD_DATA8_T int8
#define MMAD_ACC_DATA4_T int4
#define MMAD_ACC_DATA8_T int8
#elif DT_U8 == 1
#define DATA_T uchar
#define DATA2_T uchar2
#define DATA4_T uchar4
#define DATA8_T uchar8
#define DATA16_T uchar16
#define DATA_MAX UCHAR_MAX
#define DATA_MIN 0
#define DATA_ZERO 0
#define DATA_ONE 1
#define INT8_T uint8
#define DEF_ACC_DATA_T int
#define DEF_ACC_DATA2_T int2
#define DEF_ACC_DATA4_T int4
#define DEF_ACC_DATA8_T int8
#define POST_OP_DATA_T float
#define TO_DATA_T(v) convert_uchar_sat_rte(v)
#define TO_DEF_ACC_DATA_T(v) (float)(v)
#define DATA_TO_REF convert_float
#define CONVERT_DATA_T convert_uchar_sat_rte
#define CONVERT_DATA2_T convert_uchar2_sat_rte
#define CONVERT_DATA4_T convert_uchar4_sat_rte
#define CONVERT_DATA8_T convert_uchar8_sat_rte
#define CONVERT_FLOAT_T convert_float
#define CONVERT_FLOAT2_T convert_float2
#define CONVERT_FLOAT4_T convert_float4
#define CONVERT_FLOAT8_T convert_float8

#define BLOCK_READ intel_sub_group_block_read_uc
#define BLOCK_WRITE intel_sub_group_block_write_uc
#define BLOCK_READ2 intel_sub_group_block_read_uc2
#define BLOCK_READ4 intel_sub_group_block_read_uc4
#define BLOCK_READ8 intel_sub_group_block_read_uc8
#define BLOCK_WRITE2 intel_sub_group_block_write_uc2
#define BLOCK_WRITE4 intel_sub_group_block_write_uc4
#define BLOCK_WRITE8 intel_sub_group_block_write_uc8
#define AS_DATA_T as_uchar
#define AS_DATA2_T as_uchar2
#define AS_DATA4_T as_uchar4
#define AS_DATA8_T as_uchar8
#define AS_DATA16_T as_uchar16

#define AS_UINT_T as_uchar
#define AS_UINT2_T as_uchar2
#define AS_UINT4_T as_uchar4
#define AS_UINT8_T as_uchar8
#define AS_INT8_T as_uint8

#define BLOCK_DATA_T uchar
#define BLOCK_DATA2_T uchar2
#define BLOCK_DATA4_T uchar4
#define BLOCK_DATA8_T uchar8
#define AS_BLOCK_DATA_T as_uchar
#define AS_BLOCK_DATA2_T as_uchar2
#define AS_BLOCK_DATA4_T as_uchar4
#define AS_BLOCK_DATA8_T as_uchar8

#define MMAD_DATA_T uint
#define MMAD_DATA4_T uint4
#define MMAD_DATA8_T uint8
#define MMAD_ACC_DATA4_T int4
#define MMAD_ACC_DATA8_T int8
#elif DT_S32 == 1
#define MMAD_DATA_T uint
#define MMAD_DATA4_T uint4
#define MMAD_DATA8_T uint8
#define DATA_T int
#define DATA2_T int2
#define DATA4_T int4
#define DATA8_T int8
#define DATA16_T int16
#define DATA_TO_REF convert_float
#define CONVERT_DATA_T convert_int_sat_rte
#define CONVERT_DATA2_T convert_int2_sat_rte
#define CONVERT_DATA4_T convert_int4_sat_rte
#define CONVERT_DATA8_T convert_int8_sat_rte
#define CONVERT_FLOAT_T convert_float
#define CONVERT_FLOAT2_T convert_float2
#define CONVERT_FLOAT4_T convert_float4
#define CONVERT_FLOAT8_T convert_float8
#define POST_OP_DATA_T float
#define DATA_MIN INT_MIN
#define DATA_MAX INT_MAX
#define DATA_ZERO 0
#define ROUND

#define BLOCK_READ intel_sub_group_block_read
#define BLOCK_WRITE intel_sub_group_block_write
#define BLOCK_READ2 intel_sub_group_block_read2
#define BLOCK_READ4 intel_sub_group_block_read4
#define BLOCK_READ8 intel_sub_group_block_read8
#define BLOCK_WRITE2 intel_sub_group_block_write2
#define BLOCK_WRITE4 intel_sub_group_block_write4
#define BLOCK_WRITE8 intel_sub_group_block_write8

#define AS_DATA_T as_int
#define AS_DATA2_T as_int2
#define AS_DATA4_T as_int4
#define AS_DATA8_T as_int8

#define AS_UINT_T as_uint
#define AS_UINT2_T as_uint2
#define AS_UINT4_T as_uint4
#define AS_UINT8_T as_uint8

#define BLOCK_DATA_T uint
#define BLOCK_DATA2_T uint2
#define BLOCK_DATA4_T uint4
#define BLOCK_DATA8_T uint8
#define AS_BLOCK_DATA_T as_uint
#define AS_BLOCK_DATA2_T as_uint2
#define AS_BLOCK_DATA4_T as_uint4
#define AS_BLOCK_DATA8_T as_uint8
#elif !defined(DT_UNDEF)
#error "Unexpected data type"
#endif

#if VECT_DT_N == 1
#define VECT_DATA_T DATA_T
#define VECT_DEF_ACC_DATA_T DEF_ACC_DATA_T
#define AS_VECT_DATA_T AS_DATA_T
#define VECT_BLOCK_READ BLOCK_READ
#define VECT_BLOCK_WRITE BLOCK_WRITE
#define VECT_UINT_READ intel_sub_group_block_read
#define VECT_UINT_WRITE intel_sub_group_block_write
#define VECT_UCHAR_READ intel_sub_group_block_read_uc
#define VECT_UCHAR_WRITE intel_sub_group_block_write_uc
#define VECT_BLOCK_DATA_T BLOCK_DATA_T
#define AS_VECT_BLOCK_DATA_T AS_BLOCK_DATA_T
#define CONVERT_VECT_FLOAT_T CONVERT_FLOAT_T
#define CONVERT_VECTOR_DATA_T CONVERT_DATA_T
#define CONVERT_VECT_CHAR_T convert_char
#define CONVERT_VECT_INT_T convert_int
#define VECT_INT_T int
#define VECT_UINT_T uint
#define VECT_FLOAT_T float
#define VECT_CHAR_T char
#define AS_VECT_INT_T as_int
#define AS_VECT_UINT_T as_uint
#define AS_VECT_FLOAT_T as_float
#define AS_VECT_CHAR_T as_char
#define AS_VECT_UCHAR_T as_uchar
#elif VECT_DT_N == 2
#define VECT_DATA_T DATA2_T
#define VECT_DEF_ACC_DATA_T DEF_ACC_DATA2_T
#define AS_VECT_DATA_T AS_DATA2_T
#define VECT_BLOCK_READ BLOCK_READ2
#define VECT_BLOCK_WRITE BLOCK_WRITE2
#define VECT_UINT_READ intel_sub_group_block_read2
#define VECT_UINT_WRITE intel_sub_group_block_write2
#define VECT_UCHAR_READ intel_sub_group_block_read_uc2
#define VECT_UCHAR_WRITE intel_sub_group_block_write_uc2
#define VECT_BLOCK_DATA_T BLOCK_DATA2_T
#define AS_VECT_BLOCK_DATA_T AS_BLOCK_DATA2_T
#define CONVERT_VECT_FLOAT_T CONVERT_FLOAT2_T
#define CONVERT_VECTOR_DATA_T CONVERT_DATA2_T
#define CONVERT_VECT_CHAR_T convert_char2
#define CONVERT_VECT_INT_T convert_int2
#define VECT_INT_T int2
#define VECT_UINT_T uint2
#define VECT_FLOAT_T float2
#define VECT_CHAR_T char2
#define AS_VECT_INT_T as_int2
#define AS_VECT_UINT_T as_uint2
#define AS_VECT_FLOAT_T as_float2
#define AS_VECT_CHAR_T as_char2
#define AS_VECT_UCHAR_T as_uchar2
#elif VECT_DT_N == 4
#define VECT_DATA_T DATA4_T
#define VECT_DEF_ACC_DATA_T DEF_ACC_DATA4_T
#define AS_VECT_DATA_T AS_DATA4_T
#define VECT_BLOCK_READ BLOCK_READ4
#define VECT_BLOCK_WRITE BLOCK_WRITE4
#define VECT_UINT_READ intel_sub_group_block_read4
#define VECT_UINT_WRITE intel_sub_group_block_write4
#define VECT_UCHAR_READ intel_sub_group_block_read_uc4
#define VECT_UCHAR_WRITE intel_sub_group_block_write_uc4
#define VECT_BLOCK_DATA_T BLOCK_DATA4_T
#define AS_VECT_BLOCK_DATA_T AS_BLOCK_DATA4_T
#define CONVERT_VECT_FLOAT_T CONVERT_FLOAT4_T
#define CONVERT_VECTOR_DATA_T CONVERT_DATA4_T
#define CONVERT_VECT_CHAR_T convert_char4
#define CONVERT_VECT_INT_T convert_int4
#define VECT_INT_T int4
#define VECT_UINT_T uint4
#define VECT_FLOAT_T float4
#define VECT_CHAR_T char4
#define AS_VECT_INT_T as_int4
#define AS_VECT_UINT_T as_uint4
#define AS_VECT_FLOAT_T as_float4
#define AS_VECT_CHAR_T as_char4
#define AS_VECT_UCHAR_T as_uchar4
#elif VECT_DT_N == 8
#define VECT_DATA_T DATA8_T
#define VECT_DEF_ACC_DATA_T DEF_ACC_DATA8_T
#define AS_VECT_DATA_T AS_DATA8_T
#define VECT_BLOCK_READ BLOCK_READ8
#define VECT_BLOCK_WRITE BLOCK_WRITE8
#define VECT_UINT_READ intel_sub_group_block_read8
#define VECT_UINT_WRITE intel_sub_group_block_write8
#define VECT_UCHAR_READ intel_sub_group_block_read_uc8
#define VECT_UCHAR_WRITE intel_sub_group_block_write_uc8
#define VECT_BLOCK_DATA_T BLOCK_DATA8_T
#define AS_VECT_BLOCK_DATA_T AS_BLOCK_DATA8_T
#define CONVERT_VECT_FLOAT_T CONVERT_FLOAT8_T
#define CONVERT_VECTOR_DATA_T CONVERT_DATA8_T
#define CONVERT_VECT_CHAR_T convert_char8
#define CONVERT_VECT_INT_T convert_int8
#define VECT_INT_T int8
#define VECT_UINT_T uint8
#define VECT_FLOAT_T float8
#define VECT_CHAR_T char8
#define AS_VECT_INT_T as_int8
#define AS_VECT_UINT_T as_uint8
#define AS_VECT_FLOAT_T as_float8
#define AS_VECT_CHAR_T as_char8
#define AS_VECT_UCHAR_T as_uchar8
#endif

#define AS_MMAD_DATA_T CONCAT2(as_, MMAD_DATA_T)
#define AS_MMAD_DATA4_T CONCAT2(as_, MMAD_DATA4_T)
#define AS_MMAD_DATA8_T CONCAT2(as_, MMAD_DATA8_T)

#ifdef SRC_DATA_T
#define SRC_DATA2_T CONCAT2(SRC_DATA_T, 2)
#define SRC_DATA4_T CONCAT2(SRC_DATA_T, 4)
#define SRC_DATA8_T CONCAT2(SRC_DATA_T, 8)
#define SRC_DATA16_T CONCAT2(SRC_DATA_T, 16)
#ifdef SRC_DT_U8
#define SRC_MMAD_DATA_T uint
#define SRC_MMAD_DATA4_T uint4
#define SRC_MMAD_DATA8_T uint8
#elif SRC_DT_S8
#define SRC_MMAD_DATA_T int
#define SRC_MMAD_DATA4_T int4
#define SRC_MMAD_DATA8_T int8
#elif SRC_DT_F16 || SRC_DT_BF16
#define SRC_MMAD_DATA_T uint
#define SRC_MMAD_DATA4_T uint4
#define SRC_MMAD_DATA8_T uint8
#endif

#if defined(SRC_DT_U8) || defined(SRC_DT_S8)
#define SRC_MMAD_ACC_DATA4_T int4
#define SRC_MMAD_ACC_DATA8_T int8
#else
#define SRC_MMAD_ACC_DATA4_T float4
#define SRC_MMAD_ACC_DATA8_T float8
#endif

#define AS_SRC_DATA2_T CONCAT2(as_, SRC_DATA2_T)
#define AS_SRC_DATA4_T CONCAT2(as_, SRC_DATA4_T)
#define AS_SRC_DATA8_T CONCAT2(as_, SRC_DATA8_T)
#define AS_SRC_DATA16_T CONCAT2(as_, SRC_DATA16_T)
#define AS_SRC_MMAD_DATA_T CONCAT2(as_, SRC_MMAD_DATA_T)
#define AS_SRC_MMAD_DATA4_T CONCAT2(as_, SRC_MMAD_DATA4_T)
#define AS_SRC_MMAD_DATA8_T CONCAT2(as_, SRC_MMAD_DATA8_T)
#if SRC_DT_BF16
#define SRC_TO_REF(x) cvt_bf16_to_f32(x)
#define SRC_TO_REF8(x) cvt_bf16_to_f32(x)
#define REF_TO_SRC(x) cvt_f32_to_bf16(x)
#else
#define SRC_TO_REF(x) (x)
#define SRC_TO_REF8(x) (x)
#define REF_TO_SRC(x) (x)
#endif
#if SRC_DT_BF16
#define TO_SRC(x) cvt_f32_to_bf16(x)
#elif SRC_DT_U8
#define TO_SRC(x) convert_uchar_sat_rte(x)
#elif SRC_DT_S8
#define TO_SRC(x) convert_char_sat_rte(x)
#elif SRC_DT_S32
#define TO_SRC(x) convert_int_sat_rte(x)
#else
#define TO_SRC(x) (x)
#endif
#endif

#ifdef A_DATA_T
#define A_DATA8_T CONCAT2(A_DATA_T, 8)
#if A_DT_BF16
#define A_TO_REF(x) cvt_bf16_to_f32(x)
#define A_TO_REF8(x) cvt_bf16_to_f32(x)
#define REF_TO_A(x) cvt_f32_to_bf16(x)
#else
#define A_TO_REF(x) (x)
#define A_TO_REF8(x) (x)
#define REF_TO_A(x) (x)
#endif
#if A_DT_BF16
#define TO_A(x) cvt_f32_to_bf16(x)
#elif A_DT_U8
#define TO_A(x) convert_uchar_sat_rte(x)
#elif A_DT_S8
#define TO_A(x) convert_char_sat_rte(x)
#elif A_DT_S32
#define TO_A(x) convert_int_sat_rte(x)
#else
#define TO_A(x) (x)
#endif
#endif

#ifdef WEI_DATA_T
#if WEI_DT_BF16
#define WEI_TO_REF(x) cvt_bf16_to_f32(x)
#define REF_TO_WEI(x) cvt_f32_to_bf16(x)
#else
#define WEI_TO_REF(x) (x)
#define REF_TO_WEI(x) (x)
#endif
#if WEI_DT_BF16
#define TO_WEI(x) cvt_f32_to_bf16(x)
#elif WEI_DT_U8
#define TO_WEI(x) convert_uchar_sat_rte(x)
#elif WEI_DT_S8
#define TO_WEI(x) convert_char_sat_rte(x)
#elif WEI_DT_S32
#define TO_WEI(x) convert_int_sat_rte(x)
#else
#define TO_WEI(x) (x)
#endif
#endif

#ifdef DIFF_WEI_DATA_T
#if DIFF_WEI_DT_BF16
#define DIFF_WEI_TO_REF(x) cvt_bf16_to_f32(x)
#define REF_TO_DIFF_WEI(x) cvt_f32_to_bf16(x)
#else
#define DIFF_WEI_TO_REF(x) (x)
#define REF_TO_DIFF_WEI(x) (x)
#endif
#if DIFF_WEI_DT_BF16
#define TO_DIFF_WEI(x) cvt_f32_to_bf16(x)
#elif DIFF_WEI_DT_U8
#define TO_DIFF_WEI(x) convert_uchar_sat_rte(x)
#elif DIFF_WEI_DT_S8
#define TO_DIFF_WEI(x) convert_char_sat_rte(x)
#elif DIFF_WEI_DT_S32
#define TO_DIFF_WEI(x) convert_int_sat_rte(x)
#else
#define TO_DIFF_WEI(x) (x)
#endif
#endif

#ifdef B_DATA_T
#if B_DT_BF16
#define B_TO_REF(x) cvt_bf16_to_f32(x)
#define REF_TO_B(x) cvt_f32_to_bf16(x)
#else
#define B_TO_REF(x) (x)
#define REF_TO_B(x) (x)
#endif
#if B_DT_BF16
#define TO_B(x) cvt_f32_to_bf16(x)
#elif B_DT_U8
#define TO_B(x) convert_uchar_sat_rte(x)
#elif B_DT_S8
#define TO_B(x) convert_char_sat_rte(x)
#elif B_DT_S32
#define TO_B(x) convert_int_sat_rte(x)
#else
#define TO_B(x) (x)
#endif
#endif

#ifdef BIA_DATA_T
#define BIA_DATA2_T CONCAT2(BIA_DATA_T, 2)
#if BIA_DT_BF16
#define BIA_TO_REF(x) cvt_bf16_to_f32(x)
#define REF_TO_BIA(x) cvt_f32_to_bf16(x)
#else
#define BIA_TO_REF(x) (x)
#define REF_TO_BIA(x) (x)
#endif
#if BIA_DT_BF16
#define TO_BIA(x) cvt_f32_to_bf16(x)
#elif BIA_DT_U8
#define TO_BIA(x) convert_uchar_sat_rte(x)
#elif BIA_DT_S8
#define TO_BIA(x) convert_char_sat_rte(x)
#elif BIA_DT_S32
#define TO_BIA(x) convert_int_sat_rte(x)
#else
#define TO_BIA(x) (x)
#endif
#endif

#ifdef DST_DATA_T
#define DST_DATA2_T CONCAT2(DST_DATA_T, 2)
#define DST_DATA4_T CONCAT2(DST_DATA_T, 4)
#define DST_DATA8_T CONCAT2(DST_DATA_T, 8)
#define DST_DATA16_T CONCAT2(DST_DATA_T, 16)

#define AS_DST_DATA_T CONCAT2(as_, DST_DATA_T)
#define AS_DST_DATA2_T CONCAT2(as_, DST_DATA2_T)
#define AS_DST_DATA4_T CONCAT2(as_, DST_DATA4_T)
#define AS_DST_DATA8_T CONCAT2(as_, DST_DATA8_T)
#define AS_DST_DATA16_T CONCAT2(as_, DST_DATA16_T)

#if DST_DT_F32 || DST_DT_F16
#define CONVERT_DST_DATA_T CONCAT2(convert_, DST_DATA_T)
#define CONVERT_DST_DATA2_T CONCAT2(convert_, DST_DATA2_T)
#define CONVERT_DST_DATA4_T CONCAT2(convert_, DST_DATA4_T)
#define CONVERT_DST_DATA8_T CONCAT2(convert_, DST_DATA8_T)
#define CONVERT_DST_DATA16_T CONCAT2(convert_, DST_DATA16_T)
#else
#define CONVERT_DST_DATA_T CONCAT3(convert_, DST_DATA_T, _sat_rte)
#define CONVERT_DST_DATA2_T CONCAT3(convert_, DST_DATA2_T, _sat_rte)
#define CONVERT_DST_DATA4_T CONCAT3(convert_, DST_DATA4_T, _sat_rte)
#define CONVERT_DST_DATA8_T CONCAT3(convert_, DST_DATA8_T, _sat_rte)
#define CONVERT_DST_DATA16_T CONCAT3(convert_, DST_DATA16_T, _sat_rte)
#endif

#if DST_DT_U8
#define MMAD_DATA_T uint
#define MMAD_DATA4_T uint4
#define MMAD_DATA8_T uint8
#elif DST_DT_S8
#define MMAD_DATA_T int
#define MMAD_DATA4_T int4
#define MMAD_DATA8_T int8
#endif

// Block read/write macros for dst.
#if DST_DT_U8 || DST_DT_S8
#define BLOCK_READ_DST2(ptr) \
    AS_DST_DATA2_T(intel_sub_group_block_read_uc2((__global uchar *)ptr))
#define BLOCK_WRITE_DST2(ptr, v) \
    intel_sub_group_block_write_uc2((__global uchar *)ptr, as_uchar2(v))

#define BLOCK_READ_DST(ptr) \
    AS_DST_DATA_T(intel_sub_group_block_read_uc((__global uchar *)ptr))
#define BLOCK_WRITE_DST(ptr, v) \
    intel_sub_group_block_write_uc((__global uchar *)ptr, as_uchar(v))

#define BLOCK_READ_DST2(ptr) \
    AS_DST_DATA2_T(intel_sub_group_block_read_uc2((__global uchar *)ptr))
#define BLOCK_WRITE_DST2(ptr, v) \
    intel_sub_group_block_write_uc2((__global uchar *)ptr, as_uchar2(v))

#define BLOCK_READ_DST4(ptr) \
    AS_DST_DATA4_T(intel_sub_group_block_read_uc4((__global uchar *)ptr))
#define BLOCK_WRITE_DST4(ptr, v) \
    intel_sub_group_block_write_uc4((__global uchar *)ptr, as_uchar4(v))

#define BLOCK_READ_DST8(ptr) \
    AS_DST_DATA8_T(intel_sub_group_block_read_uc8((__global uchar *)ptr))
#define BLOCK_WRITE_DST8(ptr, v) \
    intel_sub_group_block_write_uc8((__global uchar *)ptr, as_uchar8(v))

#define BLOCK_READ_DST16(ptr) \
    AS_DST_DATA16_T(intel_sub_group_block_read_uc16((__global uchar *)ptr))
#define BLOCK_WRITE_DST16(ptr, v) \
    intel_sub_group_block_write_uc16((__global uchar *)ptr, as_uchar16(v))

#elif DST_DT_F16 || DST_DT_BF16
#define BLOCK_READ_DST(ptr) \
    AS_DST_DATA_T(intel_sub_group_block_read_us((__global ushort *)ptr))
#define BLOCK_WRITE_DST(ptr, v) \
    intel_sub_group_block_write_us((__global ushort *)ptr, as_ushort(v))

#define BLOCK_READ_DST2(ptr) \
    AS_DST_DATA2_T(intel_sub_group_block_read_us2((__global ushort *)ptr))
#define BLOCK_WRITE_DST2(ptr, v) \
    intel_sub_group_block_write_us2((__global ushort *)ptr, as_ushort2(v))

#define BLOCK_READ_DST4(ptr) \
    AS_DST_DATA4_T(intel_sub_group_block_read_us4((__global ushort *)ptr))
#define BLOCK_WRITE_DST4(ptr, v) \
    intel_sub_group_block_write_us4((__global ushort *)ptr, as_ushort4(v))

#define BLOCK_READ_DST8(ptr) \
    AS_DST_DATA8_T(intel_sub_group_block_read_us8((__global ushort *)ptr))
#define BLOCK_WRITE_DST8(ptr, v) \
    intel_sub_group_block_write_us8((__global ushort *)ptr, as_ushort8(v))

#define BLOCK_READ_DST16(ptr) \
    (DST_DATA16_T)( \
            BLOCK_READ_DST8(ptr), BLOCK_READ_DST8(ptr + 8 * SUB_GROUP_SIZE))
#define BLOCK_WRITE_DST16(ptr, v) \
    do { \
        BLOCK_WRITE_DST8(ptr, (v).s01234567); \
        BLOCK_WRITE_DST8(ptr + 8 * SUB_GROUP_SIZE, (v).s89abcdef); \
    } while (0)

#elif DST_DT_S32 || DST_DT_F32

#define BLOCK_READ_DST(ptr) \
    AS_DST_DATA_T(intel_sub_group_block_read((__global uint *)ptr))
#define BLOCK_WRITE_DST(ptr, v) \
    intel_sub_group_block_write((__global uint *)ptr, as_uint(v))

#define BLOCK_READ_DST2(ptr) \
    AS_DST_DATA2_T(intel_sub_group_block_read2((__global uint *)ptr))
#define BLOCK_WRITE_DST2(ptr, v) \
    intel_sub_group_block_write2((__global uint *)ptr, as_uint2(v))

#define BLOCK_READ_DST4(ptr) \
    AS_DST_DATA4_T(intel_sub_group_block_read4((__global uint *)ptr))
#define BLOCK_WRITE_DST4(ptr, v) \
    intel_sub_group_block_write4((__global uint *)ptr, as_uint4(v))

#define BLOCK_READ_DST8(ptr) \
    AS_DST_DATA8_T(intel_sub_group_block_read8((__global uint *)ptr))
#define BLOCK_WRITE_DST8(ptr, v) \
    intel_sub_group_block_write8((__global uint *)ptr, as_uint8(v))

#define BLOCK_READ_DST16(ptr) \
    (DST_DATA16_T)( \
            BLOCK_READ_DST8(ptr), BLOCK_READ_DST8(ptr + 8 * SUB_GROUP_SIZE))
#define BLOCK_WRITE_DST16(ptr, v) \
    do { \
        BLOCK_WRITE_DST8(ptr, (v).s01234567); \
        BLOCK_WRITE_DST8(ptr + 8 * SUB_GROUP_SIZE, (v).s89abcdef); \
    } while (0)

#elif DST_DT_F16 || DST_DT_BF16

#define BLOCK_READ_DST(ptr) \
    AS_DST_DATA_T(intel_sub_group_block_read_us((__global ushort *)ptr))
#define BLOCK_WRITE_DST(ptr, v) \
    intel_sub_group_block_write_us((__global ushort *)ptr, as_ushort(v))

#define BLOCK_READ_DST2(ptr) \
    AS_DST_DATA2_T(intel_sub_group_block_read_us2((__global ushort *)ptr))
#define BLOCK_WRITE_DST2(ptr, v) \
    intel_sub_group_block_write_us2((__global ushort *)ptr, as_short2(v))

#define BLOCK_READ_DST4(ptr) \
    AS_DST_DATA4_T(intel_sub_group_block_read_us4((__global ushort *)ptr))
#define BLOCK_WRITE_DST4(ptr, v) \
    intel_sub_group_block_write_us4((__global ushort *)ptr, as_ushort4(v))

#define BLOCK_READ_DST8(ptr) \
    AS_DST_DATA8_T(intel_sub_group_block_read_us8((__global ushort *)ptr))
#define BLOCK_WRITE_DST8(ptr, v) \
    intel_sub_group_block_write_us8((__global ushort *)ptr, as_ushort8(v))

#define BLOCK_READ_DST16(ptr) \
    (DST_DATA16_T)( \
            BLOCK_READ_DST8(ptr), BLOCK_READ_DST8(ptr + 8 * SUB_GROUP_SIZE))
#define BLOCK_WRITE_DST16(ptr, v) \
    do { \
        BLOCK_WRITE_DST8(ptr, (v).s01234567); \
        BLOCK_WRITE_DST8(ptr + 8 * SUB_GROUP_SIZE, (v).s89abcdef); \
    } while (0)

#endif

#if DST_DT_BF16
#define DST_TO_REF(x) cvt_bf16_to_f32(x)
#define DST_TO_REF2(x) cvt_bf16_to_f32(x)
#define DST_TO_REF8(x) cvt_bf16_to_f32(x)
#define REF_TO_DST(x) cvt_f32_to_bf16(x)
#define REF_TO_DST8(x) cvt_f32_to_bf16(convert_float8(x))
#elif DST_DT_F16
#define REF_TO_DST(x) convert_half(x)
#define DST_TO_REF(x) convert_float(x)
#define DST_TO_REF2(x) convert_float2(x)
#define DST_TO_REF8(x) convert_float8(x)
#elif DST_DT_U8
#define DST_TO_REF(x) (x)
#define DST_TO_REF2(x) (x)
#define DST_TO_REF8(x) (x)
#define REF_TO_DST(x) convert_uchar(x)
#define REF_TO_DST8(x) convert_uchar8(x)
#elif DST_DT_S8
#define DST_TO_REF(x) (x)
#define DST_TO_REF2(x) (x)
#define DST_TO_REF8(x) (x)
#define REF_TO_DST(x) convert_char(x)
#define REF_TO_DST8(x) convert_char8(x)
#else
#define DST_TO_REF(x) (x)
#define DST_TO_REF2(x) (x)
#define DST_TO_REF8(x) (x)
#define REF_TO_DST(x) (x)
#define REF_TO_DST8(x) (x)
#endif
#if DST_DT_BF16
#define TO_DST(x) cvt_f32_to_bf16(x)
#define TO_DST2(x) cvt_f32_to_bf16(convert_float2(x))
#define TO_DST4(x) cvt_f32_to_bf16(convert_float4(x))
#define TO_DST8(x) cvt_f32_to_bf16(convert_float8(x))
#elif DST_DT_F16
#define TO_DST(x) convert_half(x)
#define TO_DST2(x) convert_half2(x)
#define TO_DST4(x) convert_half4(x)
#define TO_DST8(x) convert_half8(x)
#elif DST_DT_U8
#define TO_DST(x) convert_uchar_sat_rte(x)
#define TO_DST2(x) convert_uchar2_sat_rte(x)
#define TO_DST4(x) convert_uchar4_sat_rte(x)
#define TO_DST8(x) convert_uchar8_sat_rte(x)
#define TO_DST16(x) convert_uchar16_sat_rte(x)
#elif DST_DT_S8
#define TO_DST(x) convert_char_sat_rte(x)
#define TO_DST2(x) convert_char2_sat_rte(x)
#define TO_DST4(x) convert_char4_sat_rte(x)
#define TO_DST8(x) convert_char8_sat_rte(x)
#define TO_DST16(x) convert_char16_sat_rte(x)
#elif DST_DT_S32
#define TO_DST(x) convert_int_sat_rte(x)
#define TO_DST2(x) convert_int2_sat_rte(x)
#define TO_DST4(x) convert_int4_sat_rte(x)
#define TO_DST8(x) convert_int8_sat_rte(x)
#elif DST_DT_F32
#define TO_DST(x) convert_float(x)
#define TO_DST2(x) convert_float2(x)
#define TO_DST4(x) convert_float4(x)
#define TO_DST8(x) convert_float8(x)
#elif DST_DT_F64
#define TO_DST(x) convert_double(x)
#define TO_DST2(x) convert_double2(x)
#define TO_DST4(x) convert_double4(x)
#define TO_DST8(x) convert_double8(x)
#else
#error "Not expected"
#endif
#endif

#ifdef C_DATA_T
#define C_DATA8_T CONCAT2(C_DATA_T, 8)
#if C_DT_BF16
#define C_TO_REF(x) cvt_bf16_to_f32(x)
#define C_TO_REF8(x) cvt_bf16_to_f32(x)
#define REF_TO_C(x) cvt_f32_to_bf16(x)
#define REF_TO_C8(x) cvt_f32_to_bf16(convert_float8(x))
#else
#define C_TO_REF(x) (x)
#define C_TO_REF8(x) (x)
#define REF_TO_C(x) (x)
#define REF_TO_C8(x) (x)
#endif
#if C_DT_BF16
#define TO_C(x) cvt_f32_to_bf16(x)
#define TO_C8(x) cvt_f32_to_bf16(convert_float8(x))
#elif C_DT_F16
#define TO_C(x) convert_half(x)
#define TO_C8(x) convert_half8(x)
#elif C_DT_U8
#define TO_C(x) convert_uchar_sat_rte(x)
#define TO_C8(x) convert_uchar8_sat_rte(x)
#elif C_DT_S8
#define TO_C(x) convert_char_sat_rte(x)
#define TO_C8(x) convert_char8_sat_rte(x)
#elif C_DT_S32
#define TO_C(x) convert_int_sat_rte(x)
#define TO_C8(x) convert_int8_sat_rte(x)
#elif C_DT_F32
#define TO_C(x) convert_float(x)
#define TO_C8(x) convert_float8(x)
#elif C_DT_F64
#define TO_C(x) convert_double(x)
#define TO_C8(x) convert_double8(x)
#else
#error "Not expected"
#endif
#endif

#ifdef ACC_DATA_T
#if ACC_DT_F16
#define TO_ACC(x) convert_half(x)
#elif ACC_DT_F32
#define TO_ACC(x) convert_float(x)
#elif ACC_DT_F64
#define TO_ACC(x) convert_double(x)
#elif ACC_DT_S32
#define TO_ACC(x) convert_int(x)
#else
#error "Unexpected accumulation data type"
#endif
#endif

#ifdef SUM_DATA_T
#define SUM_DATA2_T CONCAT2(SUM_DATA_T, 2)
#define SUM_DATA4_T CONCAT2(SUM_DATA_T, 4)
#define SUM_DATA8_T CONCAT2(SUM_DATA_T, 8)
#define SUM_DATA16_T CONCAT2(SUM_DATA_T, 16)
#define AS_SUM_DATA_T CONCAT2(as_, SUM_DATA_T)
#define AS_SUM_DATA2_T CONCAT2(as_, SUM_DATA2_T)
#define AS_SUM_DATA4_T CONCAT2(as_, SUM_DATA4_T)
#define AS_SUM_DATA8_T CONCAT2(as_, SUM_DATA8_T)
#define AS_SUM_DATA16_T CONCAT2(as_, SUM_DATA16_T)
#if SUM_DT_BF16
#define SUM_TO_REF cvt_bf16_to_f32
#else
#define SUM_TO_REF
#endif
#endif

#define OFF_MD_2(prefix, x0, x1, x2, x3, x4, x5) \
    ((((x0) / CONCAT2(prefix, _B0_2)) / CONCAT2(prefix, _B0_1) \
             * CONCAT2(prefix, _S0_0)) \
            + (((x0) / CONCAT2(prefix, _B0_2)) % CONCAT2(prefix, _B0_1) \
                    * CONCAT2(prefix, _S0_1)) \
            + (((x0) % CONCAT2(prefix, _B0_2)) * CONCAT2(prefix, _S0_2)) \
            + (((x1) / CONCAT2(prefix, _B1_2)) / CONCAT2(prefix, _B1_1) \
                    * CONCAT2(prefix, _S1_0)) \
            + (((x1) / CONCAT2(prefix, _B1_2)) % CONCAT2(prefix, _B1_1) \
                    * CONCAT2(prefix, _S1_1)) \
            + (((x1) % CONCAT2(prefix, _B1_2)) * CONCAT2(prefix, _S1_2)) \
            + (((x2) / CONCAT2(prefix, _B2_2)) / CONCAT2(prefix, _B2_1) \
                    * CONCAT2(prefix, _S2_0)) \
            + (((x2) / CONCAT2(prefix, _B2_2)) % CONCAT2(prefix, _B2_1) \
                    * CONCAT2(prefix, _S2_1)) \
            + (((x2) % CONCAT2(prefix, _B2_2)) * CONCAT2(prefix, _S2_2)) \
            + (((x3) / CONCAT2(prefix, _B3_2)) / CONCAT2(prefix, _B3_1) \
                    * CONCAT2(prefix, _S3_0)) \
            + (((x3) / CONCAT2(prefix, _B3_2)) % CONCAT2(prefix, _B3_1) \
                    * CONCAT2(prefix, _S3_1)) \
            + (((x3) % CONCAT2(prefix, _B3_2)) * CONCAT2(prefix, _S3_2)) \
            + (((x4) / CONCAT2(prefix, _B4_2)) / CONCAT2(prefix, _B4_1) \
                    * CONCAT2(prefix, _S4_0)) \
            + (((x4) / CONCAT2(prefix, _B4_2)) % CONCAT2(prefix, _B4_1) \
                    * CONCAT2(prefix, _S4_1)) \
            + (((x4) % CONCAT2(prefix, _B4_2)) * CONCAT2(prefix, _S4_2)) \
            + (((x5) / CONCAT2(prefix, _B5_2)) / CONCAT2(prefix, _B5_1) \
                    * CONCAT2(prefix, _S5_0)) \
            + (((x5) / CONCAT2(prefix, _B5_2)) % CONCAT2(prefix, _B5_1) \
                    * CONCAT2(prefix, _S5_1)) \
            + (((x5) % CONCAT2(prefix, _B5_2)) * CONCAT2(prefix, _S5_2)))

#define OFF_MD_3(prefix, x0, x1, x2, x3, x4, x5) \
    ((((((x0) / CONCAT2(prefix, _B0_3)) / CONCAT2(prefix, _B0_2)) \
              / CONCAT2(prefix, _B0_1)) \
             * CONCAT2(prefix, _S0_0)) \
            + (((((x0) / CONCAT2(prefix, _B0_3)) / CONCAT2(prefix, _B0_2)) \
                       % CONCAT2(prefix, _B0_1)) \
                    * CONCAT2(prefix, _S0_1)) \
            + ((((x0) / CONCAT2(prefix, _B0_3)) % CONCAT2(prefix, _B0_2)) \
                    * CONCAT2(prefix, _S0_2)) \
            + (((x0) % CONCAT2(prefix, _B0_3)) * CONCAT2(prefix, _S0_3)) \
            + (((((x1) / CONCAT2(prefix, _B1_3)) / CONCAT2(prefix, _B1_2)) \
                       / CONCAT2(prefix, _B1_1)) \
                    * CONCAT2(prefix, _S1_0)) \
            + (((((x1) / CONCAT2(prefix, _B1_3)) / CONCAT2(prefix, _B1_2)) \
                       % CONCAT2(prefix, _B1_1)) \
                    * CONCAT2(prefix, _S1_1)) \
            + ((((x1) / CONCAT2(prefix, _B1_3)) % CONCAT2(prefix, _B1_2)) \
                    * CONCAT2(prefix, _S1_2)) \
            + (((x1) % CONCAT2(prefix, _B1_3)) * CONCAT2(prefix, _S1_3)) \
            + (((((x2) / CONCAT2(prefix, _B2_3)) / CONCAT2(prefix, _B2_2)) \
                       / CONCAT2(prefix, _B2_1)) \
                    * CONCAT2(prefix, _S2_0)) \
            + (((((x2) / CONCAT2(prefix, _B2_3)) / CONCAT2(prefix, _B2_2)) \
                       % CONCAT2(prefix, _B2_1)) \
                    * CONCAT2(prefix, _S2_1)) \
            + ((((x2) / CONCAT2(prefix, _B2_3)) % CONCAT2(prefix, _B2_2)) \
                    * CONCAT2(prefix, _S2_2)) \
            + (((x2) % CONCAT2(prefix, _B2_3)) * CONCAT2(prefix, _S2_3)) \
            + (((((x3) / CONCAT2(prefix, _B3_3)) / CONCAT2(prefix, _B3_2)) \
                       / CONCAT2(prefix, _B3_1)) \
                    * CONCAT2(prefix, _S3_0)) \
            + (((((x3) / CONCAT2(prefix, _B3_3)) / CONCAT2(prefix, _B3_2)) \
                       % CONCAT2(prefix, _B3_1)) \
                    * CONCAT2(prefix, _S3_1)) \
            + ((((x3) / CONCAT2(prefix, _B3_3)) % CONCAT2(prefix, _B3_2)) \
                    * CONCAT2(prefix, _S3_2)) \
            + (((x3) % CONCAT2(prefix, _B3_3)) * CONCAT2(prefix, _S3_3)) \
            + (((((x4) / CONCAT2(prefix, _B4_3)) / CONCAT2(prefix, _B4_2)) \
                       / CONCAT2(prefix, _B4_1)) \
                    * CONCAT2(prefix, _S4_0)) \
            + (((((x4) / CONCAT2(prefix, _B4_3)) / CONCAT2(prefix, _B4_2)) \
                       % CONCAT2(prefix, _B4_1)) \
                    * CONCAT2(prefix, _S4_1)) \
            + ((((x4) / CONCAT2(prefix, _B4_3)) % CONCAT2(prefix, _B4_2)) \
                    * CONCAT2(prefix, _S4_2)) \
            + (((x4) % CONCAT2(prefix, _B4_3)) * CONCAT2(prefix, _S4_3)) \
            + (((((x5) / CONCAT2(prefix, _B5_3)) / CONCAT2(prefix, _B5_2)) \
                       / CONCAT2(prefix, _B5_1)) \
                    * CONCAT2(prefix, _S5_0)) \
            + (((((x5) / CONCAT2(prefix, _B5_3)) / CONCAT2(prefix, _B5_2)) \
                       % CONCAT2(prefix, _B5_1)) \
                    * CONCAT2(prefix, _S5_1)) \
            + ((((x5) / CONCAT2(prefix, _B5_3)) % CONCAT2(prefix, _B5_2)) \
                    * CONCAT2(prefix, _S5_2)) \
            + (((x5) % CONCAT2(prefix, _B5_3)) * CONCAT2(prefix, _S5_3)))

#define OFF_MD(prefix, x0, x1, x2, x3, x4, x5) \
    CONCAT2(OFF_MD_, CONCAT2(prefix, _NLEVELS))(prefix, x0, x1, x2, x3, x4, x5)

#if SRC_NDIMS == 3
#define CONV_SRC_OFF(n, c, d, h, w) OFF_MD(SRC, n, c, w, 0, 0, 0)
#elif SRC_NDIMS == 4
#define CONV_SRC_OFF(n, c, d, h, w) OFF_MD(SRC, n, c, h, w, 0, 0)
#elif SRC_NDIMS == 5
#define CONV_SRC_OFF(n, c, d, h, w) OFF_MD(SRC, n, c, d, h, w, 0)
#endif

#if WEI_NDIMS == 3
#define CONV_WEI_OFF(g, o, i, d, h, w) OFF_MD(WEI, o, i, w, 0, 0, 0)
#elif WEI_NDIMS == 4
#if WITH_GROUPS == 0
#define CONV_WEI_OFF(g, o, i, d, h, w) OFF_MD(WEI, o, i, h, w, 0, 0)
#else
#define CONV_WEI_OFF(g, o, i, d, h, w) OFF_MD(WEI, g, o, i, w, 0, 0)
#endif
#elif WEI_NDIMS == 5
#if WITH_GROUPS == 0
#define CONV_WEI_OFF(g, o, i, d, h, w) OFF_MD(WEI, o, i, d, h, w, 0)
#else
#define CONV_WEI_OFF(g, o, i, d, h, w) OFF_MD(WEI, g, o, i, h, w, 0)
#endif
#elif WEI_NDIMS == 6
#define CONV_WEI_OFF(g, o, i, d, h, w) OFF_MD(WEI, g, o, i, d, h, w)
#endif

#if DST_NDIMS == 3
#define CONV_DST_OFF(n, c, d, h, w) OFF_MD(DST, n, c, w, 0, 0, 0)
#elif DST_NDIMS == 4
#define CONV_DST_OFF(n, c, d, h, w) OFF_MD(DST, n, c, h, w, 0, 0)
#elif DST_NDIMS == 5
#define CONV_DST_OFF(n, c, d, h, w) OFF_MD(DST, n, c, d, h, w, 0)
#endif

#if NDIMS == 2
#define SRC_OFF(x0, x1, d, h, w) \
    (((x0) % SRC_B0) * SRC_SB0 + ((x0) / SRC_B0) * SRC_S0 \
            + ((x1) % SRC_B1) * SRC_SB1 + ((x1) / SRC_B1) * SRC_S1)

#if WITH_GROUPS == 1
#define WEI_OFF(x0, x1, x2, d, h, w) \
    (((x0) % WEI_B0) * WEI_SB0 + ((x0) / WEI_B0) * WEI_S0 \
            + ((x1) % WEI_B1) * WEI_SB1 + ((x1) / WEI_B1) * WEI_S1 \
            + ((x2) % WEI_B2) * WEI_SB2 + ((x2) / WEI_B2) * WEI_S2)
#else
#define WEI_OFF(g, x0, x1, d, h, w) \
    (((x0) % WEI_B0) * WEI_SB0 + ((x0) / WEI_B0) * WEI_S0 \
            + ((x1) % WEI_B1) * WEI_SB1 + ((x1) / WEI_B1) * WEI_S1)
#endif

#define DST_OFF(x0, x1, d, h, w) \
    (((x0) % DST_B0) * DST_SB0 + ((x0) / DST_B0) * DST_S0 \
            + ((x1) % DST_B1) * DST_SB1 + ((x1) / DST_B1) * DST_S1)
#elif NDIMS == 3
#define SRC_OFF(x0, x1, d, h, x2) \
    (((x0) % SRC_B0) * SRC_SB0 + ((x0) / SRC_B0) * SRC_S0 \
            + ((x1) % SRC_B1) * SRC_SB1 + ((x1) / SRC_B1) * SRC_S1 \
            + ((x2) % SRC_B2) * SRC_SB2 + ((x2) / SRC_B2) * SRC_S2)

#if WITH_GROUPS == 1
#define WEI_OFF(x0, x1, x2, d, h, x3) \
    (((x0) % WEI_B0) * WEI_SB0 + ((x0) / WEI_B0) * WEI_S0 \
            + ((x1) % WEI_B1) * WEI_SB1 + ((x1) / WEI_B1) * WEI_S1 \
            + ((x2) % WEI_B2) * WEI_SB2 + ((x2) / WEI_B2) * WEI_S2 \
            + ((x3) % WEI_B3) * WEI_SB3 + ((x3) / WEI_B3) * WEI_S3)
#else
#define WEI_OFF(g, x0, x1, d, h, x2) \
    (((x0) % WEI_B0) * WEI_SB0 + ((x0) / WEI_B0) * WEI_S0 \
            + ((x1) % WEI_B1) * WEI_SB1 + ((x1) / WEI_B1) * WEI_S1 \
            + ((x2) % WEI_B2) * WEI_SB2 + ((x2) / WEI_B2) * WEI_S2)
#endif

#define DST_OFF(x0, x1, d, h, x2) \
    (((x0) % DST_B0) * DST_SB0 + ((x0) / DST_B0) * DST_S0 \
            + ((x1) % DST_B1) * DST_SB1 + ((x1) / DST_B1) * DST_S1 \
            + ((x2) % DST_B2) * DST_SB2 + ((x2) / DST_B2) * DST_S2)
#elif NDIMS == 4
#define SRC_OFF(x0, x1, d, x2, x3) \
    (((x0) % SRC_B0) * SRC_SB0 + ((x0) / SRC_B0) * SRC_S0 \
            + ((x1) % SRC_B1) * SRC_SB1 + ((x1) / SRC_B1) * SRC_S1 \
            + ((x2) % SRC_B2) * SRC_SB2 + ((x2) / SRC_B2) * SRC_S2 \
            + ((x3) % SRC_B3) * SRC_SB3 + ((x3) / SRC_B3) * SRC_S3)

#if WITH_GROUPS == 1
#define WEI_OFF(x0, x1, x2, d, x3, x4) \
    (((x0) % WEI_B0) * WEI_SB0 + ((x0) / WEI_B0) * WEI_S0 \
            + ((x1) % WEI_B1) * WEI_SB1 + ((x1) / WEI_B1) * WEI_S1 \
            + ((x2) % WEI_B2) * WEI_SB2 + ((x2) / WEI_B2) * WEI_S2 \
            + ((x3) % WEI_B3) * WEI_SB3 + ((x3) / WEI_B3) * WEI_S3 \
            + ((x4) % WEI_B4) * WEI_SB4 + ((x4) / WEI_B4) * WEI_S4)
#else
#define WEI_OFF(g, x1, x2, d, x3, x4) \
    (((x1) % WEI_B0) * WEI_SB0 + ((x1) / WEI_B0) * WEI_S0 \
            + ((x2) % WEI_B1) * WEI_SB1 + ((x2) / WEI_B1) * WEI_S1 \
            + ((x3) % WEI_B2) * WEI_SB2 + ((x3) / WEI_B2) * WEI_S2 \
            + ((x4) % WEI_B3) * WEI_SB3 + ((x4) / WEI_B3) * WEI_S3)
#endif

#define DST_OFF(x0, x1, d, x2, x3) \
    (((x0) % DST_B0) * DST_SB0 + ((x0) / DST_B0) * DST_S0 \
            + ((x1) % DST_B1) * DST_SB1 + ((x1) / DST_B1) * DST_S1 \
            + ((x2) % DST_B2) * DST_SB2 + ((x2) / DST_B2) * DST_S2 \
            + ((x3) % DST_B3) * DST_SB3 + ((x3) / DST_B3) * DST_S3)
#elif NDIMS == 5
#define SRC_OFF(x0, x1, x2, x3, x4) \
    (((x0) % SRC_B0) * SRC_SB0 + ((x0) / SRC_B0) * SRC_S0 \
            + ((x1) % SRC_B1) * SRC_SB1 + ((x1) / SRC_B1) * SRC_S1 \
            + ((x2) % SRC_B2) * SRC_SB2 + ((x2) / SRC_B2) * SRC_S2 \
            + ((x3) % SRC_B3) * SRC_SB3 + ((x3) / SRC_B3) * SRC_S3 \
            + ((x4) % SRC_B4) * SRC_SB4 + ((x4) / SRC_B4) * SRC_S4)

#if WITH_GROUPS == 1
#define WEI_OFF(x0, x1, x2, x3, x4, x5) \
    (((x0) % WEI_B0) * WEI_SB0 + ((x0) / WEI_B0) * WEI_S0 \
            + ((x1) % WEI_B1) * WEI_SB1 + ((x1) / WEI_B1) * WEI_S1 \
            + ((x2) % WEI_B2) * WEI_SB2 + ((x2) / WEI_B2) * WEI_S2 \
            + ((x3) % WEI_B3) * WEI_SB3 + ((x3) / WEI_B3) * WEI_S3 \
            + ((x4) % WEI_B4) * WEI_SB4 + ((x4) / WEI_B4) * WEI_S4 \
            + ((x5) % WEI_B5) * WEI_SB5 + ((x5) / WEI_B5) * WEI_S5)
#else
#define WEI_OFF(g, x1, x2, x3, x4, x5) \
    (((x1) % WEI_B0) * WEI_SB0 + ((x1) / WEI_B0) * WEI_S0 \
            + ((x2) % WEI_B1) * WEI_SB1 + ((x2) / WEI_B1) * WEI_S1 \
            + ((x3) % WEI_B2) * WEI_SB2 + ((x3) / WEI_B2) * WEI_S2 \
            + ((x4) % WEI_B3) * WEI_SB3 + ((x4) / WEI_B3) * WEI_S3 \
            + ((x5) % WEI_B4) * WEI_SB4 + ((x5) / WEI_B4) * WEI_S4)
#endif

#define DST_OFF(x0, x1, x2, x3, x4) \
    (((x0) % DST_B0) * DST_SB0 + ((x0) / DST_B0) * DST_S0 \
            + ((x1) % DST_B1) * DST_SB1 + ((x1) / DST_B1) * DST_S1 \
            + ((x2) % DST_B2) * DST_SB2 + ((x2) / DST_B2) * DST_S2 \
            + ((x3) % DST_B3) * DST_SB3 + ((x3) / DST_B3) * DST_S3 \
            + ((x4) % DST_B4) * DST_SB4 + ((x4) / DST_B4) * DST_S4)
#endif

// clang-format off

// Shortcut accessors for special cases.
// x - product of the current and outer dimensions in gws[idx]
// y - the current dimension
#define GWS_OP_ZERO(x, y) 0
#define GWS_OP_FIRST(x, y) (x)
#define GWS_OP_MOD(x, y) ((x) % (y))
#define ROUND_UP(a,b) (((a) + (b) - 1) / (b))

#define GWS0_GET_ID0() GWS0_OP0((get_global_id(GWS0_IDX0) / GWS0_STRIDE0), ROUND_UP(GWS0_DIM0, GWS0_BLOCK0)) / GWS0_VEC_SIZE0 * GWS0_VEC_SIZE0 * GWS0_BLOCK0
#define GWS0_GET_ID1() GWS0_OP1((get_global_id(GWS0_IDX1) / GWS0_STRIDE1), ROUND_UP(GWS0_DIM1, GWS0_BLOCK1)) / GWS0_VEC_SIZE1 * GWS0_VEC_SIZE1 * GWS0_BLOCK1
#define GWS0_GET_ID2() GWS0_OP2((get_global_id(GWS0_IDX2) / GWS0_STRIDE2), ROUND_UP(GWS0_DIM2, GWS0_BLOCK2)) / GWS0_VEC_SIZE2 * GWS0_VEC_SIZE2 * GWS0_BLOCK2
#define GWS0_GET_ID3() GWS0_OP3((get_global_id(GWS0_IDX3) / GWS0_STRIDE3), ROUND_UP(GWS0_DIM3, GWS0_BLOCK3)) / GWS0_VEC_SIZE3 * GWS0_VEC_SIZE3 * GWS0_BLOCK3
#define GWS0_GET_ID4() GWS0_OP4((get_global_id(GWS0_IDX4) / GWS0_STRIDE4), ROUND_UP(GWS0_DIM4, GWS0_BLOCK4)) / GWS0_VEC_SIZE4 * GWS0_VEC_SIZE4 * GWS0_BLOCK4
#define GWS0_GET_ID5() GWS0_OP5((get_global_id(GWS0_IDX5) / GWS0_STRIDE5), ROUND_UP(GWS0_DIM5, GWS0_BLOCK5)) / GWS0_VEC_SIZE5 * GWS0_VEC_SIZE5 * GWS0_BLOCK5

#define GWS0_GET_BLOCK0() GWS0_BLOCK0
#define GWS0_GET_BLOCK1() GWS0_BLOCK1
#define GWS0_GET_BLOCK2() GWS0_BLOCK2
#define GWS0_GET_BLOCK3() GWS0_BLOCK3
#define GWS0_GET_BLOCK4() GWS0_BLOCK4
#define GWS0_GET_BLOCK5() GWS0_BLOCK5

#define GWS1_GET_ID0() GWS1_OP0((get_global_id(GWS1_IDX0) / GWS1_STRIDE0), ROUND_UP(GWS1_DIM0, GWS1_BLOCK0)) / GWS1_VEC_SIZE0 * GWS1_VEC_SIZE0 * GWS1_BLOCK0
#define GWS1_GET_ID1() GWS1_OP1((get_global_id(GWS1_IDX1) / GWS1_STRIDE1), ROUND_UP(GWS1_DIM1, GWS1_BLOCK1)) / GWS1_VEC_SIZE1 * GWS1_VEC_SIZE1 * GWS1_BLOCK1
#define GWS1_GET_ID2() GWS1_OP2((get_global_id(GWS1_IDX2) / GWS1_STRIDE2), ROUND_UP(GWS1_DIM2, GWS1_BLOCK2)) / GWS1_VEC_SIZE2 * GWS1_VEC_SIZE2 * GWS1_BLOCK2
#define GWS1_GET_ID3() GWS1_OP3((get_global_id(GWS1_IDX3) / GWS1_STRIDE3), ROUND_UP(GWS1_DIM3, GWS1_BLOCK3)) / GWS1_VEC_SIZE3 * GWS1_VEC_SIZE3 * GWS1_BLOCK3
#define GWS1_GET_ID4() GWS1_OP4((get_global_id(GWS1_IDX4) / GWS1_STRIDE4), ROUND_UP(GWS1_DIM4, GWS1_BLOCK4)) / GWS1_VEC_SIZE4 * GWS1_VEC_SIZE4 * GWS1_BLOCK4
#define GWS1_GET_ID5() GWS1_OP5((get_global_id(GWS1_IDX5) / GWS1_STRIDE5), ROUND_UP(GWS1_DIM5, GWS1_BLOCK5)) / GWS1_VEC_SIZE5 * GWS1_VEC_SIZE5 * GWS1_BLOCK5

#define GWS1_GET_BLOCK0() GWS1_BLOCK0
#define GWS1_GET_BLOCK1() GWS1_BLOCK1
#define GWS1_GET_BLOCK2() GWS1_BLOCK2
#define GWS1_GET_BLOCK3() GWS1_BLOCK3
#define GWS1_GET_BLOCK4() GWS1_BLOCK4
#define GWS1_GET_BLOCK5() GWS1_BLOCK5

#define GWS2_GET_ID0() GWS2_OP0((get_global_id(GWS2_IDX0) / GWS2_STRIDE0), ROUND_UP(GWS2_DIM0, GWS2_BLOCK0)) / GWS2_VEC_SIZE0 * GWS2_VEC_SIZE0 * GWS2_BLOCK0
#define GWS2_GET_ID1() GWS2_OP1((get_global_id(GWS2_IDX1) / GWS2_STRIDE1), ROUND_UP(GWS2_DIM1, GWS2_BLOCK1)) / GWS2_VEC_SIZE1 * GWS2_VEC_SIZE1 * GWS2_BLOCK1
#define GWS2_GET_ID2() GWS2_OP2((get_global_id(GWS2_IDX2) / GWS2_STRIDE2), ROUND_UP(GWS2_DIM2, GWS2_BLOCK2)) / GWS2_VEC_SIZE2 * GWS2_VEC_SIZE2 * GWS2_BLOCK2
#define GWS2_GET_ID3() GWS2_OP3((get_global_id(GWS2_IDX3) / GWS2_STRIDE3), ROUND_UP(GWS2_DIM3, GWS2_BLOCK3)) / GWS2_VEC_SIZE3 * GWS2_VEC_SIZE3 * GWS2_BLOCK3
#define GWS2_GET_ID4() GWS2_OP4((get_global_id(GWS2_IDX4) / GWS2_STRIDE4), ROUND_UP(GWS2_DIM4, GWS2_BLOCK4)) / GWS2_VEC_SIZE4 * GWS2_VEC_SIZE4 * GWS2_BLOCK4
#define GWS2_GET_ID5() GWS2_OP5((get_global_id(GWS2_IDX5) / GWS2_STRIDE5), ROUND_UP(GWS2_DIM5, GWS2_BLOCK5)) / GWS2_VEC_SIZE5 * GWS2_VEC_SIZE5 * GWS2_BLOCK5

#define GWS2_GET_BLOCK0() GWS2_BLOCK0
#define GWS2_GET_BLOCK1() GWS2_BLOCK1
#define GWS2_GET_BLOCK2() GWS2_BLOCK2
#define GWS2_GET_BLOCK3() GWS2_BLOCK3
#define GWS2_GET_BLOCK4() GWS2_BLOCK4
#define GWS2_GET_BLOCK5() GWS2_BLOCK5

// clang-format on

// With work-group qualifier, without sub-group qualifier.
#define KERNEL_ATTR_SG0 \
    __attribute__((reqd_work_group_size( \
            GWS_LWS0_DEFAULT, GWS_LWS1_DEFAULT, GWS_LWS2_DEFAULT)))

// With work-group and sub-group qualifiers.
#define KERNEL_ATTR_SG1 \
    KERNEL_ATTR_SG0 \
    __attribute__((intel_reqd_sub_group_size(GWS_SGS_DEFAULT)))

#define KERNEL_ATTR CONCAT2(KERNEL_ATTR_SG, GWS_WITH_SG_DEFAULT)

// Named kernel attributes - when source contains multiple kernels.
#define NAMED_KERNEL_ATTR_SG0(name) \
    __attribute__((reqd_work_group_size(CONCAT2(GWS_LWS0_, name), \
            CONCAT2(GWS_LWS1_, name), CONCAT2(GWS_LWS2_, name))))

#define NAMED_KERNEL_ATTR_SG1(name) \
    NAMED_KERNEL_ATTR_SG0(name) \
    __attribute__((intel_reqd_sub_group_size(CONCAT2(GWS_SGS_, name))))

#define NAMED_KERNEL_ATTR(name) \
    CONCAT2(NAMED_KERNEL_ATTR_SG, CONCAT2(GWS_WITH_SG_, name))(name)

// Macro to emulate behavior of non-uniform work-groups. It is expected to be
// called at the beginning of the kernel.
// NOTE: The kernel cannot use synchronization within work-group (barrier,
// etc).
#define MAYBE_SKIP_NON_UNIFORM_WG() \
    do { \
        if ((GWS_0 != GWS_ORIG_0) && (GWS_ORIG_0 % LWS_0 != 0) \
                && (get_global_id(0) >= GWS_ORIG_0)) \
            return; \
        if ((GWS_1 != GWS_ORIG_1) && (GWS_ORIG_1 % LWS_1 != 0) \
                && (get_global_id(1) >= GWS_ORIG_1)) \
            return; \
        if ((GWS_2 != GWS_ORIG_2) && (GWS_ORIG_2 % LWS_2 != 0) \
                && (get_global_id(2) >= GWS_ORIG_2)) \
            return; \
    } while (0)

#endif

#if SRC_DT_U8 == 1
#define SRC_DT_ALIAS UCHAR
#elif SRC_DT_S8 == 1
#define SRC_DT_ALIAS CHAR
#elif SRC_DT_F16 == 1
#define SRC_DT_ALIAS HALF
#elif SRC_DT_BF16 == 1
#define SRC_DT_ALIAS BFLOAT
#elif SRC_DT_F32 == 1
#define SRC_DT_ALIAS FLOAT
#endif

#if DST_DT_U8 == 1
#define DST_DT_ALIAS UCHAR
#elif DST_DT_S8 == 1
#define DST_DT_ALIAS CHAR
#elif DST_DT_F16 == 1
#define DST_DT_ALIAS HALF
#elif DST_DT_BF16 == 1
#define DST_DT_ALIAS BFLOAT
#elif DST_DT_F32 == 1
#define DST_DT_ALIAS FLOAT
#endif

#define ALIAS(prefix) CONCAT2(prefix, _DT_ALIAS)

// BLOCK types
#define BLOCK1_T uchar
#define BLOCK2_T ushort
#define BLOCK4_T uint
#define BLOCK8_T ulong
#define BLOCK_T(alias) CONCAT3(BLOCK, SIZEOF(alias), _T)

#define BLOCK1_ALIAS UCHAR
#define BLOCK2_ALIAS USHORT
#define BLOCK4_ALIAS UINT
#define BLOCK_ALIAS(prefix) CONCAT3(BLOCK, SIZEOF(ALIAS(prefix)), _ALIAS)

#define SIZEOF_UCHAR 1
#define SIZEOF_CHAR 1
#define SIZEOF_BFLOAT 2
#define SIZEOF_HALF 2
#define SIZEOF_FLOAT 4
#define SIZEOF(alias) CONCAT2(SIZEOF_, alias)

#define READ_UCHAR8 intel_sub_group_block_read_uc8
#define READ_USHORT8 intel_sub_group_block_read_us8
#define READ_UINT8 intel_sub_group_block_read8
#define READ_BLOCK8(prefix, ptr) READ_BLOCK_N(prefix, 8)(ptr)
#define READ_BLOCK_N(prefix, n) CONCAT3(READ_, BLOCK_ALIAS(prefix), n)

#define WRITE_UCHAR8 intel_sub_group_block_write_uc8
#define WRITE_USHORT8 intel_sub_group_block_write_us8
#define WRITE_UINT8 intel_sub_group_block_write8
#define WRITE_BLOCK8(prefix, ptr, val) WRITE_BLOCK_N(prefix, 8)(ptr, val)
#define WRITE_BLOCK_N(prefix, n) CONCAT3(WRITE_, BLOCK_ALIAS(prefix), n)

#define AS_UCHAR8 as_uchar8
#define AS_CHAR8 as_char8
#define AS_HALF8 as_half8
#define AS_USHORT8 as_ushort8
#define AS_BFLOAT8 as_ushort8
#define AS_FLOAT8 as_float8
#define AS_INT8 as_int8
#define AS_UINT8 as_uint8

#define BLOCK_TO_DATA8(prefix, val) BLOCK_TO_DATA_N(prefix, 8)(val)
#define BLOCK_TO_DATA_N(prefix, n) CONCAT3(AS_, ALIAS(prefix), n)

#define DATA_TO_BLOCK8(prefix, val) DATA_TO_BLOCK_N(prefix, 8)(val)
#define DATA_TO_BLOCK_N(prefix, n) CONCAT3(AS_, BLOCK_ALIAS(prefix), n)

#define UCHAR_TO_FLOAT1 convert_float
#define UCHAR_TO_FLOAT8 convert_float8
#define FLOAT_TO_UCHAR1 convert_uchar_sat_rte
#define FLOAT_TO_UCHAR8 convert_uchar8_sat_rte

#define CHAR_TO_FLOAT1 convert_float
#define CHAR_TO_FLOAT8 convert_float8
#define FLOAT_TO_CHAR1 convert_char_sat_rte
#define FLOAT_TO_CHAR8 convert_char8_sat_rte

#define HALF_TO_FLOAT1 convert_float
#define HALF_TO_FLOAT8 convert_float8
#define FLOAT_TO_HALF1 convert_half
#define FLOAT_TO_HALF8 convert_half8

#define BFLOAT_TO_FLOAT1 cvt_bf16_to_f32
#define BFLOAT_TO_FLOAT8 cvt_bf16_to_f32
#define FLOAT_TO_BFLOAT1 cvt_f32_to_bf16
#define FLOAT_TO_BFLOAT8 cvt_f32_to_bf16

#define FLOAT_TO_FLOAT1 convert_float
#define FLOAT_TO_FLOAT8 convert_float8

#define DATA_TO_FLOAT(prefix, val) DATA_TO_FLOAT_N(prefix, 1)(val)
#define DATA_TO_FLOAT8(prefix, val) DATA_TO_FLOAT_N(prefix, 8)(val)
#define DATA_TO_FLOAT_N(prefix, n) CONCAT3(ALIAS(prefix), _TO_FLOAT, n)

#define FLOAT_TO_DATA(prefix, val) FLOAT_TO_DATA_N(prefix, 1)(val)
#define FLOAT_TO_DATA8(prefix, val) FLOAT_TO_DATA_N(prefix, 8)(val)
#define FLOAT_TO_DATA_N(prefix, n) CONCAT3(FLOAT_TO_, ALIAS(prefix), n)

#if SRC_DT_F16 || DST_DT_F16
#pragma OPENCL EXTENSION cl_khr_fp16 : enable
#endif

#if SRC_DT_F64 || DST_DT_F64
#pragma OPENCL EXTENSION cl_khr_fp64 : enable
#endif

#undef SRC_OFF
#undef DST_OFF

#define SRC_OFF(x0, x1, x2, x3, x4, x5) \
    OFF_MD(SRC, (x0), (x1), (x2), (x3), (x4), (x5))
#define DST_OFF(x0, x1, x2, x3, x4, x5) \
    OFF_MD(DST, (x0), (x1), (x2), (x3), (x4), (x5))

#define SRC_OFF_G(gr, x0, x1, x2, x3, x4) \
    OFF_MD(SRC, gr, (x0), (x1), (x2), (x3), (x4))
#define DST_OFF_G(gr, x0, x1, x2, x3, x4) \
    OFF_MD(DST, gr, (x0), (x1), (x2), (x3), (x4))

#if SRC_DT_S8
#define SRC_BLOCK_READ(src) \
    as_char(intel_sub_group_block_read_uc((const __global uchar *)(src)))
#define SRC_BLOCK_READ8(src) \
    as_char8(intel_sub_group_block_read_uc8((const __global uchar *)(src)))
#define SRC_BLOCK_WRITE(dst, val) \
    intel_sub_group_block_write_uc((__global uchar *)(dst), as_uchar(val))
#define SRC_BLOCK_WRITE8(dst, val) \
    intel_sub_group_block_write_uc8((__global uchar *)(dst), as_uchar8(val))
#endif // SRC_DT_S8

#if SRC_DT_U8
#define SRC_BLOCK_READ(src) \
    as_uchar(intel_sub_group_block_read_uc((const __global uchar *)(src)))
#define SRC_BLOCK_READ8(src) \
    as_uchar8(intel_sub_group_block_read_uc8((const __global uchar *)(src)))
#define SRC_BLOCK_WRITE(dst, val) \
    intel_sub_group_block_write_uc((__global uchar *)(dst), as_uchar(val))
#define SRC_BLOCK_WRITE8(dst, val) \
    intel_sub_group_block_write_uc8((__global uchar *)(dst), as_uchar8(val))
#endif // SRC_DT_U8

#if SRC_DT_F16
#define SRC_BLOCK_READ(src) \
    as_half(intel_sub_group_block_read_us((const __global ushort *)(src)))
#define SRC_BLOCK_READ8(src) \
    as_half8(intel_sub_group_block_read_us8((const __global ushort *)(src)))
#define SRC_BLOCK_WRITE(dst, val) \
    intel_sub_group_block_write_us((__global ushort *)(dst), as_ushort(val))
#define SRC_BLOCK_WRITE8(dst, val) \
    intel_sub_group_block_write_us8((__global ushort *)(dst), as_ushort8(val))
#endif // SRC_DT_F16

#if SRC_DT_S32
#define SRC_BLOCK_READ(src) \
    as_int(intel_sub_group_block_read((const __global uint *)(src)))
#define SRC_BLOCK_READ8(src) \
    as_int8(intel_sub_group_block_read8((const __global uint *)(src)))
#define SRC_BLOCK_WRITE(dst, val) \
    intel_sub_group_block_write((__global uint *)(dst), as_uint(val))
#define SRC_BLOCK_WRITE8(dst, val) \
    intel_sub_group_block_write8((__global uint *)(dst), as_uint8(val))
#endif // SRC_DT_S32

#if SRC_DT_F32
#define SRC_BLOCK_READ(src) \
    as_float(intel_sub_group_block_read((const __global uint *)(src)))
#define SRC_BLOCK_READ8(src) \
    as_float8(intel_sub_group_block_read8((const __global uint *)(src)))
#define SRC_BLOCK_WRITE(dst, val) \
    intel_sub_group_block_write((__global uint *)(dst), as_uint(val))
#define SRC_BLOCK_WRITE8(dst, val) \
    intel_sub_group_block_write8((__global uint *)(dst), as_uint8(val))
#endif // SRC_DT_F32

#if SRC_DT_F64
#define SRC_BLOCK_READ(src) \
    as_double(intel_sub_group_block_read2((const __global uint *)(src)))
#define SRC_BLOCK_READ8(src) \
    (double8)((as_double4(intel_sub_group_block_read8( \
                      (const __global uint *)(src)))), \
            (as_double4(intel_sub_group_block_read8( \
                    (const __global uint *)(src + 8)))))
#define SRC_BLOCK_WRITE(dst, val) \
    intel_sub_group_block_write2((__global uint *)(dst), as_uint2(val))
#define SRC_BLOCK_WRITE8(dst, val) \
    do { \
        intel_sub_group_block_write8( \
                (__global uint *)(dst), as_uint8(val.lo)); \
        intel_sub_group_block_write8( \
                (__global uint *)(dst + 8), as_uint8(val.hi)); \
    } while (0)
#endif // SRC_DT_F64

#if SRC_DT_BF16
#define SRC_BLOCK_READ(src) \
    as_ushort(intel_sub_group_block_read_us((const __global ushort *)(src)))
#define SRC_BLOCK_READ8(src) \
    as_ushort8(intel_sub_group_block_read_us8((const __global ushort *)(src)))
#define SRC_BLOCK_WRITE(dst, val) \
    intel_sub_group_block_write_us((__global ushort *)(dst), as_ushort(val))
#define SRC_BLOCK_WRITE8(dst, val) \
    intel_sub_group_block_write_us8((__global ushort *)(dst), as_ushort8(val))
#endif // SRC_DT_F16

#if DST_DT_S8
#define DST_BLOCK_READ(src) \
    as_char(intel_sub_group_block_read_uc((const __global uchar *)(src)))
#define DST_BLOCK_READ8(src) \
    as_char8(intel_sub_group_block_read_uc8((const __global uchar *)(src)))
#define DST_BLOCK_WRITE(dst, val) \
    intel_sub_group_block_write_uc((__global uchar *)(dst), as_uchar(val))
#define DST_BLOCK_WRITE8(dst, val) \
    intel_sub_group_block_write_uc8((__global uchar *)(dst), as_uchar8(val))
#endif // DST_DT_S8

#if DST_DT_U8
#define DST_BLOCK_READ(src) \
    as_uchar(intel_sub_group_block_read_uc((const __global uchar *)(src)))
#define DST_BLOCK_READ8(src) \
    as_uchar8(intel_sub_group_block_read_uc8((const __global uchar *)(src)))
#define DST_BLOCK_WRITE(dst, val) \
    intel_sub_group_block_write_uc((__global uchar *)(dst), as_uchar(val))
#define DST_BLOCK_WRITE8(dst, val) \
    intel_sub_group_block_write_uc8((__global uchar *)(dst), as_uchar8(val))
#endif // SRC_DT_U8

#if DST_DT_F16
#define DST_BLOCK_READ(src) \
    as_half(intel_sub_group_block_read_us((const __global ushort *)(src)))
#define DST_BLOCK_READ8(src) \
    as_half8(intel_sub_group_block_read_us8((const __global ushort *)(src)))
#define DST_BLOCK_WRITE(dst, val) \
    intel_sub_group_block_write_us((__global ushort *)(dst), as_ushort(val))
#define DST_BLOCK_WRITE8(dst, val) \
    intel_sub_group_block_write_us8((__global ushort *)(dst), as_ushort8(val))
#endif // DST_DT_F16

#if DST_DT_S32
#define DST_BLOCK_READ(src) \
    as_int(intel_sub_group_block_read((const __global uint *)(src)))
#define DST_BLOCK_READ8(src) \
    as_int8(intel_sub_group_block_read8((const __global uint *)(src)))
#define DST_BLOCK_WRITE(dst, val) \
    intel_sub_group_block_write((__global uint *)(dst), as_uint(val))
#define DST_BLOCK_WRITE8(dst, val) \
    intel_sub_group_block_write8((__global uint *)(dst), as_uint8(val))
#endif // DST_DT_S32

#if DST_DT_F32
#define DST_BLOCK_READ(src) \
    as_float(intel_sub_group_block_read((const __global uint *)(src)))
#define DST_BLOCK_READ8(src) \
    as_float8(intel_sub_group_block_read8((const __global uint *)(src)))
#define DST_BLOCK_WRITE(dst, val) \
    intel_sub_group_block_write((__global uint *)(dst), as_uint(val))
#define DST_BLOCK_WRITE8(dst, val) \
    intel_sub_group_block_write8((__global uint *)(dst), as_uint8(val))
#endif // DST_DT_F32

#if DST_DT_F64
#define DST_BLOCK_READ(src) \
    as_double(intel_sub_group_block_read2((const __global uint *)(src)))
#define DST_BLOCK_READ8(src) \
    (double8)((as_double4(intel_sub_group_block_read8( \
                      (const __global uint *)(src)))), \
            (as_double4(intel_sub_group_block_read8( \
                    (const __global uint *)(src)))))
#define DST_BLOCK_WRITE(dst, val) \
    intel_sub_group_block_write2((__global uint *)(dst), as_uint2(val))
#define DST_BLOCK_WRITE8(dst, val) \
    do { \
        intel_sub_group_block_write8( \
                (__global uint *)(dst), as_uint8(val.lo)); \
        intel_sub_group_block_write8( \
                (__global uint *)(dst + 8), as_uint8(val.hi)); \
    } while (0)
#endif // DST_DT_F64

#if DST_DT_BF16
#define DST_BLOCK_READ(src) \
    as_ushort(intel_sub_group_block_read_us((const __global ushort *)(src)))
#define DST_BLOCK_READ8(src) \
    as_ushort8(intel_sub_group_block_read_us8((const __global ushort *)(src)))
#define DST_BLOCK_WRITE(dst, val) \
    intel_sub_group_block_write_us((__global ushort *)(dst), as_ushort(val))
#define DST_BLOCK_WRITE8(dst, val) \
    intel_sub_group_block_write_us8((__global ushort *)(dst), as_ushort8(val))
#endif // SRC_DT_F16

#if SRC_DT_BF16 && DST_DT_BF16
#define SRC_TO_DST(x) (x)
#define SRC_TO_DST8(x) (x)
#else
#define SRC_TO_DST(x) TO_DST(SRC_TO_REF(x))
#define SRC_TO_DST8(x) TO_DST8(SRC_TO_REF8(x))
#endif

#if WITH_SUM_A
#define REORDER(_dst, _src, _a, _b) \
    do { \
        const float _x = SRC_TO_REF(_src); \
        const float _s = _a * _x; \
        _dst = TO_DST(_s); \
    } while (0)
#define REORDER8(_dst, _src, _a, _b) \
    do { \
        const float8 _x = convert_float8(SRC_TO_REF8(_src)); \
        const float8 _s = _a * _x; \
        _dst = TO_DST8(_s); \
    } while (0)

#elif WITH_SUM_AB
#define REORDER(_dst, _src, _a, _b) \
    do { \
        const float _x = SRC_TO_REF(_src); \
        const float _y = DST_TO_REF(_dst); \
        const float _s = _a * _x + _b * _y; \
        _dst = TO_DST(_s); \
    } while (0)
#define REORDER8(_dst, _src, _a, _b) \
    do { \
        const float8 _x = convert_float8(SRC_TO_REF8(_src)); \
        const float8 _y = convert_float8(DST_TO_REF8(_dst)); \
        const float8 _s = _a * _x + _b * _y; \
        _dst = TO_DST8(_s); \
    } while (0)

#elif SCALE_QUANT
#define REORDER(_out, _src, _a, _b) \
    do { \
        const float _x = SRC_TO_REF(_src); \
        const float _s = _a * _x + _b; \
        _out = TO_DST(_s); \
    } while (0)
#define REORDER8(_out, _src, _a, _b) \
    do { \
        const float8 _x = convert_float8(SRC_TO_REF8(_src)); \
        const float8 _s = _a * _x + _b; \
        _out = TO_DST8(_s); \
    } while (0)

#else // WITH_SUM_AB == 0
#define REORDER(_dst, _src, _a, _b) \
    do { \
        _dst = SRC_TO_DST(_src); \
    } while (0)
#define REORDER8(_dst, _src, _a, _b) \
    do { \
        _dst = SRC_TO_DST8(_src); \
    } while (0)

#endif // WITH_SUM_AB

#if SCALE_QUANT

#define MASK_D(_d) ((SCALE_MASK >> _d) & 1)

#define SCALE_D0 (MASK_D(0) ? SRC_D0 : 1)
#define SCALE_D1 (MASK_D(1) ? SRC_D1 : 1)
#define SCALE_D2 (MASK_D(2) ? SRC_D2 : 1)
#define SCALE_D3 (MASK_D(3) ? SRC_D3 : 1)
#define SCALE_D4 (MASK_D(4) ? SRC_D4 : 1)
#define SCALE_D5 (MASK_D(5) ? SRC_D5 : 1)

#define SCALE_S0 (SCALE_D1 * SCALE_D2 * SCALE_D3 * SCALE_D4 * SCALE_D5)
#define SCALE_S1 (SCALE_D2 * SCALE_D3 * SCALE_D4 * SCALE_D5)
#define SCALE_S2 (SCALE_D3 * SCALE_D4 * SCALE_D5)
#define SCALE_S3 (SCALE_D4 * SCALE_D5)
#define SCALE_S4 (SCALE_D5)
#define SCALE_S5 (1)
#define NSCALES (SCALE_S0 * SCALE_D0)

#define SCALE_OFF(x0, x1, x2, x3, x4, x5) \
    ((x0)*SCALE_S0 * MASK_D(0) + (x1)*SCALE_S1 * MASK_D(1) \
            + (x2)*SCALE_S2 * MASK_D(2) + (x3)*SCALE_S3 * MASK_D(3) \
            + (x4)*SCALE_S4 * MASK_D(4) + (x5)*SCALE_S5 * MASK_D(5))

#endif // SCALE_QUANT

KERNEL_ATTR
__kernel void generic_reorder(__global float *restrict src,
        __global float *restrict dst, float alpha, float beta,
        __global float *restrict scales) {

    src += SRC_OFFSET0;
    dst += DST_OFFSET0;

#define LOOP_NEST_LEVEL 4
    const uint sgId = get_sub_group_local_id();
    uint d[6]; // tensor coordinates from workitem ID
    uint b[6] = {0, 0, 0, 0, 0, 0}; // ajustment to coordinates per block (loop)

    d[0] = 0;//GWS_GET_D0();
    d[1] = 0;//GWS_GET_D1();
    d[2] = 0;//GWS_GET_D2();
    d[3] = 0;//GWS_GET_D3();
    d[4] = 0;//GWS_GET_D4();
    d[5] = 0;//GWS_GET_D5();

    // Dispatcher code does not allow vectorization of dimensions that are not
    // divisible by 16. As workaround, we cheat dispatcher by pretending
    // vectorized dim is 16x larger than it really is. Here we adjust its size
    // back to original.
    d[VECT_DIM] /= RESCALE_COEFF;
    // sg_off = offset into 'cache' local mem for given subgroup.
    // Local memory will be split by subgroups so that given address in local
    // can only be accessed by single subgroup. This lets us avoid barriers.
    const uint cache_size_per_sg = D_BLK_SIZE_0 * D_BLK_SIZE_1 * D_BLK_SIZE_2
            * D_BLK_SIZE_3 * VECT_SIZE;
    const uint sg_off = get_sub_group_id() * cache_size_per_sg;

    // TODO: decide whether to store cache as SRC_DATA_T or DST_DATA_T
    __local float cache[SG_PER_WG * cache_size_per_sg];
    uint iter[LOOP_NEST_LEVEL] = {0, 0, 0, 0};

// Loop across dimensions described in src_block.
// Example: block 2a4c2b would mean:
// for(a = 0..1) { for (c = 0..3) { for (b = 0..2) {}}}
#if S_BLK_SIZE_3 > 1
    for_(iter[3] = 0; iter[3] < S_BLK_SIZE_3; iter[3]++)
#endif
#if S_BLK_SIZE_2 > 1
    for_(iter[2] = 0; iter[2] < S_BLK_SIZE_2; iter[2]++)
#endif
#if S_BLK_SIZE_1 > 1
    for_(iter[1] = 0; iter[1] < S_BLK_SIZE_1; iter[1]++)
#endif
#if S_BLK_SIZE_0 > 1
    for_(iter[0] = 0; iter[0] < S_BLK_SIZE_0; iter[0]++)
#endif
    {
        // the same IDX could be in more than one loop, this makes offset calculation tricky
        b[0] = 0;
        b[1] = 0;
        b[2] = 0;
        b[3] = 0;
        b[4] = 0;
        b[5] = 0;
        b[S_BLK_IDX_0] += iter[0] * S_BLK_STEP_0;
        b[S_BLK_IDX_1] += iter[1] * S_BLK_STEP_1;
        b[S_BLK_IDX_2] += iter[2] * S_BLK_STEP_2;
        b[S_BLK_IDX_3] += iter[3] * S_BLK_STEP_3;

        const uint src_off = SRC_OFF(d[0] + b[0], d[1] + b[1], d[2] + b[2],
                d[3] + b[3], d[4] + b[4], d[5] + b[5]);

#if S_MOD_3 > 1
        b[S_IDX_3] += S_MUL_3 * ((sgId / S_DIV_3) % S_MOD_3);
#endif
#if S_MOD_2 > 1
        b[S_IDX_2] += S_MUL_2 * ((sgId / S_DIV_2) % S_MOD_2);
#endif
#if S_MOD_1 > 1
        b[S_IDX_1] += S_MUL_1 * ((sgId / S_DIV_1) % S_MOD_1);
#endif
#if S_MOD_0 > 1
        b[S_IDX_0] += S_MUL_0 * ((sgId / S_DIV_0) % S_MOD_0);
#endif

        // Data in cache (local mem) is organized as if it had 'fedcba' format
        // tag. This is neither src's nor dst's format so some performance is
        // wasted here, but otherwise the logic to calculate offsets would be
        // too complicated.
        uint cache_idx = sg_off + b[5] * CACHE_STRIDE_5 + b[4] * CACHE_STRIDE_4
                + b[3] * CACHE_STRIDE_3 + b[2] * CACHE_STRIDE_2
                + b[1] * CACHE_STRIDE_1 + b[0] * CACHE_STRIDE_0;
        const int pad_d0 = d[0] + b[0] >= SRC_D0;
        const int pad_d1 = NDIMS > 1 && d[1] + b[1] >= SRC_D1;
        const int pad_d2 = NDIMS > 2 && d[2] + b[2] >= SRC_D2;
        const int pad_d3 = NDIMS > 3 && d[3] + b[3] >= SRC_D3;
        const int pad_d4 = NDIMS > 4 && d[4] + b[4] >= SRC_D4;
        const int pad_d5 = NDIMS > 5 && d[5] + b[5] >= SRC_D5;
        const bool pad_sgid = sgId >= LIMIT_SSGID;
        const int pad
                = pad_d0 || pad_d1 || pad_d2 || pad_d3 || pad_d4 || pad_d5;
        if (!pad_sgid) {
            // src_off is based on coordinates of blocks and returns same
            // result for each workitem in subgroup. This is to make sure
            // offset calculation is simple enough that compiler won't split
            // this burst into single bytes accesses. Yet each workitem will
            // read different address thanks to "+sgID" statement
            float src_tmp = pad ? 0 : src[src_off + sgId];
            cache[cache_idx] = src_tmp;
        }
    }
    for (uint i = 0; i < LOOP_NEST_LEVEL; i++) {
        iter[i] = 0;
    }
#if D_BLK_SIZE_3 > 1
    for_(iter[3] = 0; iter[3] < D_BLK_SIZE_3; iter[3]++)
#endif
#if D_BLK_SIZE_2 > 1
    for_(iter[2] = 0; iter[2] < D_BLK_SIZE_2; iter[2]++)
#endif
#if D_BLK_SIZE_1 > 1
    for_(iter[1] = 0; iter[1] < D_BLK_SIZE_1; iter[1]++)
#endif
#if D_BLK_SIZE_0 > 1
    for_(iter[0] = 0; iter[0] < D_BLK_SIZE_0; iter[0]++)
#endif
    {
        // the same IDX could be in more than one loop, this makes offset calculation tricky
        b[0] = 0;
        b[1] = 0;
        b[2] = 0;
        b[3] = 0;
        b[4] = 0;
        b[5] = 0;
        b[D_BLK_IDX_0] += iter[0] * D_BLK_STEP_0;
        b[D_BLK_IDX_1] += iter[1] * D_BLK_STEP_1;
        b[D_BLK_IDX_2] += iter[2] * D_BLK_STEP_2;
        b[D_BLK_IDX_3] += iter[3] * D_BLK_STEP_3;

#if D_MOD_3 > 1
        b[D_IDX_3] += D_MUL_3 * ((sgId / D_DIV_3) % D_MOD_3);
#endif
#if D_MOD_2 > 1
        b[D_IDX_2] += D_MUL_2 * ((sgId / D_DIV_2) % D_MOD_2);
#endif
#if D_MOD_1 > 1
        b[D_IDX_1] += D_MUL_1 * ((sgId / D_DIV_1) % D_MOD_1);
#endif
#if D_MOD_0 > 1
        b[D_IDX_0] += D_MUL_0 * ((sgId / D_DIV_0) % D_MOD_0);
#endif

        const uint dst_off = 0;//DST_OFF(d[0] + b[0], d[1] + b[1], d[2] + b[2],
                //d[3] + b[3], d[4] + b[4], d[5] + b[5]);

        float dst_tmp;
        uint cache_idx = sg_off + b[5] * CACHE_STRIDE_5 + b[4] * CACHE_STRIDE_4
                + b[3] * CACHE_STRIDE_3 + b[2] * CACHE_STRIDE_2
                + b[1] * CACHE_STRIDE_1 + b[0] * CACHE_STRIDE_0;

        const int pad_d0 = d[0] + b[0] >= DST_PD0;
        const int pad_d1 = NDIMS > 1 && d[1] + b[1] >= DST_PD1;
        const int pad_d2 = NDIMS > 2 && d[2] + b[2] >= DST_PD2;
        const int pad_d3 = NDIMS > 3 && d[3] + b[3] >= DST_PD3;
        const int pad_d4 = NDIMS > 4 && d[4] + b[4] >= DST_PD4;
        const int pad_d5 = NDIMS > 5 && d[5] + b[5] >= DST_PD5;
        const bool pad_sgid = sgId >= LIMIT_DSGID;
        const int pad = pad_d0 || pad_d1 || pad_d2 || pad_d3 || pad_d4 || pad_d5
                || pad_sgid;

        if (!pad) {
            float from_cache = cache[cache_idx];
#if WITH_SUM_AB
            // TODO: move to separate loop to enable burst reads from dst?
            dst_tmp = dst[dst_off];
#endif
#if SCALE_QUANT
            // TODO: move to separate loop to enable burst reads from scales?
            uint scale_idx = SCALE_OFF(d[0] + b[0], d[1] + b[1], d[2] + b[2],
                    d[3] + b[3], d[4] + b[4], d[5] + b[5]);
            alpha = scale_idx < NSCALES ? scales[scale_idx] : 0.0;
#endif

            REORDER(dst_tmp, from_cache, alpha, beta);
            dst[dst_off] = dst_tmp;
        }
    }
}

KERNEL_ATTR
__kernel void generic_reorder_dirty(__global float *restrict src,
        __global float *restrict dst, float alpha, float beta,
        __global float *restrict scales) {
    src += SRC_OFFSET0;
    dst += DST_OFFSET0;

#define LOOP_NEST_LEVEL 4

    const uint sgId = get_sub_group_local_id();
    uint d[6]; // tensor coordinates from workitem ID
    uint b[6] = {0, 0, 0, 0, 0, 0}; // ajustment to coordinates per block (loop)

    d[0] = 0;//GWS_GET_D0();
    d[1] = 0;//GWS_GET_D1();
    d[2] = 0;//GWS_GET_D2();
    d[3] = 0;//GWS_GET_D3();
    d[4] = 0;//GWS_GET_D4();
    d[5] = 0;//GWS_GET_D5();

    // Dispatcher code does not allow vectorization of dimensions that are not
    // divisible by 16. As workaround, we cheat dispatcher by pretending
    // vectorized dim is 16x larger than it really is. Here we adjust its size
    // back to original.
    d[VECT_DIM] /= RESCALE_COEFF;
    // sg_off = offset into 'cache' local mem for given subgroup.
    // Local memory will be split by subgroups so that given address in local
    // can only be accessed by single subgroup. This lets us avoid barriers.
    const uint cache_size_per_sg = D_BLK_SIZE_0 * D_BLK_SIZE_1 * D_BLK_SIZE_2
            * D_BLK_SIZE_3 * VECT_SIZE;
    const uint sg_off = get_sub_group_id() * cache_size_per_sg;

    // TODO: decide whether to store cache as SRC_DATA_T or DST_DATA_T
    __local float cache[SG_PER_WG * cache_size_per_sg];
    uint iter[LOOP_NEST_LEVEL] = {0, 0, 0, 0};

// Loop across dimensions described in src_block.
// Example: block 2a4c2b would mean:
// for(a = 0..1) { for (c = 0..3) { for (b = 0..2) {}}}
#if S_BLK_SIZE_3 > 1
    for_(iter[3] = 0; iter[3] < S_BLK_SIZE_3; iter[3]++)
#endif
#if S_BLK_SIZE_2 > 1
    for_(iter[2] = 0; iter[2] < S_BLK_SIZE_2; iter[2]++)
#endif
#if S_BLK_SIZE_1 > 1
    for_(iter[1] = 0; iter[1] < S_BLK_SIZE_1; iter[1]++)
#endif
#if S_BLK_SIZE_0 > 1
    for_(iter[0] = 0; iter[0] < S_BLK_SIZE_0; iter[0]++)
#endif
    {
        // the same IDX could be in more than one loop, this makes offset calculation tricky
        b[0] = 0;
        b[1] = 0;
        b[2] = 0;
        b[3] = 0;
        b[4] = 0;
        b[5] = 0;
        b[S_BLK_IDX_0] += iter[0] * S_BLK_STEP_0;
        b[S_BLK_IDX_1] += iter[1] * S_BLK_STEP_1;
        b[S_BLK_IDX_2] += iter[2] * S_BLK_STEP_2;
        b[S_BLK_IDX_3] += iter[3] * S_BLK_STEP_3;

        const uint src_off = SRC_OFF(d[0] + b[0], d[1] + b[1], d[2] + b[2],
                d[3] + b[3], d[4] + b[4], d[5] + b[5]);

#if S_MOD_3 > 1
        b[S_IDX_3] += S_MUL_3 * ((sgId / S_DIV_3) % S_MOD_3);
#endif
#if S_MOD_2 > 1
        b[S_IDX_2] += S_MUL_2 * ((sgId / S_DIV_2) % S_MOD_2);
#endif
#if S_MOD_1 > 1
        b[S_IDX_1] += S_MUL_1 * ((sgId / S_DIV_1) % S_MOD_1);
#endif
#if S_MOD_0 > 1
        b[S_IDX_0] += S_MUL_0 * ((sgId / S_DIV_0) % S_MOD_0);
#endif

        // Data in cache (local mem) is organized as if it had 'fedcba' format
        // tag. This is neither src's nor dst's format so some performance is
        // wasted here, but otherwise the logic to calculate offsets would be
        // too complicated.
        uint cache_idx = sg_off + b[5] * CACHE_STRIDE_5 + b[4] * CACHE_STRIDE_4
                + b[3] * CACHE_STRIDE_3 + b[2] * CACHE_STRIDE_2
                + b[1] * CACHE_STRIDE_1 + b[0] * CACHE_STRIDE_0;
        const int pad_d0 = d[0] + b[0] >= SRC_D0;
        const int pad_d1 = NDIMS > 1 && d[1] + b[1] >= SRC_D1;
        const int pad_d2 = NDIMS > 2 && d[2] + b[2] >= SRC_D2;
        const int pad_d3 = NDIMS > 3 && d[3] + b[3] >= SRC_D3;
        const int pad_d4 = NDIMS > 4 && d[4] + b[4] >= SRC_D4;
        const int pad_d5 = NDIMS > 5 && d[5] + b[5] >= SRC_D5;
        const bool pad_sgid = sgId >= LIMIT_SSGID;
        const int pad
                = pad_d0 || pad_d1 || pad_d2 || pad_d3 || pad_d4 || pad_d5;
        if (!pad_sgid) {
            // src_off is based on coordinates of blocks and returns same
            // result for each workitem in subgroup. This is to make sure
            // offset calculation is simple enough that compiler won't split
            // this burst into single bytes accesses. Yet each workitem will
            // read different address thanks to "+sgID" statement
            float src_tmp = pad ? 0 : src[src_off + sgId];
            cache[cache_idx] = src_tmp;
        }
    }
    for (uint i = 0; i < LOOP_NEST_LEVEL; i++) {
        iter[i] = 0;
    }
#if D_BLK_SIZE_3 > 1
    for_(iter[3] = 0; iter[3] < D_BLK_SIZE_3; iter[3]++)
#endif
#if D_BLK_SIZE_2 > 1
    for_(iter[2] = 0; iter[2] < D_BLK_SIZE_2; iter[2]++)
#endif
#if D_BLK_SIZE_1 > 1
    for_(iter[1] = 0; iter[1] < D_BLK_SIZE_1; iter[1]++)
#endif
#if D_BLK_SIZE_0 > 1
    for_(iter[0] = 0; iter[0] < D_BLK_SIZE_0; iter[0]++)
#endif
    float* dst2 = dst;
    {
        // the same IDX could be in more than one loop, this makes offset calculation tricky
        b[0] = 0;
        b[1] = 0;
        b[2] = 0;
        b[3] = 0;
        b[4] = 0;
        b[5] = 0;
        b[D_BLK_IDX_0] += iter[0] * D_BLK_STEP_0;
        b[D_BLK_IDX_1] += iter[1] * D_BLK_STEP_1;
        b[D_BLK_IDX_2] += iter[2] * D_BLK_STEP_2;
        b[D_BLK_IDX_3] += iter[3] * D_BLK_STEP_3;

#if D_MOD_3 > 1
        b[D_IDX_3] += D_MUL_3 * ((sgId / D_DIV_3) % D_MOD_3);
#endif
#if D_MOD_2 > 1
        b[D_IDX_2] += D_MUL_2 * ((sgId / D_DIV_2) % D_MOD_2);
#endif
#if D_MOD_1 > 1
        b[D_IDX_1] += D_MUL_1 * ((sgId / D_DIV_1) % D_MOD_1);
#endif
#if D_MOD_0 > 1
        b[D_IDX_0] += D_MUL_0 * ((sgId / D_DIV_0) % D_MOD_0);
#endif

        const uint dst_off = 0;//DST_OFF(d[0] + b[0], d[1] + b[1], d[2] + b[2],
                //d[3] + b[3], d[4] + b[4], d[5] + b[5]);

        float dst_tmp;
        uint cache_idx = sg_off + b[5] * CACHE_STRIDE_5 + b[4] * CACHE_STRIDE_4
                + b[3] * CACHE_STRIDE_3 + b[2] * CACHE_STRIDE_2
                + b[1] * CACHE_STRIDE_1 + b[0] * CACHE_STRIDE_0;

        const int pad_d0 = d[0] + b[0] >= DST_PD0;
        const int pad_d1 = NDIMS > 1 && d[1] + b[1] >= DST_PD1;
        const int pad_d2 = NDIMS > 2 && d[2] + b[2] >= DST_PD2;
        const int pad_d3 = NDIMS > 3 && d[3] + b[3] >= DST_PD3;
        const int pad_d4 = NDIMS > 4 && d[4] + b[4] >= DST_PD4;
        const int pad_d5 = NDIMS > 5 && d[5] + b[5] >= DST_PD5;
        const bool pad_sgid = sgId >= LIMIT_DSGID;
        const int pad = pad_d0 || pad_d1 || pad_d2 || pad_d3 || pad_d4 || pad_d5
                || pad_sgid;

        if (!pad) {
            float from_cache = cache[cache_idx];
//#if WITH_SUM_AB
            // TODO: move to separate loop to enable burst reads from dst?
            dst_tmp = dst[dst_off - 1];
//#endif
#if SCALE_QUANT
            // TODO: move to separate loop to enable burst reads from scales?
            uint scale_idx = SCALE_OFF(d[0] + b[0], d[1] + b[1], d[2] + b[2],
                    d[3] + b[3], d[4] + b[4], d[5] + b[5]);
            alpha = scale_idx < NSCALES ? scales[scale_idx] : 0.0;
#endif

            REORDER(dst_tmp, from_cache, alpha, beta);
            dst2[dst_off - 1] = dst_tmp;
        }
    }
}
