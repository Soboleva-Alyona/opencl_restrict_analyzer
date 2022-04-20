#include "restrict_checker.h"

#include <clang/AST/Expr.h>

namespace {
    void insert_inner_blocks(std::vector<const clsma::block*>& vector, const clsma::block* block) {
        vector.push_back(block);
        std::for_each(block->inner_begin(), block->inner_end(), [&](const clsma::block* inner) {
            insert_inner_blocks(vector, inner);
        });
    }
}

std::optional<clsma::violation> clsma::restrict_checker::check_memory_access(const clsma::block* const block,
                                                                             const clang::Expr* const expr,
                                                                             const clsma::memory_access_type access_type,
                                                                             const z3::expr& address) {
    const clang::ValueDecl* value_decl = nullptr;
    if (clang::isa<clang::ArraySubscriptExpr>(expr)) {
        const auto* array_subscript_expr = clang::cast<clang::ArraySubscriptExpr>(expr);
        value_decl = get_value_decl(array_subscript_expr->getBase());
    } else if (clang::isa<clang::UnaryOperator>(expr)) {
        const auto* unary_operator = clang::cast<clang::UnaryOperator>(expr);
        if (unary_operator->getOpcode() == clang::UO_Deref) {
            value_decl = get_value_decl(unary_operator->getSubExpr());
        }
    } else if (clang::isa<clang::ImplicitCastExpr>(expr)) {
        return check_memory_access(block, clang::cast<clang::ImplicitCastExpr>(expr)->getSubExpr(), access_type, address);
    }
    const clsma::variable* var = block->var_get(value_decl);
    if (nullptr == var) {
        return std::nullopt;
    }
    const auto access_data = memory_access_data {
        expr, address, var
    };
    accesses[block].emplace_back(access_data);

    std::vector<const clsma::block*> affected_blocks;
    std::unordered_map<const clsma::block*, std::vector<memory_access_data>>* other_accesses_by_block;
    const char* operation_name;
    const char* other_operation_name;
    if (value_decl->getType().isRestrictQualified() && access_type == clsma::write) {
        restrict_writes[block].emplace_back(access_data);

        insert_inner_blocks(affected_blocks, var->block);
        other_accesses_by_block = &accesses;
        operation_name = "restricted write";
        other_operation_name = "access";
    } else {
        for (auto cur_block = block; cur_block != nullptr; cur_block = cur_block->parent) {
            affected_blocks.emplace_back(cur_block);
        }
        other_accesses_by_block = &restrict_writes;
        operation_name = access_type == clsma::write ? "write" : "read";
        other_operation_name = "restricted write";
    }

    z3::expr violation_access_idx = z3_ctx.int_const("IDX");
    z3::expr condition = violation_access_idx == 0;
    std::vector<const memory_access_data*> other_accesses;
    for (const auto* affected_block : affected_blocks) {
        const std::optional<z3::expr> affected_block_assumption = affected_block->get_assumption();
        for (const auto& other_access_data : (*other_accesses_by_block)[affected_block]) {
            if (other_access_data.var != var) {
                other_accesses.emplace_back(&other_access_data);
                z3::expr access_condition = other_access_data.address == address;
                if (affected_block_assumption.has_value()) {
                    access_condition = access_condition && affected_block_assumption.value();
                }
                condition = z3::ite(access_condition, violation_access_idx == z3_ctx.int_val(uint64_t(other_accesses.size())), condition);
            }
        }
    }

    if (const std::optional<z3::model> result = check(block, condition); result.has_value()) {
        const std::int64_t id = result.value().eval(violation_access_idx).get_numeral_int64();
        if (id != 0) {
            const auto* other_access_data = other_accesses[id - 1];

            const std::int64_t var_value = result.value().eval(var->to_z3_expr()).get_numeral_int64();
            const std::int64_t address_value = result.value().eval(address).get_numeral_int64();
            const std::int64_t other_var_address_value = result.value().eval(other_access_data->var->to_z3_expr()).get_numeral_int64();

            std::ostringstream message;
            message << operation_name << " through `" << var->decl->getName().str()
                    << "` (offset " << address_value - var_value << ") clashes with "
                    << other_operation_name << " through `" << other_access_data->var->decl->getName().str()
                    << "` (offset " << address_value - other_var_address_value << ") at "
                    << other_access_data->expr->getExprLoc().printToString(get_source_manager()) << std::endl;
            return clsma::violation {
                .location = expr->getExprLoc(),
                .message = message.str()
            };
        }
    }

    return std::nullopt;
}
