#include <iostream>
#include <regex>
#include <sstream>

#include <clang/Frontend/FrontendActions.h>
#include <clang/Tooling/Tooling.h>
#include <llvm/Support/CommandLine.h>

#include "lib/analyzer/core/analyzer_parameters.h"
#include "lib/analyzer/core/pseudocl.h"
#include "lib/analyzer/frontend/analyzer.h"

llvm::cl::OptionCategory option_category("Analyzer Options", "Options for controlling the analysis process.");

llvm::cl::opt<std::string> filename(llvm::cl::cat(option_category), "input", llvm::cl::Required,
    llvm::cl::desc("The input file."));

llvm::cl::opt<std::string> kernel_name(llvm::cl::cat(option_category), "kernel", llvm::cl::Required,
    llvm::cl::desc("The name of the kernel to analyze."));

enum checks {
    bounds,
    restrict,
    race
};

llvm::cl::bits<clsa::analyzer::checks> checks(llvm::cl::cat(option_category), llvm::cl::desc("Available Checks:"),
    llvm::cl::OneOrMore, llvm::cl::values(
        clEnumValN(checks::bounds, "check-bounds", "Look for out of bounds memory accesses."),
        clEnumValN(checks::restrict, "check-restrict", "Look for 'restrict' constraint violations."),
        clEnumValN(checks::race, "check-race", "Look for data races.")
        ));

llvm::cl::opt<std::uint32_t> work_dim(llvm::cl::cat(option_category), "work-dim", llvm::cl::Required,
    llvm::cl::desc("The number of dimensions used to specify the global work-items and work-items in the work-group. "
                   "work-dim must be greater than zero and less than or equal to three."));

llvm::cl::list<std::size_t> global_work_size(llvm::cl::cat(option_category), "global-work-size", llvm::cl::OneOrMore,
    llvm::cl::desc("List of work-dim unsigned values that describe the number of global work-items in work-dim "
                   "dimensions that will execute the kernel function. The total number of global work-items is "
                   "computed as global-work-size[0] * ... * global-work-size[work-dim - 1]."));

llvm::cl::list<std::size_t> local_work_size(llvm::cl::cat(option_category), "local-work-size", llvm::cl::ZeroOrMore,
    llvm::cl::desc("List of work-dim unsigned values that describe the number of work-items that make up a work-group. "
                   "The total number of work-items in a work-group is computed as local-work-size[0] * ... * "
                   "local-work-size[work_dim - 1]. If it is not specified, the implementation will determine how to "
                   "break the global work-items into appropriate work-group instances."));

llvm::cl::list<std::string> args(llvm::cl::cat(option_category), "arg", llvm::cl::ZeroOrMore,
    llvm::cl::desc("Arguments passed to the kernel. You can skip an argument specifying it as 'unknown'. "
                   "Only integer and buffer arguments are supported. Rest must be unknown. "
                   "Integer arguments can be postfixed with i<ulong> or u<ulong> to be interpreted"
                   "as a signed or an unsigned value of <ulong> bits. Default is i64."
                   "Buffer arguments must be specified as 'b<ulong>', with the number interpreted as size in bytes. "
                   "Note: if you specify invalid arguments, the program may crash. Also, its strictly recommended "
                   "to specify all buffer arguments - otherwise you may get weird results."));

namespace {
    template<typename T>
    void emplace_value(std::vector<std::unique_ptr<void, std::function<void(void*)>>>& arg_ptrs,
                       std::vector<size_t>& arg_sizes, const T& value) {
        arg_ptrs.emplace_back(new T(value), [](void* ptr) {
            delete (T*) ptr;
        });
        arg_sizes.emplace_back(sizeof(T));
    }

    template<typename T>
    T parse(const std::string& input) {
        T value;
        std::istringstream(input) >> value;
        return value;
    }
}

