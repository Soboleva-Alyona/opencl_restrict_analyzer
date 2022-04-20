#include "pseudocl.h"

#include <mutex>
#include <unordered_set>

namespace {
    std::unordered_set<clsma::pseudocl_mem> memobjs;
    std::mutex memobjs_mutex;
}

struct clsma::_pseudocl_mem {
    size_t size;
};

clsma::pseudocl_mem clsma::pseudocl_create_buffer(std::size_t size) {
    auto memobj = new clsma::_pseudocl_mem {
        .size = size
    };
    std::lock_guard guard(memobjs_mutex);
    memobjs.emplace(memobj);
    return memobj;
}

bool clsma::pseudocl_is_valid_mem_object(clsma::pseudocl_mem memobj) {
    std::lock_guard guard(memobjs_mutex);
    return memobjs.contains(memobj);
}

size_t clsma::pseudocl_get_mem_object_size(clsma::pseudocl_mem memobj) {
    return memobj->size;
}

void clsma::pseudocl_release_mem_object(clsma::pseudocl_mem memobj) {
    delete memobj;
    std::lock_guard guard(memobjs_mutex);
    memobjs.erase(memobj);
}
