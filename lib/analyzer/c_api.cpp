#include <clang/AST/ASTContext.h>

#include "core/pseudocl.h"
#include "frontend/analyzer.h"

#include "c_api.h"

extern "C" {

struct _clsa_mem {
    clsa::pseudocl_mem value;
};

clsa_mem clsa_create_buffer(size_t size) {
    return new _clsa_mem {clsa::pseudocl_create_buffer(size)};
}

void clsa_release_mem_object(clsa_mem memobj) {
    clsa::pseudocl_release_mem_object(memobj->value);
    delete memobj;
}

clsa_violation* clsa_analyze(std::set<uint32_t>* checks, const char* filename, const char* kernel_name, uint32_t work_dim,
                             const size_t* global_work_size, const size_t* local_work_size,
                             size_t args_count, const std::size_t* arg_sizes, void** arg_values,
                             size_t* violation_count) {
    std::vector<clsa_violation> violations;
    clsa::analyzer analyzer(filename);
    analyzer.set_violation_handler([&violations](const clang::ASTContext& ctx, const clsa::violation& violation) {
        violations.emplace_back(clsa_violation {
            .location = strdup(violation.location.printToString(ctx.getSourceManager()).c_str()),
            .message = strdup(violation.message.c_str())
        });
    });
    analyzer.analyze(checks, kernel_name, work_dim, global_work_size, local_work_size, 32, args_count,
                     arg_sizes, arg_values);
    *violation_count = violations.size();
    auto* violations_data = (clsa_violation*) malloc(sizeof(clsa_violation) * violations.size());
    memcpy(violations_data, violations.data(), sizeof(clsa_violation) * violations.size());
    return violations_data;
}

void clsa_release_violations(clsa_violation* violations, size_t violation_count) {
    for (size_t i = 0; i < violation_count; ++i) {
        clsa_violation* violation = violations + i;
        free(violation->message);
        free(violation->location);
    }
    free(violations);
}

}
