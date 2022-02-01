#include "address_checker.h"

std::optional<clsma::violation> address_checker::check_memory_access(const clsma::block* block, const clang::Expr* expr,
                                                                     abstract_checker::memory_access_type access_type,
                                                                     const z3::expr& address) {
    const clsma::variable* var = block->get_var(get_pointer_decl(expr));
    if (nullptr == var) {
        return std::nullopt;
    }
    const auto result = check(block, address < var->to_z3_expr("begin") || address >= var->to_z3_expr("begin") + var->to_z3_expr("size"));
    if (result.has_value()) {
        return clsma::violation {
            .location = expr->getExprLoc(),
            .message = "out of bounds :("
        };
    }
    return std::nullopt;
}
