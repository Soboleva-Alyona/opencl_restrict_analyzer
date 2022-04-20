#include <unordered_set>
#include <vector>

#include "c_api.h"

struct clsma_context {
    std::unordered_set<void*> buffers;
};

struct clsma_buffer {
    size_t size;
};

struct clsma_kernel_args {
    std::vector<std::pair<size_t, void*>> args;
};

extern "C" {
    clsma_context* clsma_create_context() {
        return new clsma_context();
    }

    void clsma_delete_context(clsma_context* context) {
        delete context;
    }
}
