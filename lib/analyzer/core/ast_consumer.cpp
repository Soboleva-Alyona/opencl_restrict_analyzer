#include "analyzer_context.h"
#include "ast_consumer.h"

#include <utility>

clsma::ast_consumer::ast_consumer(const clsma::analyzer_parameters& parameters,
                                  std::function<void(clsma::ast_consumer&, clsma::analyzer_context&)> initializer)
                                  : parameters(parameters), initializer(std::move(initializer)) {}

void clsma::ast_consumer::Initialize(clang::ASTContext& ast_ctx) {
    ctx.emplace(parameters, ast_ctx);
    visitor = std::make_unique<clsma::ast_visitor>(*ctx);
    visitor->set_violation_handler(std::move(violation_handler));
    for (auto&& checker : checkers) {
        visitor->add_checker(std::move(checker));
    }
    checkers.clear();
    initializer(*this, *ctx);
}

void clsma::ast_consumer::HandleTranslationUnit(clang::ASTContext& ast_ctx) {
    visitor->TraverseAST(ast_ctx);
}

void clsma::ast_consumer::set_violation_handler(std::function<void(clsma::violation)> handler) {
    if (visitor) {
        visitor->set_violation_handler(std::move(handler));
    } else {
        this->violation_handler = handler;
    }
}

void clsma::ast_consumer::add_checker(std::unique_ptr<clsma::abstract_checker> checker) {
    if (visitor) {
        visitor->add_checker(std::move(checker));
    } else {
        checkers.emplace_back(std::move(checker));
    }
}
