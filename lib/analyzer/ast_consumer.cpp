#include "analyzer_context.h"
#include "ast_consumer.h"

ast_consumer::ast_consumer(const analyzer_parameters& parameters) : parameters(parameters) { }

void ast_consumer::HandleTranslationUnit(clang::ASTContext& ast_ctx) {
    analyzer_context ctx(parameters, ast_ctx);
    ast_visitor visitor(ctx);
    visitor.TraverseAST(ast_ctx);
}
