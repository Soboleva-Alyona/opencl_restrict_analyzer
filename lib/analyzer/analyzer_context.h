#ifndef OPENCL_RESTRICT_ANALYZER_ANALYZER_CONTEXT_H
#define OPENCL_RESTRICT_ANALYZER_ANALYZER_CONTEXT_H

#include "analyzer_parameters.h"
#include "model/block.h"
#include "model/memory.h"

#include <clang/AST/ASTContext.h>
#include <z3++.h>

class analyzer_context {
public:
    analyzer_context(const analyzer_parameters& parameters, clang::ASTContext& ast_ctx);

    const analyzer_parameters& parameters;
    clang::ASTContext& ast_ctx;
    z3::context z3_ctx;
    z3::solver solver;
    memory mem;
    block_context block_ctx;
};

#endif //OPENCL_RESTRICT_ANALYZER_ANALYZER_CONTEXT_H
