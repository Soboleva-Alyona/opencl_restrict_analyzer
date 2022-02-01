#include "abstract_checker.h"

abstract_checker::abstract_checker(analyzer_context& ctx)
: z3_ctx(ctx.z3_ctx), block_ctx(ctx.block_ctx), ctx(ctx), assumptions(z3_ctx) { }

void abstract_checker::write_meta(std::string_view meta, const z3::expr& address, const z3::expr& value) {
    ctx.mem.write_meta(meta, address, value);
}

z3::expr abstract_checker::read_meta(std::string_view meta, const z3::expr& address, const z3::sort& sort) const {
    return ctx.mem.read_meta(meta, address, sort);
}

z3::expr abstract_checker::read_meta(std::string_view meta, const z3::sort& sort) const {
    return ctx.mem.read_meta(meta, sort);
}

void abstract_checker::assume(const z3::expr& assumption) {
    assumptions.push_back(assumption);
}

std::optional<z3::model> abstract_checker::check(const clsma::block* block, const z3::expr& assumption) {
    //assumptions.push_back(assumption);
    const bool result = block->check(assumption) == z3::check_result::sat;
    //assumptions.pop_back();
    return result ? std::make_optional(ctx.solver.get_model()) : std::nullopt;
    /*const auto retval = result ? std::make_optional(ctx.solver.get_model()) : std::nullopt;
    if (const auto r2 = ctx.solver.check(assumptions); r2 != z3::check_result::sat) {
        std::cout << "ass broken" << std::endl;
        for (const auto& assertion : ctx.solver.assertions()) {
            std::cout << assertion.to_string() << std::endl;
        }
        for (const auto ass : assumptions) {
            std::cout << ass.to_string() << std::endl;
        }
        std::cout << "ass end" << std::endl;
        std::cout << "res: " << r2 << std::endl;
        std::cout << "reason: " << ctx.solver.reason_unknown() << std::endl;
        std::cout << ctx.solver.proof().to_string() << std::endl;
    }
    return retval;*/
}

const clang::SourceManager& abstract_checker::get_source_manager() const {
    return ctx.ast_ctx.getSourceManager();
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
}

const clang::ValueDecl* abstract_checker::get_pointer_decl(const clang::Expr* expr) {
    if (clang::isa<clang::ArraySubscriptExpr>(expr)) {
        const auto* array_subscript_expr = clang::cast<clang::ArraySubscriptExpr>(expr);
        return get_value_decl(array_subscript_expr->getBase());
    } else if (clang::isa<clang::UnaryOperator>(expr)) {
        const auto* unary_operator = clang::cast<clang::UnaryOperator>(expr);
        if (unary_operator->getOpcode() == clang::UO_Deref) {
            return get_value_decl(unary_operator->getSubExpr());
        }
    } else if (clang::isa<clang::ParenExpr>(expr)) {
        return get_pointer_decl(expr->IgnoreParens());
    } else if (clang::isa<clang::ImplicitCastExpr>(expr)) {
        return get_pointer_decl(expr->IgnoreImpCasts());
    }
    return nullptr;
}
