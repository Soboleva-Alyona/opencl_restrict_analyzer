#include "analyzer_context.h"

clsma::analyzer_context::analyzer_context(const clsma::analyzer_parameters& parameters, clang::ASTContext& ast_ctx) :
        parameters(parameters), ast(ast_ctx), solver(z3), block(solver) {}
