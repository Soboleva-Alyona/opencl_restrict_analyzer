#ifndef OPENCL_RESTRICT_ANALYZER_ANALYZER_H
#define OPENCL_RESTRICT_ANALYZER_ANALYZER_H


#include <string_view>

#include <clang/Frontend/CompilerInstance.h>

namespace clsma {

enum checks {
    restrict = 1,
    address = 2,
};

class analyzer {
public:
    explicit analyzer(std::string_view filename);

    void analyze(uint32_t checks, std::string_view kernel_name, uint32_t work_dim, const size_t* global_work_size,
                 const size_t* local_work_size, size_t args_count, const size_t* arg_sizes, void** arg_values);

private:
    clang::CompilerInstance compiler_instance;
    const clang::FileEntry* file_entry;
};

}

#endif //OPENCL_RESTRICT_ANALYZER_ANALYZER_H
