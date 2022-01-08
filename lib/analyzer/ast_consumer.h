#ifndef OPENCL_RESTRICT_ANALYZER_AST_CONSUMER_H
#define OPENCL_RESTRICT_ANALYZER_AST_CONSUMER_H


#include <clang/AST/ASTConsumer.h>
#include "analyzer_parameters.h"
#include "ast_visitor.h"

class ast_consumer : public clang::ASTConsumer {
public:
    explicit ast_consumer(const analyzer_parameters& parameters);
    void HandleTranslationUnit(clang::ASTContext& ctx) override;
private:
    const analyzer_parameters& parameters;
};


#endif //OPENCL_RESTRICT_ANALYZER_AST_CONSUMER_H
