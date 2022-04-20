#ifndef OPENCL_RESTRICT_ANALYZER_ANALYZER_PARAMETERS_H
#define OPENCL_RESTRICT_ANALYZER_ANALYZER_PARAMETERS_H


#include <cstdlib>
#include <string>
#include <optional>
#include <utility>
#include <vector>

#include "analyzer_options.h"

namespace clsma {

    struct analyzer_parameters {
        const std::string kernel_name;
        const std::vector<std::pair<size_t, void*>> args;
        const std::uint32_t work_dim;
        const std::vector<size_t> global_work_size;
        const std::optional<std::vector<size_t>> local_work_size;
        const clsma::analyzer_options options;
    };

}


#endif //OPENCL_RESTRICT_ANALYZER_ANALYZER_PARAMETERS_H
