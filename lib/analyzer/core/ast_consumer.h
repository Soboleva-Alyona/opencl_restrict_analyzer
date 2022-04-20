#ifndef OPENCL_RESTRICT_ANALYZER_AST_CONSUMER_H
#define OPENCL_RESTRICT_ANALYZER_AST_CONSUMER_H


#include <clang/AST/ASTConsumer.h>

#include "analyzer_context.h"
#include "analyzer_parameters.h"
#include "ast_visitor.h"

namespace clsa {

    class ast_consumer : public clang::ASTConsumer {
    public:
        explicit ast_consumer(const clsa::analyzer_parameters& parameters,
                              std::function<void(clsa::ast_consumer&, clsa::analyzer_context&)> initializer);

        void Initialize(clang::ASTContext& ctx) override;
        void HandleTranslationUnit(clang::ASTContext& ctx) override;

        void set_violation_handler(std::function<void(clsa::violation)> handler);

        void add_checker(std::unique_ptr<clsa::abstract_checker> checker);

    private:
        const clsa::analyzer_parameters& parameters;
        std::function<void(clsa::ast_consumer&, clsa::analyzer_context&)> initializer;

        std::optional<clsa::analyzer_context> ctx;

        std::function<void(clsa::violation)> violation_handler;
        std::vector<std::unique_ptr<clsa::abstract_checker>> checkers;

        std::unique_ptr<clsa::ast_visitor> visitor;
    };

}


#endif //OPENCL_RESTRICT_ANALYZER_AST_CONSUMER_H
