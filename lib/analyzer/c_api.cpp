#include <unordered_set>
#include <vector>

#include "c_api.h"

struct clsa_context {
    std::unordered_set<void*> buffers;
};

struct clsa_buffer {
    size_t size;
};

struct clsa_kernel_args {
    std::vector<std::pair<size_t, void*>> args;
};

extern "C" {
clsa_context* clsa_create_context() {
    return new clsa_context();
}

void clsa_delete_context(clsa_context* context) {
    delete context;
}
}
