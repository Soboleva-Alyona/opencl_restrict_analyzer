#include "analyzer_context.h"

analyzer_context::analyzer_context(const analyzer_parameters& parameters, clang::ASTContext& ast_ctx) :
        parameters(parameters), ast(ast_ctx), solver(z3), block(ast_ctx, solver) {}
