#ifndef OPENCL_RESTRICT_ANALYZER_ANALYZER_H
#define OPENCL_RESTRICT_ANALYZER_ANALYZER_H


#include <string_view>
#include <unordered_set>

#include <clang/Frontend/CompilerInstance.h>

#include "../core/analyzer_options.h"

namespace clsma {

    class analyzer {
    public:
        enum checks {
            restrict = 1,
            address = 2,
        };

        explicit analyzer(std::string_view filename);

        void analyze(std::uint32_t checks, std::string_view kernel_name, std::uint32_t work_dim,
            const std::size_t* global_work_size, const std::size_t* local_work_size, std::size_t args_count,
            const std::size_t* arg_sizes, void** arg_values, const clsma::analyzer_options& = {});

    private:
        clang::CompilerInstance compiler_instance;
        const clang::FileEntry* file_entry;
    };

}

#endif //OPENCL_RESTRICT_ANALYZER_ANALYZER_H
