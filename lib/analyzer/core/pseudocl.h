#ifndef OPENCL_RESTRICT_ANALYZER_PSEUDOCL_H
#define OPENCL_RESTRICT_ANALYZER_PSEUDOCL_H

#include <cstddef>

namespace clsa {
    typedef struct _pseudocl_mem* pseudocl_mem;

    clsa::pseudocl_mem pseudocl_create_buffer(std::size_t size);

    bool pseudocl_is_valid_mem_object(clsa::pseudocl_mem memobj);

    std::size_t pseudocl_get_mem_object_size(clsa::pseudocl_mem memobj);

    void pseudocl_release_mem_object(clsa::pseudocl_mem memobj);
}

#endif //OPENCL_RESTRICT_ANALYZER_PSEUDOCL_H
