#ifndef OPENCL_RESTRICT_ANALYZER_ABSTRACT_CHECKER_H
#define OPENCL_RESTRICT_ANALYZER_ABSTRACT_CHECKER_H


#include <vector>

#include <clang/AST/Expr.h>

#include <z3++.h>

#include "../model/memory.h"
#include "../analyzer_context.h"

class abstract_checker {
public:
    enum memory_access_type {
        READ,
        WRITE
    };

    explicit abstract_checker(analyzer_context& ctx);
    virtual ~abstract_checker() = default;
    virtual void check_memory_access(const scope* scope, const clang::Expr* expr, memory_access_type access_type, const z3::expr& address) = 0;
protected:
    z3::context& z3_ctx;
    const scope_context& scope_ctx;

    void write_meta(std::string_view meta, const z3::expr& address, const z3::expr& value);
    [[nodiscard]] z3::expr read_meta(std::string_view meta, const z3::expr& address, const z3::sort& sort) const;
    [[nodiscard]] z3::expr read_meta(std::string_view meta, const z3::sort& sort) const;

    void assume(const z3::expr& assumption);
    std::optional<z3::model> check(const z3::expr& assumption);
private:
    analyzer_context& ctx;
    z3::expr_vector assumptions;
};


#endif //OPENCL_RESTRICT_ANALYZER_ABSTRACT_CHECKER_H
