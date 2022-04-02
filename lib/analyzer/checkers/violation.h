#ifndef OPENCL_RESTRICT_ANALYZER_VIOLATION_H
#define OPENCL_RESTRICT_ANALYZER_VIOLATION_H


#include <string>

#include <clang/Basic/SourceLocation.h>

namespace clsma {

    struct violation {
        const clang::SourceLocation location;
        const std::string message;
    };

}


#endif //OPENCL_RESTRICT_ANALYZER_VIOLATION_H
