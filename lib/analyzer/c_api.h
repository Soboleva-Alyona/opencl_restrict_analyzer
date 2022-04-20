#ifndef OPENCL_RESTRICT_ANALYZER_C_API_H
#define OPENCL_RESTRICT_ANALYZER_C_API_H

#ifdef _WIN32
#ifdef CLSMA_COMPILE_LIBRARY
#define CLSMA_CAPI_EXPORT __declspec(dllexport)
#else
#define CLSMA_CAPI_EXPORT __declspec(dllimport)
#endif
#else
#define CLSMA_CAPI_EXPORT __attribute__((visibility("default")))
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

typedef struct clsma_kernel_args clsma_kernel_args;

typedef struct clsma_kernel_info {
    const char* kernel_name;
    clsma_kernel_args* kernel_args;
} clsma_kernel_info;

CLSMA_CAPI_EXPORT extern void clsma_analyze(const char* filename, uint32_t checks, clsma_kernel_info kernel_info,
                                            uint32_t work_dim,
                                            const size_t *global_work_size,
                                            const size_t *local_work_size);

CLSMA_CAPI_EXPORT extern clsma_kernel_info* clsma_kernel_to_info(cl_kernel kernel, cl_int* errcode_ret);

CLSMA_CAPI_EXPORT extern void clsma_delete_kernel_info(clsma_kernel_info* kernel_info);

typedef struct clsma_buffer clsma_buffer;

typedef struct clsma_context clsma_context;

CLSMA_CAPI_EXPORT extern clsma_context* clsma_create_context();
CLSMA_CAPI_EXPORT extern void clsma_delete_context(clsma_context* context);

CLSMA_CAPI_EXPORT extern clsma_buffer* clsma_create_buffer(clsma_context* context, size_t size);
CLSMA_CAPI_EXPORT extern void clsma_delete_buffer(clsma_buffer* buffer);

CLSMA_CAPI_EXPORT extern clsma_kernel_args* clsma_create_kernel_args(uint32_t args_count);
CLSMA_CAPI_EXPORT extern void clsma_set_kernel_arg(clsma_kernel_args* kernel_args, uint32_t arg_index,
                                                   size_t arg_size, const void* arg_value);
CLSMA_CAPI_EXPORT extern void clsma_delete_kernel_args(clsma_kernel_args* args);

#ifdef __cplusplus
} /* end extern "C" */
#endif

#endif //OPENCL_RESTRICT_ANALYZER_C_API_H
