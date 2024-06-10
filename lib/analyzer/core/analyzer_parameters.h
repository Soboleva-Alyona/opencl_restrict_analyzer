#ifndef OPENCL_RESTRICT_ANALYZER_ANALYZER_PARAMETERS_H
#define OPENCL_RESTRICT_ANALYZER_ANALYZER_PARAMETERS_H


#include <cstdlib>
#include <string>
#include <optional>
#include <utility>
#include <vector>

#include "analyzer_options.h"

namespace clsa {

    struct analyzer_parameters {
        const std::string kernel_name;
        const std::vector<std::pair<std::size_t, void*>> args;
        const std::uint32_t work_dim;
        const std::optional<std::vector<std::size_t>> global_work_offset;
        const std::vector<std::size_t> global_work_size;
        const std::optional<std::vector<std::size_t>> local_work_size;
        const std::uint32_t sub_group_size;
        const clsa::analyzer_options options;
    };

}


#endif //OPENCL_RESTRICT_ANALYZER_ANALYZER_PARAMETERS_H
