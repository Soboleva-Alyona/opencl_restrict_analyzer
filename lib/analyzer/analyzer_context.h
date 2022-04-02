#ifndef OPENCL_RESTRICT_ANALYZER_ANALYZER_CONTEXT_H
#define OPENCL_RESTRICT_ANALYZER_ANALYZER_CONTEXT_H

#include "analyzer_parameters.h"
#include "model/block.h"

#include <clang/AST/ASTContext.h>
#include <z3++.h>

class analyzer_context {
public:
    analyzer_context(const analyzer_parameters& parameters, clang::ASTContext& ast_ctx);

    const analyzer_parameters& parameters;

    clang::ASTContext& ast;
    z3::context z3;
    z3::solver solver;
    clsma::block_context block;

    std::vector<clsma::value_reference*> global_ids;
    std::vector<clsma::value_reference*> local_sizes;
    std::vector<clsma::value_reference*> local_ids;
};

#endif //OPENCL_RESTRICT_ANALYZER_ANALYZER_CONTEXT_H
