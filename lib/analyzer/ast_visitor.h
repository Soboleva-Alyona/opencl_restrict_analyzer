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
    bool process_stmt(clsma::block* block, const clang::Stmt* stmt, clsma::value_reference& ret_ref);
    bool process_compound_stmt(clsma::block* block, const clang::CompoundStmt* compound_stmt, clsma::value_reference& ret_ref);
    bool process_while_stmt(clsma::block* block, const clang::WhileStmt* while_stmt, clsma::value_reference& ret_ref);
    bool process_if_stmt(clsma::block* block, const clang::IfStmt* if_stmt, clsma::value_reference& ret_ref);
    bool process_return_stmt(clsma::block* block, const clang::ReturnStmt* return_stmt, clsma::value_reference& ret_ref);

    void process_decl_stmt(clsma::block* block, const clang::DeclStmt* decl_stmt, clsma::value_reference& ret_ref);
    void process_decl(clsma::block* block, const clang::Decl* decl, clsma::value_reference& ret_ref);
    void process_var_decl(clsma::block* block, const clang::VarDecl* var_decl, clsma::value_reference& ret_ref);

    clsma::optional_value transform_value_stmt(clsma::block* block, const clang::ValueStmt* value_stmt);
    clsma::optional_value transform_expr(clsma::block* block, const clang::Expr* expr);
    clsma::optional_value transform_array_subscript_expr(clsma::block* block, const clang::ArraySubscriptExpr* array_subscript_expr);
    clsma::optional_value transform_binary_operator(clsma::block* block, const clang::BinaryOperator* binary_operator);
    clsma::optional_value transform_unary_operator(clsma::block* block, const clang::UnaryOperator* unary_operator);
    clsma::optional_value transform_call_expr(clsma::block* block, const clang::CallExpr* call_expr);
    clsma::optional_value transform_decl_ref_expr(clsma::block* block, const clang::DeclRefExpr* decl_ref_expr);
    clsma::optional_value transform_implicit_cast_expr(clsma::block* block, const clang::ImplicitCastExpr* implicit_cast_expr);
    clsma::optional_value transform_paren_expr(clsma::block* block, const clang::ParenExpr* paren_expr);

    std::optional<z3::expr> get_address(clsma::block* block, const clang::Expr* expr);
    void assign(clsma::block* block, const clang::Expr* expr, const clsma::optional_value& value);
    void assign(clsma::block* block, const clang::Expr* expr, const clsma::optional_value& value, const std::optional<z3::expr>& storage);
    z3::expr read(const clsma::block* block, const clang::Expr* expr, const z3::expr& address);
    void write(const clsma::block* block, const clang::Expr* expr, const z3::expr& address, const z3::expr& value);
    z3::expr push(const z3::expr& value);

    z3::expr unknown(const z3::sort& sort);

    void check_memory_access(const clsma::block* block, const clang::Expr* expr, abstract_checker::memory_access_type access_type, const z3::expr& address);

    std::vector<z3::expr> predicates;

    analyzer_context& ctx;
    clsma::block* global_block;

    clsma::block* get_call_block(clsma::block* block);

    /*const analyzer_parameters& parameters;
    clang::ASTContext& ast_ctx;
    z3::context z3_ctx;
    z3::solver solver;
    memory mem;
    block_context block_ctx;*/

    std::vector<std::unique_ptr<abstract_checker>> checkers;

    size_t unknowns = 0;
};


#endif //OPENCL_RESTRICT_ANALYZER_AST_VISITOR_H
