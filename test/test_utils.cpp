#include <iostream>

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
        std::cout << violation.location.printToString(ctx.getSourceManager()) << ": " << violation.message << std::endl;
        violations.emplace_back(violation);
    });
    size_t global_work_size = 16;
    std::vector<pseudocl_mem_ptr> buffers;
    buffers.emplace_back(create_buffer_ptr(global_work_size * sizeof(int)));
    buffers.emplace_back(create_buffer_ptr(global_work_size * sizeof(int)));
    buffers.emplace_back(create_buffer_ptr(global_work_size * sizeof(int)));
    std::vector<size_t> arg_sizes = { sizeof(clsa::pseudocl_mem), sizeof(clsa::pseudocl_mem), sizeof(clsa::pseudocl_mem) };
    std::vector<void*> arg_ptrs = {buffers[0].get(), buffers[1].get(), buffers[2].get() };
    analyzer.analyze(checks, kernel, 1, &global_work_size, nullptr, arg_ptrs.size(), arg_sizes.data(), arg_ptrs.data());
    return std::move(violations);
}

std::vector<clsa::violation> analyze_one_dnn(std::string_view path, std::string_view kernel, std::uint32_t checks) {
    std::vector<clsa::violation> violations;
    clsa::analyzer analyzer(path);
    analyzer.set_violation_handler([&violations](const clang::ASTContext& ctx, const clsa::violation& violation) {
        std::cout << violation.location.printToString(ctx.getSourceManager()) << ": " << violation.message << std::endl;
        violations.emplace_back(violation);
    });
    size_t global_work_size = 16;
    std::vector<pseudocl_mem_ptr> buffers;
    buffers.emplace_back(create_buffer_ptr(global_work_size * sizeof(int)));
    buffers.emplace_back(create_buffer_ptr(global_work_size * sizeof(int)));
    buffers.emplace_back(create_buffer_ptr(global_work_size * sizeof(int)));
    float alpha = 0.5;
    float beta = 0.5;
    std::vector<size_t> arg_sizes = { sizeof(clsa::pseudocl_mem), sizeof(clsa::pseudocl_mem), sizeof(alpha), sizeof(beta), sizeof(clsa::pseudocl_mem) };
    std::vector<void*> arg_ptrs = { buffers[0].get(), buffers[1].get(), &alpha, &beta, buffers[2].get() };
    analyzer.analyze(checks, kernel, 1, &global_work_size, nullptr, arg_ptrs.size(), arg_sizes.data(), arg_ptrs.data(), {
        .array_values = true
    });
    return std::move(violations);
}

std::vector<clsa::violation> analyze_pipe_cnn(std::string_view path, std::string_view kernel, std::uint32_t checks) {
    std::vector<clsa::violation> violations;
    clsa::analyzer analyzer(path);
    analyzer.set_violation_handler([&violations](const clang::ASTContext& ctx, const clsa::violation& violation) {
        std::cout << violation.location.printToString(ctx.getSourceManager()) << ": " << violation.message << std::endl;
        violations.emplace_back(violation);
    });
    std::vector<size_t> global_work_sizes = { 16, 16, 16 };
    std::vector<pseudocl_mem_ptr> buffers;
    buffers.emplace_back(create_buffer_ptr(global_work_sizes[0] * 16));
    buffers.emplace_back(create_buffer_ptr(global_work_sizes[1] * 16));
    buffers.emplace_back(create_buffer_ptr(global_work_sizes[2] * 16));
    char data_dim1 = 8;
    char data_dim2 = 8;
    char frac_dout = 8;
    std::vector<size_t> arg_sizes = { sizeof(char), sizeof(char), sizeof(char), sizeof(clsa::pseudocl_mem), sizeof(clsa::pseudocl_mem), sizeof(clsa::pseudocl_mem) };
    std::vector<void*> arg_ptrs = { &data_dim1, &data_dim2, &frac_dout, buffers[0].get(), buffers[1].get(), buffers[2].get() };
    analyzer.analyze(checks, kernel, 3, global_work_sizes.data(), nullptr, arg_ptrs.size(), arg_sizes.data(), arg_ptrs.data());
    return std::move(violations);
}
