#ifndef OPENCL_RESTRICT_ANALYZER_VAR_H
#define OPENCL_RESTRICT_ANALYZER_VAR_H


#include <cstdlib>
#include <string>

#include <clang/AST/Decl.h>
#include <clang/AST/Type.h>

#include <z3++.h>

class scope;

class var {
    friend class scope;
private:
    var(const scope* scope, const clang::VarDecl*, const clang::QualType& type, uint64_t size, uint64_t address);
public:
    const scope* const scope;
    const clang::VarDecl* const decl;
    const clang::QualType type;
    const uint64_t size;
    const uint64_t address;

    z3::expr to_z3_expr(z3::context& z3_ctx) const override;
private:
    uint64_t version = 0;
};


#endif //OPENCL_RESTRICT_ANALYZER_VAR_H
