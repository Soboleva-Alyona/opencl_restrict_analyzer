#ifndef OPENCL_RESTRICT_ANALYZER_SCOPE_H
#define OPENCL_RESTRICT_ANALYZER_SCOPE_H


#include <optional>
#include <string_view>
#include <unordered_map>
#include <unordered_set>

#include <clang/AST/ASTContext.h>
#include <clang/AST/Decl.h>

#include <z3++.h>

#include "memory.h"
#include "var.h"

class scope;

class scope_context {
    friend class scope;
public:
    scope_context(clang::ASTContext const& ast_ctx, z3::context& z3_ctx, memory& mem);

    [[nodiscard]] scope* make_scope();

    [[nodiscard]] const scope* get_scope(int64_t scope_id) const;
    [[nodiscard]] const var* get_var(const clang::ValueDecl* decl) const;
    [[nodiscard]] const var* get_var(int64_t decl_id) const;
private:
    [[nodiscard]] scope* make_scope(scope* parent);

    clang::ASTContext const& ast_ctx;
    z3::context& z3_ctx;
    memory& mem;
    uint64_t next_id = 1;
    uint64_t mem_ptr = 1;

    std::unordered_map<int64_t, scope> scopes;
    std::unordered_map<int64_t, var> variables;
};

class scope {
    friend class scope_context;
public:
    [[nodiscard]] scope* make_inner();

    [[nodiscard]] const scope* const* inner_begin() const;
    [[nodiscard]] const scope* const* inner_end() const;

    var* decl_var(const clang::VarDecl* decl);
    [[nodiscard]] const var* get_var(const clang::ValueDecl* decl) const;
    [[nodiscard]] const var* get_var(int64_t decl_id) const;

    const scope* const parent;
    const uint64_t id;
private:
    explicit scope(scope_context& ctx);
    explicit scope(scope* parent);

    scope_context& ctx;

    std::vector<scope*> children;
    std::unordered_set<int64_t> owned_variables;

    [[nodiscard]] bool is_owned(int64_t decl_id) const;
};


#endif //OPENCL_RESTRICT_ANALYZER_SCOPE_H
