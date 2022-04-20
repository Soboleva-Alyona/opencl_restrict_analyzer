#ifndef OPENCL_RESTRICT_ANALYZER_ABSTRACT_CHECKER_H
#define OPENCL_RESTRICT_ANALYZER_ABSTRACT_CHECKER_H


#include <vector>

#include <clang/AST/Expr.h>

#include <z3++.h>

#include "analyzer_context.h"
#include "memory_access_type.h"
#include "violation.h"

namespace clsma {

    class abstract_checker {
    public:
        explicit abstract_checker(clsma::analyzer_context& ctx);

        virtual ~abstract_checker() = default;

        virtual std::optional<clsma::violation> check_memory_access(const clsma::block* block, const clang::Expr* expr,
                                                                    clsma::memory_access_type access_type,
                                                                    const z3::expr& address) = 0;

    protected:
        z3::context& z3_ctx;

        std::optional<z3::model> check(const clsma::block* block, const z3::expr& assumption) const;

        [[nodiscard]] const clang::SourceManager& get_source_manager() const;

        [[nodiscard]] static const clang::ValueDecl* get_value_decl(const clang::Expr* expr);

        [[nodiscard]] static const clang::ValueDecl* get_pointer_decl(const clang::Expr* expr);

    private:
        clsma::analyzer_context& ctx;
    };

}

#endif //OPENCL_RESTRICT_ANALYZER_ABSTRACT_CHECKER_H
