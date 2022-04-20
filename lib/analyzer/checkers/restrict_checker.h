#ifndef OPENCL_RESTRICT_ANALYZER_RESTRICT_CHECKER_H
#define OPENCL_RESTRICT_ANALYZER_RESTRICT_CHECKER_H


#include "../core/abstract_checker.h"

namespace clsma {

    class restrict_checker : public clsma::abstract_checker {
    public:
        using abstract_checker::abstract_checker;

        std::optional<clsma::violation> check_memory_access(const clsma::block* block, const clang::Expr* expr,
                                                            clsma::memory_access_type access_type,
                                                            const z3::expr& address) override;

    private:
        struct memory_access_data {
            const clang::Expr* const expr;
            const z3::expr address;
            const clsma::variable* const var;
        };

        std::unordered_map<const clsma::block*, std::vector<memory_access_data>> accesses = {};
        std::unordered_map<const clsma::block*, std::vector<memory_access_data>> restrict_writes = {};
    };

}


#endif //OPENCL_RESTRICT_ANALYZER_RESTRICT_CHECKER_H
