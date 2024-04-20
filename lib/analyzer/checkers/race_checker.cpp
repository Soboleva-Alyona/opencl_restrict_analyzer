#include "race_checker.h"

#include <clang/AST/Expr.h>

#include "../core/ast_visitor.h"


namespace {

    const std::string global_memory_qualifier = "__global";
    const std::string local_memory_qualifier = "__local";


    bool is_memory_private_or_constant(const clang::Qualifiers qualifiers)
    {
        if (qualifiers.getAsString().find(global_memory_qualifier) != std::string::npos
            || qualifiers.getAsString().find(local_memory_qualifier) != std::string::npos)
        {
            return false;
        }
        return true;
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
                                                    const clsa::optional_value& value) {
    if (access_type != clsa::write)
    {
        return std::nullopt;
    }
    const clang::Qualifiers expression_type_qualifiers = expr->getType().getQualifiers();
    // private memory is only visible to a worker, constant is read-only => no write-write races inside of warp
    if (is_memory_private_or_constant(expression_type_qualifiers))
    {
        return std::nullopt;
    }

    // check lhs
    // if similar for all threads - it's race
    //          1) arraySubExps - index is integer literal or some combination of local_id functions (a[get_local_id(0)] = 0)
    //          2) any variable (consider it's __local or __global)

    // NOTE: now chechker detects even benign races like:
    // k = 3; - benign race
    const clang::ValueDecl* value_decl = get_pointer_decl(expr);
    const clsa::variable* var = block->var_get(value_decl);

    if (clang::isa<clang::DeclRefExpr>(expr))
    {
        std::ostringstream message;
        message << "Possible write/write race discovered through access to variable`" << var->decl->getName().str()
                << "` at " << expr->getExprLoc().printToString(get_source_manager()) << std::endl;

        return clsa::violation {
            .location = expr->getExprLoc(),
            .message = message.str()
        };
    }

    if (clang::isa<clang::ArraySubscriptExpr>(expr))
    {
        std::ostringstream message;
        message << "Possible write/write race discovered through the access to array`" << var->decl->getName().str()
                << "` at " << expr->getExprLoc().printToString(get_source_manager()) << std::endl;

        const auto* array_subscript_expr = clang::cast<clang::ArraySubscriptExpr>(expr);
        const auto* index_expr = array_subscript_expr->getIdx();

        if (clang::isa<clang::IntegerLiteral>(index_expr)) // TODO add check that num of workers > 1 (local_size)
        {
            return clsa::violation {
                .location = expr->getExprLoc(),
                .message = message.str()
            };
        }
        // local_size
        // size_of_array from parameters or declaration
        // std::optional<z3::expr> array_size = value.metadata(VAR_META_MEM_SIZE);
        // const auto array_size_expr = array_size.value();
        std::cout << "\n Type of expr: "<<expr->getType().getAsString() << '\n';

        std::cout << "\n Qualifiers: "<<expression_type_qualifiers.getAsString() << '\n';
        if (expression_type_qualifiers.getAsString().find("__global") != std::string::npos)
        {
            std::cout << "Is global memeory\n";
        }
        std::cout << "\n inner type: " << index_expr->getType().getAsString() << "\n";

        // iterate throw all the children of block recursively
        // block->get_children();


        return std::nullopt;
    }

    return std::nullopt;
}


std::optional<clsa::violation> clsa::race_checker::check_memory_access(const clsa::block* const block,
                                                                           const clang::Expr* const expr,
                                                                           const clsa::memory_access_type access_type,
                                                                           const z3::expr& address,
                                                                           const clsa::optional_value& value) {
    // inside of warp
    std::optional<violation> warp_race_violation = check_inside_of_warp(block, expr, access_type, address, value);
    if (warp_race_violation != std::nullopt)
    {
        return warp_race_violation;
    }
    // inside of a workgroup


    return std::nullopt;
}