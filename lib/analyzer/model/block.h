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
#include "optional_value.h"

namespace clsma {

class block;
class variable;

class value_reference {
    friend class block;
public:
    [[nodiscard]] z3::expr to_z3_expr() const;
    [[nodiscard]] z3::expr to_z3_expr(const std::string& key) const;

    [[nodiscard]] clsma::optional_value to_value() const;

    const clsma::block* const block;
protected:
    value_reference(const clsma::block* block, std::string unique_id, std::string name, z3::sort sort);

    [[nodiscard]] clsma::variable* as_variable();
    [[nodiscard]] virtual const clsma::variable* as_variable() const;

    const std::string unique_id;
    const std::string name;
    const z3::sort sort;
    uint64_t version = 0;
private:
    std::unordered_set<std::string> meta_keys;
};

class variable : public value_reference {
    friend class block;
private:
    variable(const clsma::block* block, const clang::VarDecl*, const clang::QualType& type, uint64_t size, uint64_t address);

protected:
    [[nodiscard]] const clsma::variable* as_variable() const final;

public:
    const clang::VarDecl* const decl;
    const clang::QualType type;
    const uint64_t size;
    const uint64_t address;

    //void next_version()

    [[nodiscard]] z3::expr to_z3_storage_expr() const;
};

class block_context {
    friend class block;

    friend class variable;
    friend class clsma::value_reference;
public:
    block_context(clang::ASTContext const& ast_ctx, z3::solver& solver, memory& mem);

    [[nodiscard]] block* make_block();

    [[nodiscard]] const block* get_block(int64_t block_id) const;

private:
    [[nodiscard]] block* make_block(block* parent, std::optional<z3::expr> precondition);

    clang::ASTContext const& ast_ctx;
    z3::solver& solver;
    z3::context& z3_ctx;
    memory& mem;
    uint64_t next_id = 1;
    uint64_t versioned_values = 1;
    uint64_t mem_ptr = 1;

    std::unordered_map<int64_t, block> blocks;
};

class block {
    friend class block_context;

    friend class variable;
    friend class clsma::value_reference;

public:
    [[nodiscard]] block* make_inner();

    [[nodiscard]] block* make_inner(z3::expr precondition);

    /** Joins states of all not yet joined inner blocks with a precondition into the state of this block. */
    void join();

    [[nodiscard]] const block* const* inner_begin() const;

    [[nodiscard]] const block* const* inner_end() const;

    void write(const z3::expr& address, const z3::expr& value);
    [[nodiscard]] z3::expr read(const z3::expr& address, const clang::QualType& type);
    [[nodiscard]] z3::expr read(const z3::expr& address, const z3::sort& sort);

    value_reference* value_decl(std::string name, const clang::QualType& type);
    void var_set(value_reference* var, const clsma::optional_value& value);

    variable* decl_var(const clang::VarDecl* decl, const clsma::optional_value& value = clsma::optional_value());

    variable* set_var(const clang::ValueDecl* decl, const clsma::optional_value& value);

    variable* set_var(const clang::ValueDecl* decl, const clsma::optional_value& value, const z3::expr& address);

    variable* set_var(int64_t decl_id, const clsma::optional_value& value);

    variable* set_var(int64_t decl_id, const clsma::optional_value& value, const z3::expr& address);

    [[nodiscard]] variable* get_var(const clang::ValueDecl* decl);

    [[nodiscard]] const variable* get_var(const clang::ValueDecl* decl) const;

    [[nodiscard]] variable* get_var(int64_t decl_id);

    [[nodiscard]] const variable* get_var(int64_t decl_id) const;

    void assume(const z3::expr& assumption);

    [[nodiscard]] z3::check_result check(const z3::expr& assumption) const;

    [[nodiscard]] std::optional<z3::expr> get_assumption() const;

    block* const parent;
    const uint64_t id;
protected:
    [[nodiscard]] clsma::value_reference* get(const std::string& unique_id);
    [[nodiscard]] const clsma::value_reference* get(const std::string& unique_id) const;
    clsma::value_reference* set(const std::string& unique_id, const clsma::optional_value& value);
private:
    explicit block(block_context& ctx);

    block(block* parent, std::optional<z3::expr> precondition);

    block_context& ctx;

    const std::optional<z3::expr> precondition;
    std::optional<z3::expr> assumptions;

    std::vector<block*> children;
    std::unordered_map<std::string, std::unique_ptr<value_reference>> variables;

    std::vector<block*> forks;
    std::unordered_set<std::string> forked_decl_ids;
};
}


#endif //OPENCL_RESTRICT_ANALYZER_block_H