int main(int argc, const char** argv) {
    llvm::cl::HideUnrelatedOptions(option_category);
    llvm::cl::ParseCommandLineOptions(argc, argv);

    if (global_work_size.size() != work_dim) {
        std::cerr << "global-work-size should be specified exactly `work-dim` times" << std::endl;
        return 1;
    }
    if (!local_work_size.empty() && local_work_size.size() != work_dim) {
        std::cerr << "local-work-size should be omitted or specified exactly `work-dim` times" << std::endl;
        return 1;
    }

    std::set<uint32_t> analyzer_checks;
    if (checks.isSet(checks::bounds)) {
        analyzer_checks.insert(clsa::analyzer::checks::bounds);
    }
    if (checks.isSet(checks::restrict)) {
        analyzer_checks.insert(clsa::analyzer::checks::restrict);
    }
    if (checks.isSet(checks::race)) {
        analyzer_checks.insert(clsa::analyzer::checks::race);
    }

    std::vector<std::size_t> global_sizes = global_work_size;
    std::vector<std::size_t> local_sizes = local_work_size;

    std::regex buffer_regex("^b(\\d+)$");
    std::regex integer_regex("^(-?\\d+)((i|u)\\d+)?$");

    std::vector<std::unique_ptr<void, std::function<void(void*)>>> arg_ptrs;
    std::vector<size_t> arg_sizes;

    for (auto&& arg : args) {
        std::smatch match;
        if (arg == "unknown") {
            arg_ptrs.emplace_back(nullptr);
            arg_sizes.emplace_back(0);
        } else if (std::regex_match(arg, match, buffer_regex)) {
            arg_ptrs.emplace_back(new clsa::pseudocl_mem(clsa::pseudocl_create_buffer(std::stoull(match[1].str()))), [](void* ptr) {
                clsa::pseudocl_release_mem_object(*(clsa::pseudocl_mem*) ptr);
                delete (clsa::pseudocl_mem*) ptr;
            });
            arg_sizes.emplace_back(sizeof(clsa::pseudocl_mem));
        } else if (std::regex_match(arg, match, integer_regex)) {
            std::string integer = match[1].str();
            std::string type = match[2].str();
            if (type.empty()) {
                type = "i64";
            }
            if (type == "i8") {
                emplace_value(arg_ptrs, arg_sizes, parse<int8_t>(integer));
            } else if (type == "u8") {
                emplace_value(arg_ptrs, arg_sizes, parse<uint8_t>(integer));
            } else if (type == "i16") {
                emplace_value(arg_ptrs, arg_sizes, parse<int16_t>(integer));
            } else if (type == "u16") {
                emplace_value(arg_ptrs, arg_sizes, parse<uint16_t>(integer));
            } else if (type == "i32") {
                emplace_value(arg_ptrs, arg_sizes, parse<int32_t>(integer));
            } else if (type == "u32") {
                emplace_value(arg_ptrs, arg_sizes, parse<uint32_t>(integer));
            } else if (type == "i64") {
                emplace_value(arg_ptrs, arg_sizes, parse<int64_t>(integer));
            } else if (type == "u64") {
                emplace_value(arg_ptrs, arg_sizes, parse<uint64_t>(integer));
            } else {
                std::cerr << "invalid integer type: " << type << std::endl;
                return 1;
            }
        } else {
            std::cerr << "invalid argument format: " << arg << std::endl;
            return 1;
        }
    }

    std::vector<void*> arg_pure_ptrs;
    for (auto&& ptr : arg_ptrs) {
        arg_pure_ptrs.emplace_back(ptr.get());
    }

    clsa::analyzer analyzer(filename);
    try {
        analyzer.set_violation_handler([](const clang::ASTContext& ctx, const clsa::violation& violation) {
            std::cout << "violation at " << violation.location.printToString(ctx.getSourceManager())
                      << ": " << violation.message;
        });
        analyzer.analyze(&analyzer_checks, kernel_name, work_dim, global_sizes.data(),
            local_sizes.empty() ? nullptr : local_sizes.data(), arg_sizes.size(), arg_sizes.data(), arg_pure_ptrs.data());
    } catch (const std::exception& e) {
        std::cerr << "an error occurred during analysis: " << e.what() << std::endl;
    }

    return 0;
}
