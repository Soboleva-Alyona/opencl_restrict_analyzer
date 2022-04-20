#ifndef OPENCL_RESTRICT_ANALYZER_AST_CONSUMER_H
#define OPENCL_RESTRICT_ANALYZER_AST_CONSUMER_H


#include <clang/AST/ASTConsumer.h>

#include "analyzer_context.h"
#include "analyzer_parameters.h"
#include "ast_visitor.h"

namespace clsma {

    class ast_consumer : public clang::ASTConsumer {
    public:
        explicit ast_consumer(const clsma::analyzer_parameters& parameters,
                              std::function<void(clsma::ast_consumer&, clsma::analyzer_context&)> initializer);

        void Initialize(clang::ASTContext& ctx) override;
        void HandleTranslationUnit(clang::ASTContext& ctx) override;

        void set_violation_handler(std::function<void(clsma::violation)> handler);

        void add_checker(std::unique_ptr<clsma::abstract_checker> checker);

    private:
        const clsma::analyzer_parameters& parameters;
        std::function<void(clsma::ast_consumer&, clsma::analyzer_context&)> initializer;

        std::optional<clsma::analyzer_context> ctx;

        std::function<void(clsma::violation)> violation_handler;
        std::vector<std::unique_ptr<clsma::abstract_checker>> checkers;

        std::unique_ptr<clsma::ast_visitor> visitor;
    };

}


#endif //OPENCL_RESTRICT_ANALYZER_AST_CONSUMER_H
