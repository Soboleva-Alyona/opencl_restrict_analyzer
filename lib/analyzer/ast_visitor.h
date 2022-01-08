#ifndef OPENCL_RESTRICT_ANALYZER_AST_VISITOR_H
#define OPENCL_RESTRICT_ANALYZER_AST_VISITOR_H


#include <clang/AST/RecursiveASTVisitor.h>
#include "analyzer_parameters.h"
#include "analyzer_context.h"
#include "model/scope.h"
#include "model/memory.h"
#include "checkers/address_checker.h"
#include "checkers/restrict_checker.h"

class ast_visitor : public clang::RecursiveASTVisitor<ast_visitor> {
public:
    explicit ast_visitor(analyzer_context& ctx);
    bool VisitFunctionDecl(clang::FunctionDecl* f);
private:
    void transformStmt(scope* scope, clang::Stmt* stmt);
    void transformCompoundStmt(scope* scope, clang::CompoundStmt* compound_stmt);
    void processDeclStmt(scope* scope, clang::DeclStmt* decl_stmt);
    void processDecl(scope* scope, clang::Decl* decl);
    void processVarDecl(scope* scope, clang::VarDecl* var_decl);

    std::optional<z3::expr> transformValueStmt(scope* scope, const clang::ValueStmt* value_stmt);
    std::optional<z3::expr> transformExpr(scope* scope, const clang::Expr* expr);
    std::optional<z3::expr> transformArraySubscriptExpr(scope* scope, const clang::ArraySubscriptExpr* array_subscript_expr);
    std::optional<z3::expr> transformBinaryOperator(scope* scope, const clang::BinaryOperator* binary_operator);
    std::optional<z3::expr> transformUnaryOperator(scope* scope, const clang::UnaryOperator* unary_operator);
    std::optional<z3::expr> transformCallExpr(scope* scope, const clang::CallExpr* call_expr);
    std::optional<z3::expr> transformDeclRefExpr(scope* scope, const clang::DeclRefExpr* decl_ref_expr);
    std::optional<z3::expr> transformImplicitCastExpr(scope* scope, const clang::ImplicitCastExpr* implicit_cast_expr);
    std::optional<z3::expr> transformParenExpr(scope* scope, const clang::ParenExpr* paren_expr);

    z3::expr read(const scope* scope, const clang::Expr* expr, const z3::expr& address);
    void write(const scope* scope, const clang::Expr* expr, const z3::expr& address, const z3::expr& value);
    z3::expr push(const z3::expr& value);

    std::vector<z3::expr> predicates;

    analyzer_context& ctx;

    /*const analyzer_parameters& parameters;
    clang::ASTContext& ast_ctx;
    z3::context z3_ctx;
    z3::solver solver;
    memory mem;
    scope_context scope_ctx;*/

    std::vector<std::unique_ptr<abstract_checker>> checkers;

    size_t mem_version = 1;
};


#endif //OPENCL_RESTRICT_ANALYZER_AST_VISITOR_H
