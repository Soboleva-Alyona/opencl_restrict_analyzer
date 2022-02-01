#include "restrict_checker.h"
#include "../model/block.h"

#include <clang/AST/ASTContext.h>
#include <clang/AST/Expr.h>

restrict_checker::restrict_checker(analyzer_context& ctx) : abstract_checker(ctx) {
    //const auto address = z3_ctx.constant("address", z3_ctx.int_sort());
    //const auto decl_id = z3_ctx.constant("decl_id", z3_ctx.int_sort());
    //assume(z3::forall(address,z3::forall(decl_id, z3::select(read_meta("ACCESSES", address,
    //                                    z3_ctx.array_sort(z3_ctx.int_sort(), z3_ctx.array_sort(z3_ctx.int_sort(), z3_ctx.bool_sort()))), decl_id)
    //== z3::empty_set(z3_ctx.int_sort()))));
    //assume(z3::forall(address, read_meta("RESTRICT_WRITES", address, z3_ctx.bool_sort()) == z3_ctx.bool_val(false)));
}

namespace {
    const clang::ValueDecl* get_value_decl(const clang::Expr* expr) {
        if (clang::isa<clang::DeclRefExpr>(expr)) {
            return clang::cast<clang::DeclRefExpr>(expr)->getDecl();
        } else if (clang::isa<clang::UnaryOperator>(expr)) {
            const auto* unary_operator = clang::cast<clang::UnaryOperator>(expr);
            if (unary_operator->isIncrementDecrementOp()) {
                return get_value_decl(unary_operator->getSubExpr());
            }
        } else if (clang::isa<clang::BinaryOperator>(expr)) {
            const auto* binary_operator = clang::cast<clang::BinaryOperator>(expr);
            if (binary_operator->isAssignmentOp()) {
                return get_value_decl(binary_operator->getLHS());
            } else if (binary_operator->isAdditiveOp()) {
                if (binary_operator->getLHS()->getType()->isPointerType()) {
                    if (!binary_operator->getRHS()->getType()->isPointerType()) {
                        return get_value_decl(binary_operator->getLHS());
                    }
                } else if (binary_operator->getRHS()->getType()->isPointerType()) {
                    return get_value_decl(binary_operator->getRHS());
                }
            }
        } else if (clang::isa<clang::ParenExpr>(expr)) {
            return get_value_decl(clang::cast<clang::ParenExpr>(expr)->getSubExpr());
        } else if (clang::isa<clang::ImplicitCastExpr>(expr)) {
            return get_value_decl(clang::cast<clang::ImplicitCastExpr>(expr)->getSubExpr());
        }
        return nullptr;
    }

    void insert_inner_blocks(std::vector<const clsma::block*>& vector, const clsma::block* block) {
        vector.push_back(block);
        std::for_each(block->inner_begin(), block->inner_end(), [&](const clsma::block* inner) {
            insert_inner_blocks(vector, inner);
        });
    }
}

