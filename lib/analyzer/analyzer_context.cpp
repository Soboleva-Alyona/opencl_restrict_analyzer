#include "analyzer_context.h"

analyzer_context::analyzer_context(const analyzer_parameters &parameters, clang::ASTContext &ast_ctx) :
parameters(parameters), ast_ctx(ast_ctx), solver(z3_ctx), mem(solver), block_ctx(ast_ctx, solver, mem) {}
