#ifndef OPENCL_RESTRICT_ANALYZER_ANALYZER_H
#define OPENCL_RESTRICT_ANALYZER_ANALYZER_H


#include <string_view>
#include <clang/Frontend/CompilerInstance.h>

class analyzer {
public:
    explicit analyzer(std::string_view source);

    void analyze(std::string_view kernel_name, uint32_t work_dim, const size_t* global_work_size, const size_t* local_work_size, size_t args_count, const size_t* arg_sizes, void** arg_values);
private:
    clang::CompilerInstance compiler_instance;
    const clang::FileEntry* file_entry;
};


#endif //OPENCL_RESTRICT_ANALYZER_ANALYZER_H
