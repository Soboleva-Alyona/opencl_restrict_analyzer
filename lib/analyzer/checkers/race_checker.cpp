#include "race_checker.h"

#include <clang/AST/Expr.h>

#include "../core/ast_visitor.h"


namespace {

    const std::string global_memory_qualifier = "global";
    const std::string local_memory_qualifier = "local";
    const std::string read_only_memory_qualifier = "read_only";

    bool is_memory_private_or_readonly(const clang::Expr *const expr)
    {
        const auto type = expr->getType();
        if (type.getUnqualifiedType().getAsString().find(global_memory_qualifier) == std::string::npos
            && type.getUnqualifiedType().getAsString().find(local_memory_qualifier) == std::string::npos
            && type.getQualifiers().getAsString().find(global_memory_qualifier) == std::string::npos
            && type.getQualifiers().getAsString().find(local_memory_qualifier) == std::string::npos
            && type.getQualifiers().getAsString().find(read_only_memory_qualifier) == std::string::npos)
       {
            return true;
        }
        return false;
    }

    bool is_memory_global(const clang::Expr *const expr)
    {
        const auto type = expr->getType();
        if (type.getUnqualifiedType().getAsString().find(global_memory_qualifier) != std::string::npos
            || type.getQualifiers().getAsString().find(global_memory_qualifier) != std::string::npos)
       {
            return true;
        }
        return false;
    }

}

std::optional<clsa::violation> clsa::race_checker::check_inside_of_warp(const clsa::block *const block,
                                                                        const clang::Expr *const expr,
                                                                        const clsa::memory_access_type access_type,
                                                                        const z3::expr &address,
                                                                        const clsa::optional_value& value,
                                                                        const clsa::optional_value& value_copy,
                                                                        const z3::expr& address_copy) {
    if (access_type != clsa::write)
    {
        return std::nullopt;
    }

    // check lhs
    // if similar for all threads (but rhs can be different for two different threads) - it's race
    //          1) arraySubExps - index is integer literal or some combination of local_id functions (a[get_local_id(0)] = 0)
    //          2) any variable (consider it's __local or __global)

    const clang::ValueDecl* value_decl = get_pointer_decl(expr);
    const clsa::variable* var = block->var_get(value_decl);

    if (clang::isa<clang::DeclRefExpr>(expr))
    {
        std::ostringstream message;
        message << "Possible write/write race inside of warp discovered through access to variable`" << var->decl->getName().str()
                << "` at " << expr->getExprLoc().printToString(get_source_manager()) << std::endl;

        z3::expr different_values_written = value.value() != value_copy.value();

        if (const std::optional<z3::model> result = check(block, different_values_written); result.has_value())
        {
            return clsa::violation {
                .location = expr->getExprLoc(),
                .message = message.str()
            };
        }
    }

    if (clang::isa<clang::ArraySubscriptExpr>(expr))
    {
        z3::expr race_condition = address_copy == address;
        if (value.has_value() && value_copy.has_value())
        {
            race_condition = z3::ite(race_condition,
                                     race_condition && (value.value() != value_copy.value()), race_condition);
        }
        std::ostringstream message;
        message << "Possible write/write race inside of warp discovered through the access to array`" << var->decl->getName().str()
            << "` at " << expr->getExprLoc().printToString(get_source_manager()) << std::endl;

        if (const std::optional<z3::model> result = check(block, race_condition); result.has_value())
        {
            const z3::expr size = result->eval(var->to_z3_expr(clsa::VAR_META_MEM_SIZE));
            return clsa::violation {
                .location = expr->getExprLoc(),
                .message = message.str()
            };
        }
    }

    return std::nullopt;
}


