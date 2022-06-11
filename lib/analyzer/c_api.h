#ifndef OPENCL_RESTRICT_ANALYZER_C_API_H
#define OPENCL_RESTRICT_ANALYZER_C_API_H

#ifdef _WIN32
#ifdef CLSA_COMPILE_LIBRARY
#define CLSA_CAPI_EXPORT __declspec(dllexport)
#else
#define CLSA_CAPI_EXPORT __declspec(dllimport)
#endif
#else
#define CLSA_CAPI_EXPORT __attribute__((visibility("default")))
#endif

#ifdef __cplusplus

#include <cstdint>

using std::size_t;
using std::uint32_t;
#else
#include "stdint.h"
#endif

#if defined(__APPLE__) || defined(__MACOSX)
#include <OpenCL/cl.h>
#else
#include <CL/cl.h>
#endif

#ifdef __cplusplus
extern "C" {
#endif

struct _clsa_mem;
typedef _clsa_mem* clsa_mem;

CLSA_CAPI_EXPORT extern clsa_mem clsa_create_buffer(size_t size);
CLSA_CAPI_EXPORT extern void clsa_release_mem_object(clsa_mem memobj);

typedef struct {
    char* location;
    char* message;
} clsa_violation;

CLSA_CAPI_EXPORT extern clsa_violation* clsa_analyze(
    uint32_t checks, const char* filename, const char* kernel_name,
    uint32_t work_dim, const size_t* global_work_size, const size_t* local_work_size,
    size_t args_count, const std::size_t* arg_sizes, void** arg_values, size_t* violation_count);

void clsa_release_violations(clsa_violation* violation, size_t violation_count);

#ifdef __cplusplus
} /* end extern "C" */
#endif

#endif //OPENCL_RESTRICT_ANALYZER_C_API_H
