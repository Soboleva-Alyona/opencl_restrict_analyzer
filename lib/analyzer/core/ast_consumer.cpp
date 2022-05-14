#include "analyzer_context.h"
#include "ast_consumer.h"

#include <utility>

clsa::ast_consumer::ast_consumer(const clsa::analyzer_parameters& parameters,
                                 std::function<void(clsa::ast_consumer&, clsa::analyzer_context&)> initializer)
    : parameters(parameters), initializer(std::move(initializer)) {}

void clsa::ast_consumer::Initialize(clang::ASTContext& ast_ctx) {
    ctx.emplace(parameters, ast_ctx);
    visitor = std::make_unique<clsa::ast_visitor>(*ctx);
    visitor->set_violation_handler(std::move(violation_handler));
    for (auto&& checker : checkers) {
        visitor->add_checker(std::move(checker));
    }
    checkers.clear();
    initializer(*this, *ctx);
}

void clsa::ast_consumer::HandleTranslationUnit(clang::ASTContext& ast_ctx) {
    try {
        visitor->TraverseAST(ast_ctx);
    } catch (const z3::exception& e) {
        throw std::logic_error(std::string("Z3 error: ") + e.msg());
    }
}

void clsa::ast_consumer::set_violation_handler(std::function<void(clsa::violation)> handler) {
    if (visitor) {
        visitor->set_violation_handler(std::move(handler));
    } else {
        this->violation_handler = handler;
    }
}

void clsa::ast_consumer::add_checker(std::unique_ptr<clsa::abstract_checker> checker) {
    if (visitor) {
        visitor->add_checker(std::move(checker));
    } else {
        checkers.emplace_back(std::move(checker));
    }
}