void clsa::race_checker::fill_accesses(const clsa::memory_access_type access_type, const clsa::race_checker::memory_access_data_race_condition& access, const bool& is_global_space_mem)
{
    if (is_global_space_mem)
    {
        global_memory_accesses.emplace_back(access);
    } else
    {
        local_memory_accesses.emplace_back(access);
    }
    if (access_type == clsa::write) {
        if (is_global_space_mem)
        {
            global_writes.emplace_back(access);
        } else
        {
            local_writes.emplace_back(access);
        }
    }
    if (access_type == clsa::read_image)
    {
        image_memory_accesses.emplace_back(access);
    } else if (access_type == clsa::write_image)
    {
        image_memory_writes.emplace_back(access);
    }
}

void clsa::race_checker::get_workgroup_race_message(const clang::Expr* const expr, const clsa::memory_access_type access_type, const clsa::variable* var, const std::vector<clsa::race_checker::memory_access_data_race_condition>::value_type& other_access, std::ostringstream& message)
{
    message << "Possible " << (access_type == clsa::read ? "read" : "write")
        << "/" << (other_access.access_type == clsa::read ? "read" : "write")
        << " race inside workgroup discovered through access to `" << var->decl->getName().str()
        << "` at " << expr->getExprLoc().printToString(get_source_manager())
        << " conflicting with " << other_access.expr->getExprLoc().printToString(get_source_manager()) << std::endl;
}

std::optional<clsa::violation> clsa::race_checker::check_memory_access(const clsa::block* const block,
                                                                       const clang::Expr* const expr,
                                                                       const clsa::memory_access_type access_type,
                                                                       const z3::expr& address,
                                                                       const clsa::optional_value& value,
                                                                       const clsa::optional_value& value_copy,
                                                                       const z3::expr& address_copy) {
    // private memory is only visible to a worker, constant is read-only => no races
    if (is_memory_private_or_readonly(expr))
    {
        return std::nullopt;
    }
    // check race condition inside of a warp
    std::optional<violation> warp_race_violation = check_inside_of_warp(block, expr, access_type, address, value, value_copy, address_copy);

    // check race condition inside of a workgroup
    const clang::ValueDecl* value_decl = get_pointer_decl(expr);
    const clsa::variable* var = block->var_get(value_decl);
    if (nullptr == var) {
        return warp_race_violation;
    }
    const auto access = memory_access_data_race_condition {
        expr, access_type, address, var, address_copy
    };

    const bool is_global_space_mem = is_memory_global(expr);
    auto& other_accesses =
        access_type == clsa::read_image ? image_memory_writes
            : access_type == clsa::write_image ? image_memory_accesses
            : access_type == clsa::read ? (is_global_space_mem ? global_writes : local_writes)
            : is_global_space_mem ? global_memory_accesses : local_memory_accesses;

    z3::expr violation_access_idx = z3_ctx.int_const("IDX");
    z3::expr race_condition = violation_access_idx == 0;
    for (const auto& other_access : other_accesses)
    {
        const auto other_address = other_access.address;
        const auto other_address_copy = other_access.address_copy;

        race_condition = z3::ite(address == other_address_copy || address_copy == other_address,
            violation_access_idx == z3_ctx.int_val(uint64_t(other_accesses.size())), race_condition);
    }

    if (warp_race_violation != std::nullopt)
    {
        return warp_race_violation;
    }
    if (const std::optional<z3::model> result = check(block, race_condition); result.has_value())
    {
        const std::int64_t id = result.value().eval(violation_access_idx).get_numeral_int64();
        if (id != 0)
        {
            const auto&  other_access = other_accesses[id - 1];
            std::ostringstream message;
            get_workgroup_race_message(expr, access_type, var, other_access, message);

            return clsa::violation {
                .location = expr->getExprLoc(),
                .message = message.str()
            };
        }
    }

    fill_accesses(access_type, access, is_global_space_mem);
    return std::nullopt;

}

void clsa::race_checker::sync_global_memory()
{
    global_memory_accesses.clear();
    global_writes.clear();
}

void clsa::race_checker::sync_local_memory()
{
    local_memory_accesses.clear();
    local_writes.clear();
}

void clsa::race_checker::sync_image_memory()
{
    image_memory_accesses.clear();
}

