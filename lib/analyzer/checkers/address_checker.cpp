#include "address_checker.h"

#include "../core/ast_visitor.h"

std::optional<clsa::violation> clsa::address_checker::check_memory_access(const clsa::block* const block,
                                                                          const clang::Expr* const expr,
                                                                          const clsa::memory_access_type access_type,
                                                                          const z3::expr& address) {
    const clsa::variable* var = block->var_get(get_pointer_decl(expr));
    if (nullptr == var) {
        return std::nullopt;
    }
    const std::optional<z3::model> result = check(block, address < var->to_z3_expr(clsa::VAR_META_MEM_BASE)
        || address >= var->to_z3_expr(clsa::VAR_META_MEM_BASE) + var->to_z3_expr(clsa::VAR_META_MEM_SIZE));
    if (result.has_value()) {
        const std::int64_t var_value = result.value().eval(var->to_z3_expr()).get_numeral_int64();
        const std::int64_t address_value = result->eval(address).get_numeral_int64();
        const std::int64_t base_address_value = result->eval(var->to_z3_expr(clsa::VAR_META_MEM_BASE)).get_numeral_int64();
        const std::int64_t size_value = result->eval(var->to_z3_expr(clsa::VAR_META_MEM_SIZE)).get_numeral_int64();
        std::ostringstream message;
        message << "access through `" << var->decl->getName().str() << "`"
                << " (offset " << address_value - var_value << ") is out of bounds"
                << " (ptr offset is " << var_value - base_address_value
                << ", memory block size is " << size_value << ")" << std::endl;
        return clsa::violation {
            .location = expr->getExprLoc(),
            .message = message.str()
        };
    }
    return std::nullopt;
}
