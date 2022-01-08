#ifndef OPENCL_RESTRICT_ANALYZER_block_H
#define OPENCL_RESTRICT_ANALYZER_block_H


#include <optional>
#include <string_view>
#include <unordered_map>
#include <unordered_set>

#include <clang/AST/ASTContext.h>
#include <clang/AST/Decl.h>

#include <z3++.h>

#include "memory.h"
#include "var.h"

class block;

class var {
    friend class block;
private:
    var(const block* block, const clang::VarDecl*, const clang::QualType& type, uint64_t size, uint64_t address);
public:
    const block* const block;
    const clang::VarDecl* const decl;
    const clang::QualType type;
    const uint64_t size;
    const uint64_t address;

    void next_version();
    [[nodiscard]] z3::expr to_z3_expr() const;
private:
    uint64_t version = 0;
};

class block_context {
    friend class block;
    friend class var;
public:
    block_context(clang::ASTContext const& ast_ctx, z3::context& z3_ctx, memory& mem);

    [[nodiscard]] block* make_block();

    [[nodiscard]] const block* get_block(int64_t block_id) const;
private:
    [[nodiscard]] block* make_block(block* parent);

    clang::ASTContext const& ast_ctx;
    z3::context& z3_ctx;
    memory& mem;
    uint64_t next_id = 1;
    uint64_t mem_ptr = 1;

    std::unordered_map<int64_t, block> blocks;
};

class block {
    friend class block_context;
    friend class var;
public:
    [[nodiscard]] block* make_inner();

    [[nodiscard]] const block* const* inner_begin() const;
    [[nodiscard]] const block* const* inner_end() const;

    var* decl_var(const clang::VarDecl* decl);
    [[nodiscard]] var* get_var(const clang::ValueDecl* decl);
    [[nodiscard]] const var* get_var(const clang::ValueDecl* decl) const;
    [[nodiscard]] var* get_var(int64_t decl_id);
    [[nodiscard]] const var* get_var(int64_t decl_id) const;

    block* const parent;
    const uint64_t id;
private:
    explicit block(block_context& ctx);
    explicit block(block* parent);

    block_context& ctx;

    std::vector<block*> children;
    std::unordered_map<int64_t, var> variables;
};


#endif //OPENCL_RESTRICT_ANALYZER_block_H