std::optional<clsma::violation> restrict_checker::check_memory_access(const clsma::block* const block,
                                                                      const clang::Expr* const expr,
                                                                      const abstract_checker::memory_access_type access_type,
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
    if (value_decl == nullptr) {
        return std::nullopt;
    }
    const auto* var = block->get_var(value_decl);
    const auto access_data = memory_access_data {
        expr, access_type, address, var
    };
    accesses[block].emplace_back(access_data);
    //write_access(block, address, variable);

    //write_meta("ACCESSES", address, z3::set_add(read_meta("ACCESSES", address, z3_ctx.array_sort(z3_ctx.int_sort(), z3_ctx.bool_sort())), z3_ctx.int_val(value_decl->getID())));
    if (value_decl->getType().isRestrictQualified() && access_type == WRITE) {
        restrict_writes[block].emplace_back(access_data);
        //write_restrict_write(block, address, variable);
        //restrict_writes[variable->block].emplace(variable);

        std::vector<const clsma::block*> affected_blocks;
        insert_inner_blocks(affected_blocks, var->block);

        z3::expr violation_var_id = z3_ctx.int_const("ID");
        z3::expr condition = violation_var_id == 0;
        std::vector<const memory_access_data*> other_accesses;
        for (const auto* affected_block : affected_blocks) {
            const auto affected_block_assumption = affected_block->get_assumption();
            for (const auto& other_access_data : accesses[affected_block]) {
                if (other_access_data.var != var) {
                    other_accesses.emplace_back(&other_access_data);
                    auto access_condition = other_access_data.address == address;
                    if (affected_block_assumption.has_value()) {
                        access_condition = access_condition & affected_block_assumption.value();
                    }
                    condition = z3::ite(access_condition, violation_var_id == z3_ctx.int_val(uint64_t(other_accesses.size())), condition);
                }
            }
        }
        //const auto block_assumption = block->get_assumption();
        //if (block_assumption.has_value()) {
        //    condition = block_assumption.value() && condition;
        //}
        if (const auto result = check(block, condition); result.has_value()) {
            const auto id = result.value().eval(violation_var_id).as_uint64();
            if (id != 0) {
                const uint64_t var_address = result.value().eval(var->to_z3_expr()).as_uint64();
                const uint64_t address_value = result.value().eval(address).as_uint64();
                const auto* other_access_data = other_accesses[id - 1];
                const uint64_t other_var_address = result.value().eval(other_access_data->var->to_z3_expr()).as_uint64();
                //std::cout << "BUG PRESENT!" << std::endl;
                //std::cout << "this variable name: " << variable->decl->getName().str() << std::endl;
                //std::cout << "variable name: " << other_access_data->variable->decl->getName().str() << std::endl;
                std::ostringstream message;
                message << "write through `" << var->decl->getName().str()
                << "` (offset " << address_value - var_address << ") clashes with access through `"
                << other_access_data->var->decl->getName().str()
                << "` (offset " << address_value - other_var_address << ") at "
                << other_access_data->expr->getExprLoc().printToString(get_source_manager()) << std::endl;
                return clsma::violation {
                    .location = expr->getExprLoc(),
                    .message = message.str()
                };
            }
            //const auto id = result.optional_value().eval(z3_ctx.constant("ID", z3_ctx.int_sort())).as_int64();
            //std::cout << id << std::endl;
            //std::cout << "this variable name: " << variable->decl->getName().str() << std::endl;
            //std::cout << "variable name: " << block->get_var(id)->decl->getName().str() << std::endl;
            //std::cout << "address: " << result.optional_value().eval(address).as_int64() << std::endl;
        }
        /*const z3::expr other_id = z3_ctx.constant("ID", z3_ctx.int_sort());
        z3::expr condition = z3_ctx.bool_val(false);
        for (const auto* affected_block : affected_blocks) {
            condition = condition ||
                    other_id != z3_ctx.int_val(value_decl->getID()) && z3::set_member(other_id, read_accesses(affected_block, address));
        }
        const auto result = check(condition);
        if (result.has_value()) {
            const auto id = result.optional_value().eval(z3_ctx.constant("ID", z3_ctx.int_sort())).as_int64();
            std::cout << id << std::endl;
            std::cout << "this variable name: " << variable->decl->getName().str() << std::endl;
            std::cout << "variable name: " << block->get_var(id)->decl->getName().str() << std::endl;
            std::cout << "address: " << result.optional_value().eval(address).as_int64() << std::endl;
        }*/

        /*write_meta("RESTRICT_WRITES", address, z3_ctx.bool_val(true));
        const auto ch = z3_ctx.constant("ID", z3_ctx.int_sort()) != z3_ctx.int_val(value_decl->getID()) &&
                z3::set_member(z3_ctx.constant("ID", z3_ctx.int_sort()), read_meta("ACCESSES", address, z3_ctx.array_sort(z3_ctx.int_sort(), z3_ctx.bool_sort())));
        const auto result = check(ch);
        if (result.has_value()) {
            const auto id = result.optional_value().eval(z3_ctx.constant("ID", z3_ctx.int_sort())).as_int64();
            std::cout << id << std::endl;
        }*/
    } else {
        std::unordered_set<const clsma::block*> affected_blocks;
        for (auto cur_block = block; cur_block != nullptr; cur_block = cur_block->parent) {
            affected_blocks.emplace(cur_block);
        }

        z3::expr violation_var_id = z3_ctx.int_const("ID");
        z3::expr condition = violation_var_id == 0;
        std::vector<const memory_access_data*> other_accesses;
        for (const auto* affected_block : affected_blocks) {
            for (const auto& other_access_data : restrict_writes[affected_block]) {
                if (other_access_data.var != var) {
                    other_accesses.emplace_back(&other_access_data);
                    condition = z3::ite(other_access_data.address == address, violation_var_id == z3_ctx.int_val(uint64_t(other_accesses.size())), condition);
                }
            }
        }

        /*std::unordered_set<const class block*> affected_blocks;
        for (auto cur_block = block; cur_block != nullptr; cur_block = cur_block->parent) {
            affected_blocks.emplace(cur_block);
        }

        const z3::expr other_id = z3_ctx.constant("ID", z3_ctx.int_sort());
        z3::expr condition = z3_ctx.bool_val(false);
        for (const auto* affected_block : affected_blocks) {
            condition = condition ||
                        other_id != z3_ctx.int_val(value_decl->getID()) && z3::set_member(other_id, read_restrict_writes(affected_block, address));
        }
        const auto result = check(condition);
        if (result.has_value()) {
            const auto id = result.optional_value().eval(z3_ctx.constant("ID", z3_ctx.int_sort())).as_int64();
            std::cout << id << std::endl;
        }*/
    }

    //std::vector<const class variable*> restrict_vars;
    //for (const auto& [var_block, vars] : restrict_writes) {
        /*auto var_block = variable->block;
        while (var_block && var_block != block) {
            var_block = var_block->parent;
        }
        if (!var_block) {
            continue;
        }*/
   //     if (affected_blocks.count(var_block)) {
    //        restrict_vars.insert(restrict_vars.end(), vars.begin(), vars.end());
   //     }
   // }

   // const auto decl_id = z3_ctx.constant("DECL_ID", z3_ctx.int_sort());

    // const access_block = block_by_decl(value_decl);
    // for block : restrict_writes_blocks
    //    all_decls = get_all_decls_in_block
    //
    //
    //
    //check(!z3::implies(read_meta("RESTRICT_WRITES", address, z3_ctx.bool_sort()) == true, false));
    //check(z3::implies(read_meta("r", address, ctx.bool_sort()), z3::se));
    return std::nullopt;
}

