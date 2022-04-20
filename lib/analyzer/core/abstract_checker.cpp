#include "abstract_checker.h"

clsma::abstract_checker::abstract_checker(analyzer_context& ctx) : ctx(ctx), z3_ctx(ctx.z3) {}

std::optional<z3::model> clsma::abstract_checker::check(const clsma::block* block, const z3::expr& assumption) const {
    const bool result = block->check(assumption) == z3::check_result::sat;
    return result ? std::make_optional(ctx.solver.get_model()) : std::nullopt;
}

const clang::SourceManager& clsma::abstract_checker::get_source_manager() const {
    return ctx.ast.getSourceManager();
}

const clang::ValueDecl* clsma::abstract_checker::get_value_decl(const clang::Expr* expr) {
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

const clang::ValueDecl* clsma::abstract_checker::get_pointer_decl(const clang::Expr* expr) {
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
