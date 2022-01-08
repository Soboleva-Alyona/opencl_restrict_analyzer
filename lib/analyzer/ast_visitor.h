#ifndef OPENCL_RESTRICT_ANALYZER_AST_VISITOR_H
#define OPENCL_RESTRICT_ANALYZER_AST_VISITOR_H


#include <clang/AST/RecursiveASTVisitor.h>
#include "analyzer_parameters.h"
#include "analyzer_context.h"
#include "model/block.h"
#include "model/memory.h"
#include "checkers/address_checker.h"
#include "checkers/restrict_checker.h"

class ast_visitor : public clang::RecursiveASTVisitor<ast_visitor> {
public:
    explicit ast_visitor(analyzer_context& ctx);
    bool VisitFunctionDecl(clang::FunctionDecl* f);
private:
    void transformStmt(block* block, clang::Stmt* stmt);
    void transformCompoundStmt(block* block, clang::CompoundStmt* compound_stmt);
    void processDeclStmt(block* block, clang::DeclStmt* decl_stmt);
    void processDecl(block* block, clang::Decl* decl);
    void processVarDecl(block* block, clang::VarDecl* var_decl);

    std::optional<z3::expr> transformValueStmt(block* block, const clang::ValueStmt* value_stmt);
    std::optional<z3::expr> transformExpr(block* block, const clang::Expr* expr);
    std::optional<z3::expr> transformArraySubscriptExpr(block* block, const clang::ArraySubscriptExpr* array_subscript_expr);
    std::optional<z3::expr> transformBinaryOperator(block* block, const clang::BinaryOperator* binary_operator);
    std::optional<z3::expr> transformUnaryOperator(block* block, const clang::UnaryOperator* unary_operator);
    std::optional<z3::expr> transformCallExpr(block* block, const clang::CallExpr* call_expr);
    std::optional<z3::expr> transformDeclRefExpr(block* block, const clang::DeclRefExpr* decl_ref_expr);
    std::optional<z3::expr> transformImplicitCastExpr(block* block, const clang::ImplicitCastExpr* implicit_cast_expr);
    std::optional<z3::expr> transformParenExpr(block* block, const clang::ParenExpr* paren_expr);

    void assign(block* block, const clang::Expr* expr, const z3::expr& value);
    z3::expr read(const block* block, const clang::Expr* expr, const z3::expr& address);
    void write(const block* block, const clang::Expr* expr, const z3::expr& address, const z3::expr& value);
    z3::expr push(const z3::expr& value);

    std::vector<z3::expr> predicates;

    analyzer_context& ctx;

    /*const analyzer_parameters& parameters;
    clang::ASTContext& ast_ctx;
    z3::context z3_ctx;
    z3::solver solver;
    memory mem;
    block_context block_ctx;*/

    std::vector<std::unique_ptr<abstract_checker>> checkers;

    size_t mem_version = 1;
};


#endif //OPENCL_RESTRICT_ANALYZER_AST_VISITOR_H
