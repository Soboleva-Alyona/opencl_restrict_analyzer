#ifndef OPENCL_RESTRICT_ANALYZER_block_H
#define OPENCL_RESTRICT_ANALYZER_block_H


#include <optional>
#include <string_view>
#include <unordered_map>
#include <unordered_set>

#include <clang/AST/ASTContext.h>
#include <clang/AST/Decl.h>

#include <z3++.h>

#include "optional_value.h"

namespace clsa {

    class block;

    class variable;

    class value_reference {
        friend class clsa::block;

    public:
        [[nodiscard]] z3::expr to_z3_expr() const;

        [[nodiscard]] z3::expr to_z3_expr(std::string_view key) const;

        [[nodiscard]] clsa::optional_value to_value() const;

        const clsa::block* const block;
    protected:
        value_reference(const clsa::block* block, std::string unique_id, std::string name, const z3::sort& sort);

        [[nodiscard]] clsa::variable* as_variable();

        [[nodiscard]] virtual const clsa::variable* as_variable() const;

        const std::string unique_id;
        const std::string name;
        const z3::sort sort;
        std::uint64_t version = 0;
    private:
        std::unordered_set<std::string> meta_keys;
    };

    class variable : public value_reference {
        friend class block;

    private:
        variable(const clsa::block* block, const clang::VarDecl*, const clang::QualType& type);

    protected:
        [[nodiscard]] const clsa::variable* as_variable() const final;

    public:
        const clang::VarDecl* const decl;
        const clang::QualType type;
    };

    class block_context;

    class block {
        friend class clsa::block_context;

        friend class clsa::variable;

        friend class clsa::value_reference;

    public:
        [[nodiscard]] clsa::block* make_inner();

        [[nodiscard]] clsa::block* make_inner(std::optional<z3::expr> precondition);

        /** Joins states of all not yet joined inner blocks with a precondition into the state of this block. */
        void join();

        [[nodiscard]] const clsa::block* const* inner_begin() const;

        [[nodiscard]] const clsa::block* const* inner_end() const;

        void write(const z3::expr& address, const z3::expr& value);

        [[nodiscard]] z3::expr read(const z3::expr& address, const clang::QualType& type);

        [[nodiscard]] z3::expr read(const z3::expr& address, const z3::sort& sort);

        clsa::value_reference* value_decl(std::string name, const clang::QualType& type);

        clsa::value_reference* value_decl(std::string name, const z3::sort& sort);

        void value_set(clsa::value_reference* value_ref, const clsa::optional_value& value);

        clsa::variable* var_decl(const clang::VarDecl* decl, const clsa::optional_value& value = clsa::optional_value());

        clsa::variable* var_set(const clang::ValueDecl* decl, const clsa::optional_value& value);

        clsa::variable* var_set(std::int64_t decl_id, const clsa::optional_value& value);

        [[nodiscard]] clsa::variable* var_get(const clang::ValueDecl* decl);

        [[nodiscard]] const clsa::variable* var_get(const clang::ValueDecl* decl) const;

        [[nodiscard]] clsa::variable* var_get(std::int64_t decl_id);

        [[nodiscard]] const clsa::variable* var_get(std::int64_t decl_id) const;

        void assume(const z3::expr& assumption);

        [[nodiscard]] z3::check_result check(const z3::expr& assumption) const;

        [[nodiscard]] std::optional<z3::expr> get_assumption() const;

        clsa::block* const parent;
        const std::uint64_t id;

    private:
        explicit block(clsa::block_context& ctx);

        block(clsa::block* parent, std::optional<z3::expr> precondition);

        [[nodiscard]] std::optional<z3::expr> get_assumption(const clsa::block* up_to) const;

        [[nodiscard]] clsa::value_reference* get(const std::string& unique_id);

        [[nodiscard]] const clsa::value_reference* get(const std::string& unique_id) const;

        clsa::value_reference* set(const std::string& unique_id, const clsa::optional_value& value);

        clsa::block_context& ctx;

        const std::optional<z3::expr> precondition = {};
        std::optional<z3::expr> assumptions = {};

        std::vector<clsa::block*> children = {};
        std::unordered_map<std::string, std::unique_ptr<clsa::value_reference>> variables = {};

        std::vector<clsa::block*> forks = {};
        std::unordered_set<std::string> forked_decl_ids = {};
    };

    class block_context {
        friend class clsa::block;

        friend class clsa::variable;

        friend class clsa::value_reference;

    public:
        explicit block_context(z3::solver& solver);

        [[nodiscard]] clsa::block* make_block();

        std::uint64_t allocate(std::uint64_t size);

    private:
        [[nodiscard]] clsa::block* make_block(clsa::block* parent, std::optional<z3::expr> precondition);

        z3::solver& solver;
        z3::context& z3;

        std::uint64_t next_id = 1;
        std::uint64_t versioned_values = 1;
        std::uint64_t mem_ptr = 1;

        std::unordered_map<int64_t, block> blocks = {};
    };
}


#endif //OPENCL_RESTRICT_ANALYZER_block_H
