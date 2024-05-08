#include "race_checker.h"

#include <clang/AST/Expr.h>

#include "../core/ast_visitor.h"


namespace {

    const std::string global_memory_qualifier = "__global";
    const std::string local_memory_qualifier = "__local";

    bool is_memory_private_or_constant(const clang::Expr *const expr)
    {
        const auto type = expr->getType();
        if (type.getUnqualifiedType().getAsString().find(global_memory_qualifier) == std::string::npos
            && type.getUnqualifiedType().getAsString().find(local_memory_qualifier) == std::string::npos
            && type.getQualifiers().getAsString().find(global_memory_qualifier) == std::string::npos
            && type.getQualifiers().getAsString().find(local_memory_qualifier) == std::string::npos)
       {
            return true;
        }
        return false;
    }

    bool do_workers_intersect_for_array_access()
    {

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
    // private memory is only visible to a worker, constant is read-only => no write-write races inside of warp
    if (is_memory_private_or_constant(expr))
    {
        return std::nullopt;
    }

    // check lhs
    // if similar for all threads - it's race
    //          1) arraySubExps - index is integer literal or some combination of local_id functions (a[get_local_id(0)] = 0)
    //          2) any variable (consider it's __local or __global)

    const clang::ValueDecl* value_decl = get_pointer_decl(expr);
    const clsa::variable* var = block->var_get(value_decl);

    if (clang::isa<clang::DeclRefExpr>(expr))
    {
        std::ostringstream message;
        message << "Possible write/write race discovered through access to variable`" << var->decl->getName().str()
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
        std::ostringstream message;
        message << "Possible write/write race discovered through the access to array`" << var->decl->getName().str()
                << "` at " << expr->getExprLoc().printToString(get_source_manager()) << std::endl;

        z3::expr race_condition = address_copy == address;
        if (value.has_value() && value_copy.has_value())
        {
            race_condition = race_condition && (value.value() != value_copy.value());
        }

        if (const std::optional<z3::model> result = check(block, race_condition); result.has_value())
        {
            const std::int64_t var_value = result.value().eval(var->to_z3_expr()).get_numeral_int64();
            const std::int64_t address_value = result->eval(address).get_numeral_int64();
            const std::int64_t address_copy_value = result->eval(address_copy).get_numeral_int64();
            const std::int64_t base_address_value = result->eval(var->to_z3_expr(clsa::VAR_META_MEM_BASE)).get_numeral_int64();
            const z3::expr size = result->eval(var->to_z3_expr(clsa::VAR_META_MEM_SIZE));
            return clsa::violation {
                .location = expr->getExprLoc(),
                .message = message.str()
            };
        }
    }

    return std::nullopt;
}


std::optional<clsa::violation> clsa::race_checker::check_memory_access(const clsa::block* const block,
                                                                       const clang::Expr* const expr,
                                                                       const clsa::memory_access_type access_type,
                                                                       const z3::expr& address,
                                                                       const clsa::optional_value& value,
                                                                       const clsa::optional_value& value_copy,
                                                                       const z3::expr& address_copy) {
    // inside of warp
    std::optional<violation> warp_race_violation = check_inside_of_warp(block, expr, access_type, address, value, value_copy, address_copy);
    if (warp_race_violation != std::nullopt)
    {
        // todo - don't return (merge them, otherwise writes/accesses will not be kept)
        return warp_race_violation;
    }
    // inside of a workgroup


    return std::nullopt;
}