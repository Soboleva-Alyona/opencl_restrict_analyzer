#ifndef OPENCL_RESTRICT_ANALYZER_TEST_UTILS_H
#define OPENCL_RESTRICT_ANALYZER_TEST_UTILS_H


#include <functional>
#include <memory>

#include "../lib/analyzer/core/pseudocl.h"
#include "../lib/analyzer/core/violation.h"

using pseudocl_mem_ptr = std::unique_ptr<clsa::pseudocl_mem, std::function<void(clsa::pseudocl_mem *)>>;

pseudocl_mem_ptr create_buffer_ptr(size_t size);

std::vector<clsa::violation> analyze(std::string_view path, std::string_view kernel, std::uint32_t checks);


#endif //OPENCL_RESTRICT_ANALYZER_TEST_UTILS_H
