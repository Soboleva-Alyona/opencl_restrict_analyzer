#ifndef OPENCL_RESTRICT_ANALYZER_ANALYZER_CONTEXT_H
#define OPENCL_RESTRICT_ANALYZER_ANALYZER_CONTEXT_H


#include <clang/AST/ASTContext.h>

#include <z3++.h>

#include "analyzer_parameters.h"
#include "block.h"

namespace clsma {
    class ast_visitor;

    class analyzer_context {
        friend class clsma::ast_visitor;
    public:
        analyzer_context(const clsma::analyzer_parameters& parameters, clang::ASTContext& ast_ctx);

        const clsma::analyzer_parameters& parameters;

        clang::ASTContext& ast;
        z3::context z3;
        z3::solver solver;
        clsma::block_context block;
    };

}

#endif //OPENCL_RESTRICT_ANALYZER_ANALYZER_CONTEXT_H
