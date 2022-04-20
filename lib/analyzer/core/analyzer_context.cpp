#include "analyzer_context.h"

clsa::analyzer_context::analyzer_context(const clsa::analyzer_parameters& parameters, clang::ASTContext& ast_ctx) :
        parameters(parameters), ast(ast_ctx), solver(z3), block(solver) {}
