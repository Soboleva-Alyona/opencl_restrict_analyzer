#ifndef OPENCL_RESTRICT_ANALYZER_ADDRESS_CHECKER_H
#define OPENCL_RESTRICT_ANALYZER_ADDRESS_CHECKER_H


#include "../core/abstract_checker.h"

namespace clsma {

    class address_checker : public clsma::abstract_checker {
    public:
        using abstract_checker::abstract_checker;

        std::optional<clsma::violation> check_memory_access(const clsma::block *block, const clang::Expr *expr,
                                                            clsma::memory_access_type access_type,
                                                            const z3::expr &address) override;
    };

}


#endif //OPENCL_RESTRICT_ANALYZER_ADDRESS_CHECKER_H
