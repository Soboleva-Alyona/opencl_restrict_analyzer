#ifndef OPENCL_RESTRICT_ANALYZER_ANALYZER_H
#define OPENCL_RESTRICT_ANALYZER_ANALYZER_H


#include <string_view>
#include <unordered_set>

#include <clang/AST/ASTContext.h>
#include <clang/Frontend/CompilerInstance.h>

#include "../core/analyzer_options.h"
#include "../core/violation.h"

namespace clsa {

    class analyzer {
    public:
        enum checks {
            bounds = 1,
            restrict = 2,
            race = 3,
        };

        explicit analyzer(std::string_view filename);

        void set_violation_handler(std::function<void(const clang::ASTContext&, const clsa::violation&)> handler) noexcept;

        void analyze(std::set<uint32_t>* checks, std::string_view kernel_name, std::uint32_t work_dim,
                     const std::size_t* global_work_size, const std::size_t* local_work_size, std::uint32_t sub_group_size,
                     std::size_t args_count, const std::size_t* arg_sizes, void** arg_values, const clsa::analyzer_options& = {});

    private:
        clang::CompilerInstance compiler_instance;
        const clang::FileEntry* file_entry;

        std::function<void(const clang::ASTContext&, const clsa::violation&)> violation_handler;
    };

}

#endif //OPENCL_RESTRICT_ANALYZER_ANALYZER_H