namespace {
    std::string blockd(const std::string& meta, const clsma::block* block) {
        return meta + '#' + std::to_string(block->id);
    }
}

z3::expr restrict_checker::read_accesses(const clsma::block* block, const z3::expr& address) {
    if (accessed_blocks.insert(block).second) {
        const auto any_address = z3_ctx.constant("address", z3_ctx.int_sort());
        assume(z3::forall(any_address, read_accesses(block, any_address) == z3::empty_set(z3_ctx.int_sort())));
    }
    return read_meta(blockd("ACCESSES", block), address, z3_ctx.array_sort(z3_ctx.int_sort(), z3_ctx.bool_sort()));
}

void restrict_checker::write_access(const clsma::block* block, const z3::expr& address, const clsma::variable* var) {
    write_meta(blockd("ACCESSES", block), address, z3::set_add(read_accesses(block, address), z3_ctx.int_val(var->decl->getID())));
}

z3::expr restrict_checker::read_restrict_writes(const clsma::block* block, const z3::expr& address) {
    if (restrict_written_blocks.insert(block).second) {
        assume(read_meta(blockd("RESTRICT_WRITES", block), z3_ctx.array_sort(z3_ctx.int_sort(), z3_ctx.bool_sort())) == z3::const_array(z3_ctx.int_sort(), z3::empty_set(z3_ctx.int_sort())));
        //const auto any_address = z3_ctx.constant("address", z3_ctx.int_sort());
        //const auto any_decl_id = z3_ctx.constant("decl_id", z3_ctx.int_sort());
        //assume(z3::forall(any_address, z3::forall(any_decl_id, !z3::set_member(any_decl_id, read_restrict_writes(block, any_address)))));
    }
    return read_meta(blockd("RESTRICT_WRITES", block), address, z3_ctx.array_sort(z3_ctx.int_sort(), z3_ctx.bool_sort()));
}

void restrict_checker::write_restrict_write(const clsma::block* block, const z3::expr& address, const clsma::variable* var) {
    write_meta(blockd("RESTRICT_WRITES", block), address, z3::set_add(read_restrict_writes(block, address), z3_ctx.int_val(var->decl->getID())));
}
