#include "restrict_checker.h"

#include <clang/AST/Expr.h>

namespace {
    void insert_inner_blocks(std::vector<const clsa::block*>& vector, const clsa::block* block) {
        vector.push_back(block);
        std::for_each(block->inner_begin(), block->inner_end(), [&](const clsa::block* inner) {
            insert_inner_blocks(vector, inner);
        });
    }
}

const char* clsa::restrict_checker::get_access_name(const clsa::restrict_checker::memory_access_data& access) {
    if (access.access_type == clsa::write) {
        return access.var->decl->getType().isRestrictQualified() ? "restricted write" : "write";
    }
    return access.var->decl->getType().isRestrictQualified() ? "restricted read" : "read";
}

std::optional<clsa::violation> clsa::restrict_checker::check_memory_access(const clsa::block* const block,
                                                                           const clang::Expr* const expr,
                                                                           const clsa::memory_access_type access_type,
                                                                           const z3::expr& address,
                                                                           const clsa::optional_value& value) {
    const clang::ValueDecl* value_decl = get_pointer_decl(expr);
    const clsa::variable* var = block->var_get(value_decl);

    // todo: remove (for debug)
    // std::cout << "\n Type of expr: "<<expr->getType().getAsString() << '\n';
    // const clang::Qualifiers expression_type_qualifiers = expr->getType().getQualifiers();
    //
    // std::cout << "\n Qualifiers: "<<expression_type_qualifiers.getAsString() << '\n';
    // if (expression_type_qualifiers.getAsString().find("__global") != std::string::npos)
    // {
    //     std::cout << "Is global memeory\n";
    // }

    if (nullptr == var) {
        return std::nullopt;
    }
    const auto access = memory_access_data {
        expr, access_type, address, var
    };

    accesses[block].emplace_back(access);
    if (access_type == clsa::write) {
        writes[block].emplace_back(access);
    }

    std::vector<std::pair<const clsa::block*, const memory_access_data*>> other_accesses;
    {
        auto& other_accesses_by_block = access_type == clsa::write ? accesses : writes;
        {
            std::unordered_set<const clsa::block*> parent_blocks;
            for (auto cur_block = block; cur_block != nullptr; cur_block = cur_block->parent) {
                parent_blocks.emplace(cur_block);
            }
            for (const auto& [other_block, other_block_accesses] : other_accesses_by_block) {
                for (const auto& other_access : other_block_accesses) {
                    if (other_access.var != var && parent_blocks.contains(other_access.var->block) &&
                        other_access.var->decl->getType().isRestrictQualified()) {
                        other_accesses.emplace_back(other_access.var->block, &other_access);
                    }
                }
            }
        }
        if (value_decl->getType().isRestrictQualified()) {
            std::vector<const clsa::block*> other_blocks;
            insert_inner_blocks(other_blocks, var->block);
            for (const auto* other_block : other_blocks) {
                for (const auto& other_access : other_accesses_by_block[other_block]) {
                    if (other_access.var != var) {
                        other_accesses.emplace_back(other_block, &other_access);
                    }
                }
            }
        }
    }

    z3::expr violation_access_idx = z3_ctx.int_const("IDX");
    z3::expr condition = violation_access_idx == 0;
    for (const auto& [other_block, other_access] : other_accesses) {
        const std::optional<z3::expr> other_block_assumption = other_block->get_assumption();
        z3::expr access_condition = other_access->address == address;
        if (other_block_assumption.has_value()) {
            access_condition = access_condition && other_block_assumption.value();
        }
        condition = z3::ite(access_condition,
            violation_access_idx == z3_ctx.int_val(uint64_t(other_accesses.size())), condition);
    }

    if (const std::optional<z3::model> result = check(block, condition); result.has_value()) {
        const std::int64_t id = result.value().eval(violation_access_idx).get_numeral_int64();
        if (id != 0) {
            const auto& [_, other_access] = other_accesses[id - 1];

            const std::int64_t var_value = result.value().eval(var->to_z3_expr()).get_numeral_int64();
            const std::int64_t address_value = result.value().eval(address).get_numeral_int64();
            const std::int64_t other_var_address_value = result.value().eval(
                other_access->var->to_z3_expr()).get_numeral_int64();

            std::ostringstream message;
            message << get_access_name(access) << " through `" << var->decl->getName().str()
                    << "` (offset " << address_value - var_value << ") clashes with "
                    << get_access_name(*other_access) << " through `" << other_access->var->decl->getName().str()
                    << "` (offset " << address_value - other_var_address_value << ") at "
                    << other_access->expr->getExprLoc().printToString(get_source_manager()) << std::endl;
            return clsa::violation {
                .location = expr->getExprLoc(),
                .message = message.str()
            };
        }
    }

    return std::nullopt;
}
