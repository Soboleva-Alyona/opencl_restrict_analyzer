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

namespace clsma {

    class block;

    class variable;

    class value_reference {
        friend class clsma::block;

    public:
        [[nodiscard]] z3::expr to_z3_expr() const;

        [[nodiscard]] z3::expr to_z3_expr(std::string_view key) const;

        [[nodiscard]] clsma::optional_value to_value() const;

        const clsma::block* const block;
    protected:
        value_reference(const clsma::block* block, std::string unique_id, std::string name, const z3::sort& sort);

        [[nodiscard]] clsma::variable* as_variable();

        [[nodiscard]] virtual const clsma::variable* as_variable() const;

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
        variable(const clsma::block* block, const clang::VarDecl*, const clang::QualType& type, std::uint64_t size,
                 std::uint64_t address);

    protected:
        [[nodiscard]] const clsma::variable* as_variable() const final;

    public:
        const clang::VarDecl* const decl;
        const clang::QualType type;
        const std::uint64_t size;
        const std::uint64_t address;
    };

    class block_context;

    class block {
        friend class clsma::block_context;

        friend class clsma::variable;

        friend class clsma::value_reference;

    public:
        [[nodiscard]] clsma::block* make_inner();

        [[nodiscard]] clsma::block* make_inner(std::optional<z3::expr> precondition);

        /** Joins states of all not yet joined inner blocks with a precondition into the state of this block. */
        void join();

        [[nodiscard]] const clsma::block* const* inner_begin() const;

        [[nodiscard]] const clsma::block* const* inner_end() const;

        void write(const z3::expr& address, const z3::expr& value);

        [[nodiscard]] z3::expr read(const z3::expr& address, const clang::QualType& type);

        [[nodiscard]] z3::expr read(const z3::expr& address, const z3::sort& sort);

        clsma::value_reference* value_decl(std::string name, const clang::QualType& type);

        clsma::value_reference* value_decl(std::string name, const z3::sort& sort);

        void value_set(clsma::value_reference* value_ref, const clsma::optional_value& value);

        clsma::variable* var_decl(const clang::VarDecl* decl, const clsma::optional_value& value = clsma::optional_value());

        clsma::variable* var_set(const clang::ValueDecl* decl, const clsma::optional_value& value);

        clsma::variable* var_set(std::int64_t decl_id, const clsma::optional_value& value);

        [[nodiscard]] clsma::variable* var_get(const clang::ValueDecl* decl);

        [[nodiscard]] const clsma::variable* var_get(const clang::ValueDecl* decl) const;

        [[nodiscard]] clsma::variable* var_get(std::int64_t decl_id);

        [[nodiscard]] const clsma::variable* var_get(std::int64_t decl_id) const;

        void assume(const z3::expr& assumption);

        [[nodiscard]] std::optional<z3::expr> get_assumption() const;

        [[nodiscard]] z3::check_result check(const z3::expr& assumption) const;

        clsma::block* const parent;
        const std::uint64_t id;
    protected:
        [[nodiscard]] clsma::value_reference* get(const std::string& unique_id);

        [[nodiscard]] const clsma::value_reference* get(const std::string& unique_id) const;

        clsma::value_reference* set(const std::string& unique_id, const clsma::optional_value& value);

    private:
        explicit block(clsma::block_context& ctx);

        block(clsma::block* parent, std::optional<z3::expr> precondition);

        clsma::block_context& ctx;

        const std::optional<z3::expr> precondition = {};
        std::optional<z3::expr> assumptions = {};

        std::vector<clsma::block*> children = {};
        std::unordered_map<std::string, std::unique_ptr<clsma::value_reference>> variables = {};

        std::vector<clsma::block*> forks = {};
        std::unordered_set<std::string> forked_decl_ids = {};
    };

    class block_context {
        friend class clsma::block;

        friend class clsma::variable;

        friend class clsma::value_reference;

    public:
        explicit block_context(z3::solver& solver);

        [[nodiscard]] clsma::block* make_block();

        std::uint64_t allocate(std::uint64_t size);

    private:
        [[nodiscard]] clsma::block* make_block(clsma::block* parent, std::optional<z3::expr> precondition);

        z3::solver& solver;
        z3::context& z3;

        std::uint64_t next_id = 1;
        std::uint64_t versioned_values = 1;
        std::uint64_t mem_ptr = 1;

        std::unordered_map<int64_t, block> blocks = {};
    };
}


#endif //OPENCL_RESTRICT_ANALYZER_block_H
