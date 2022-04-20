#ifndef OPENCL_RESTRICT_ANALYZER_C_API_H
#define OPENCL_RESTRICT_ANALYZER_C_API_H

#ifdef _WIN32
#ifdef clsa_COMPILE_LIBRARY
#define clsa_CAPI_EXPORT __declspec(dllexport)
#else
#define clsa_CAPI_EXPORT __declspec(dllimport)
#endif
#else
#define clsa_CAPI_EXPORT __attribute__((visibility("default")))
#endif

#ifdef __cplusplus

#include <cstdint>

using std::size_t;
using std::uint32_t;
#else
#include "stdint.h"
#endif

#include <CL/cl.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct clsa_kernel_args clsa_kernel_args;

typedef struct clsa_kernel_info {
    const char* kernel_name;
    clsa_kernel_args* kernel_args;
} clsa_kernel_info;

clsa_CAPI_EXPORT extern void clsa_analyze(const char* filename, uint32_t checks, clsa_kernel_info kernel_info,
                                          uint32_t work_dim,
                                          const size_t* global_work_size,
                                          const size_t* local_work_size);

clsa_CAPI_EXPORT extern clsa_kernel_info* clsa_kernel_to_info(cl_kernel kernel, cl_int* errcode_ret);

clsa_CAPI_EXPORT extern void clsa_delete_kernel_info(clsa_kernel_info* kernel_info);

typedef struct clsa_buffer clsa_buffer;

typedef struct clsa_context clsa_context;

clsa_CAPI_EXPORT extern clsa_context* clsa_create_context();
clsa_CAPI_EXPORT extern void clsa_delete_context(clsa_context* context);

clsa_CAPI_EXPORT extern clsa_buffer* clsa_create_buffer(clsa_context* context, size_t size);
clsa_CAPI_EXPORT extern void clsa_delete_buffer(clsa_buffer* buffer);

clsa_CAPI_EXPORT extern clsa_kernel_args* clsa_create_kernel_args(uint32_t args_count);
clsa_CAPI_EXPORT extern void clsa_set_kernel_arg(clsa_kernel_args* kernel_args, uint32_t arg_index,
                                                 size_t arg_size, const void* arg_value);
clsa_CAPI_EXPORT extern void clsa_delete_kernel_args(clsa_kernel_args* args);

#ifdef __cplusplus
} /* end extern "C" */
#endif

#endif //OPENCL_RESTRICT_ANALYZER_C_API_H
