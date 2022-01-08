#include "var.h"

namespace {
    z3::sort type_to_sort(z3::context& z3_ctx, const clang::QualType& type) {
        if (type->isIntegerType() || type->isPointerType()) {
            return z3_ctx.int_sort();
        } else if (type->isFloatingType()) {
            return z3_ctx.real_sort();
        } else if (type->isBooleanType()) {
            return z3_ctx.bool_sort();
        } else {
            return z3_ctx.uninterpreted_sort(type->getTypeClassName());
        }
    }
}

var::var(const class scope* scope, const clang::VarDecl* decl, const clang::QualType& type, uint64_t size, uint64_t address)
: scope(scope), decl(decl), type(type), size(size), address(address) {}

z3::expr var::to_z3_expr(z3::context& z3_ctx) const {
    std::string version_suffix;
    if (version > 0) {
        version_suffix = "!!" + std::to_string(version);
    }
    return z3_ctx.constant(("scope$" + decl->getName().str() + version_suffix).c_str(), type_to_sort(z3_ctx, type));
}
