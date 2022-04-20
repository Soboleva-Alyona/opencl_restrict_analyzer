#include "pseudocl.h"

#include <mutex>
#include <unordered_set>

namespace {
    std::unordered_set<clsa::pseudocl_mem> memobjs;
    std::mutex memobjs_mutex;
}

struct clsa::_pseudocl_mem {
    size_t size;
};

clsa::pseudocl_mem clsa::pseudocl_create_buffer(std::size_t size) {
    auto memobj = new clsa::_pseudocl_mem {
        .size = size
    };
    std::lock_guard guard(memobjs_mutex);
    memobjs.emplace(memobj);
    return memobj;
}

bool clsa::pseudocl_is_valid_mem_object(clsa::pseudocl_mem memobj) {
    std::lock_guard guard(memobjs_mutex);
    return memobjs.contains(memobj);
}

size_t clsa::pseudocl_get_mem_object_size(clsa::pseudocl_mem memobj) {
    return memobj->size;
}

void clsa::pseudocl_release_mem_object(clsa::pseudocl_mem memobj) {
    delete memobj;
    std::lock_guard guard(memobjs_mutex);
    memobjs.erase(memobj);
}
