#ifndef OPENCL_RESTRICT_ANALYZER_ANALYZER_PARAMETERS_H
#define OPENCL_RESTRICT_ANALYZER_ANALYZER_PARAMETERS_H


#include <cstdlib>
#include <string>
#include <utility>
#include <vector>

struct analyzer_parameters {
    const std::string kernel_name;
    const std::vector<std::pair<size_t, void*>> args;
    const uint32_t work_dim;
    const std::vector<size_t> global_work_size;
    const std::vector<size_t> local_work_size;
};


#endif //OPENCL_RESTRICT_ANALYZER_ANALYZER_PARAMETERS_H
