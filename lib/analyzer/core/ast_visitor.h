#ifndef OPENCL_RESTRICT_ANALYZER_AST_VISITOR_H
#define OPENCL_RESTRICT_ANALYZER_AST_VISITOR_H


#include <clang/AST/RecursiveASTVisitor.h>

#include "abstract_checker.h"
#include "analyzer_context.h"
#include "analyzer_parameters.h"
#include "block.h"
#include "violation.h"

namespace clsa {

    constexpr const char* VAR_META_MEM_BASE = "base";
    constexpr const char* VAR_META_MEM_SIZE = "size";

    class ast_visitor : public clang::RecursiveASTVisitor<ast_visitor> {
    public:
        explicit ast_visitor(analyzer_context& ctx);

        void set_violation_handler(std::function<void(clsa::violation)>);

        void add_checker(std::unique_ptr<clsa::abstract_checker> checker);

        [[maybe_unused]] bool VisitFunctionDecl(clang::FunctionDecl* f);

    private:
        bool process_stmt(clsa::block* block, const clang::Stmt* stmt, clsa::value_reference& ret_ref);

        bool process_compound_stmt(clsa::block* block, const clang::CompoundStmt* compound_stmt,
                                   clsa::value_reference& ret_ref);

        bool
        process_while_stmt(clsa::block* block, const clang::WhileStmt* while_stmt, clsa::value_reference& ret_ref);

        bool process_do_stmt(clsa::block* block, const clang::DoStmt* do_stmt, clsa::value_reference& ref_ref);

        bool process_for_stmt(clsa::block* block, const clang::ForStmt* for_stmt, clsa::value_reference& ret_ref);

        bool process_if_stmt(clsa::block* block, const clang::IfStmt* if_stmt, clsa::value_reference& ret_ref);

        bool
        process_return_stmt(clsa::block* block, const clang::ReturnStmt* return_stmt, clsa::value_reference& ret_ref);

        void process_decl_stmt(clsa::block* block, const clang::DeclStmt* decl_stmt, clsa::value_reference& ret_ref);

        void process_decl(clsa::block* block, const clang::Decl* decl, clsa::value_reference& ret_ref);

        void process_var_decl(clsa::block* block, const clang::VarDecl* var_decl, clsa::value_reference& ret_ref);

        clsa::optional_value transform_value_stmt(clsa::block* block, const clang::ValueStmt* value_stmt);

        clsa::optional_value transform_expr(clsa::block* block, const clang::Expr* expr);

        clsa::optional_value
        transform_array_subscript_expr(clsa::block* block, const clang::ArraySubscriptExpr* array_subscript_expr);

        clsa::optional_value
        transform_binary_operator(clsa::block* block, const clang::BinaryOperator* binary_operator);

        clsa::optional_value transform_unary_operator(clsa::block* block, const clang::UnaryOperator* unary_operator);

        clsa::optional_value transform_call_expr(clsa::block* block, const clang::CallExpr* call_expr);

        clsa::optional_value transform_decl_ref_expr(clsa::block* block, const clang::DeclRefExpr* decl_ref_expr);

        clsa::optional_value
        transform_implicit_cast_expr(clsa::block* block, const clang::ImplicitCastExpr* implicit_cast_expr);

        clsa::optional_value transform_paren_expr(clsa::block* block, const clang::ParenExpr* paren_expr);

        void assign(clsa::block* block, const clang::Expr* expr, const clsa::optional_value& value);

        void assign(clsa::block* block, const clang::Expr* expr, const clsa::optional_value& value,
                    const std::optional<z3::expr>& storage);

        z3::expr unknown(const z3::sort& sort);

        void
        check_memory_access(const clsa::block* block, const clang::Expr* expr, clsa::memory_access_type access_type,
                            const z3::expr& address);

        clsa::optional_value get_dim_value(const std::vector<clsa::value_reference*>& values, const std::vector<clsa::optional_value>& args, const z3::expr& default_value);

        // builtin handlers - https://www.khronos.org/registry/OpenCL/sdk/2.0/docs/man/html/workItemFunctions.html
        clsa::optional_value handle_get_work_dim(const std::vector<clsa::optional_value>& args);

        clsa::optional_value handle_get_global_size(const std::vector<clsa::optional_value>& args);

        clsa::optional_value handle_get_global_id(const std::vector<clsa::optional_value>& args);

        clsa::optional_value handle_get_local_size(const std::vector<clsa::optional_value>& args);

        clsa::optional_value handle_get_enqueued_local_size(const std::vector<clsa::optional_value>& args);

        clsa::optional_value handle_get_local_id(const std::vector<clsa::optional_value>& args);

        clsa::optional_value handle_get_num_groups(const std::vector<clsa::optional_value>& args);

        clsa::optional_value handle_get_group_id(const std::vector<clsa::optional_value>& args);

        clsa::optional_value handle_get_global_offset(const std::vector<clsa::optional_value>& args);

        clsa::optional_value handle_get_global_linear_id(const std::vector<clsa::optional_value>& args);

        clsa::optional_value handle_get_local_linear_id(const std::vector<clsa::optional_value>& args);
        // builtin handlers

        clsa::analyzer_context& ctx;
        clsa::block* global_block;

        std::vector<clsa::value_reference*> global_sizes;
        std::vector<clsa::value_reference*> global_ids;
        std::vector<clsa::value_reference*> local_sizes;
        std::vector<clsa::value_reference*> enqueued_local_sizes;
        std::vector<clsa::value_reference*> local_ids;
        std::vector<clsa::value_reference*> group_nums;
        std::vector<clsa::value_reference*> group_ids;
        std::vector<clsa::value_reference*> global_offsets;

        std::vector<std::unique_ptr<clsa::abstract_checker>> checkers;

        std::function<void(clsa::violation)> violation_handler;

        size_t unknowns = 0;
    };

}


#endif //OPENCL_RESTRICT_ANALYZER_AST_VISITOR_H
