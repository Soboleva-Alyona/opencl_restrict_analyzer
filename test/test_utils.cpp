#include "test_utils.h"

#include "../lib/analyzer/frontend/analyzer.h"

pseudocl_mem_ptr create_buffer_ptr(std::size_t size) {
    return { new clsa::pseudocl_mem(clsa::pseudocl_create_buffer(size)), [](clsa::pseudocl_mem* ptr) {
        clsa::pseudocl_release_mem_object(*ptr);
        delete ptr;
    } };
}

std::vector<clsa::violation> analyze(std::string_view path, std::string_view kernel, std::uint32_t checks) {
    std::vector<clsa::violation> violations;
    clsa::analyzer analyzer(path);
    analyzer.set_violation_handler([&violations](const clang::ASTContext& ctx, const clsa::violation& violation) {
        violations.emplace_back(violation);
    });
    size_t global_work_size = 16;
    std::vector<pseudocl_mem_ptr> args;
    args.emplace_back(create_buffer_ptr(global_work_size * sizeof(int)));
    args.emplace_back(create_buffer_ptr(global_work_size * sizeof(int)));
    args.emplace_back(create_buffer_ptr(global_work_size * sizeof(int)));
    std::vector<size_t> arg_sizes = { sizeof(clsa::pseudocl_mem), sizeof(clsa::pseudocl_mem), sizeof(clsa::pseudocl_mem) };
    std::vector<void*> arg_ptrs = { args[0].get(), args[1].get(), args[2].get() };
    analyzer.analyze(checks, kernel, 1, &global_work_size, nullptr, args.size(), arg_sizes.data(), arg_ptrs.data());
    return std::move(violations);
}
