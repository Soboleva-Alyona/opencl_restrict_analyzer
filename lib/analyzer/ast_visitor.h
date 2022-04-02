#ifndef OPENCL_RESTRICT_ANALYZER_AST_VISITOR_H
#define OPENCL_RESTRICT_ANALYZER_AST_VISITOR_H


#include <clang/AST/RecursiveASTVisitor.h>
#include "analyzer_parameters.h"
#include "analyzer_context.h"
#include "model/block.h"
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
    bool process_do_stmt(clsma::block* block, const clang::DoStmt* do_stmt, clsma::value_reference& ref_ref);
    bool process_for_stmt(clsma::block* block, const clang::ForStmt* for_stmt, clsma::value_reference& ret_ref);
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

    // builtin handlers - https://www.khronos.org/registry/OpenCL/sdk/2.2/docs/man/html/workItemFunctions.html
    clsma::optional_value handle_get_work_dim(const std::vector<clsma::optional_value>& args);
    clsma::optional_value handle_get_global_size(const std::vector<clsma::optional_value>& args);
    clsma::optional_value handle_get_global_id(const std::vector<clsma::optional_value>& args);
    //clsma::optional_value handle_get_local_size(const std::vector<clsma::optional_value>& args);
    //clsma::optional_value handle_get_enqueued_local_size(const std::vector<clsma::optional_value>& args);
    clsma::optional_value handle_get_local_id(const std::vector<clsma::optional_value>& args);
    //clsma::optional_value handle_get_num_groups(const std::vector<clsma::optional_value>& args);
    clsma::optional_value handle_get_group_id(const std::vector<clsma::optional_value>& args);
    //clsma::optional_value handle_get_global_offset(const std::vector<clsma::optional_value>& args);
    //clsma::optional_value handle_get_global_linear_id(const std::vector<clsma::optional_value>& args);
    //clsma::optional_value handle_get_local_linear_id(const std::vector<clsma::optional_value>& args);

    std::optional<z3::expr> get_address(clsma::block* block, const clang::Expr* expr);
    void assign(clsma::block* block, const clang::Expr* expr, const clsma::optional_value& value);
    void assign(clsma::block* block, const clang::Expr* expr, const clsma::optional_value& value, const std::optional<z3::expr>& storage);

    z3::expr unknown(const z3::sort& sort);

    void check_memory_access(const clsma::block* block, const clang::Expr* expr, clsma::memory_access_type access_type, const z3::expr& address);

    analyzer_context& ctx;
    clsma::block* global_block;

    clsma::block* get_call_block(clsma::block* block);

    std::vector<std::unique_ptr<abstract_checker>> checkers;

    size_t unknowns = 0;
};


#endif //OPENCL_RESTRICT_ANALYZER_AST_VISITOR_H
