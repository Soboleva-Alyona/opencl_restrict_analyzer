#ifndef OPENCL_RESTRICT_ANALYZER_ADDRESS_CHECKER_H
#define OPENCL_RESTRICT_ANALYZER_ADDRESS_CHECKER_H


#include "../core/abstract_checker.h"

namespace clsa {

    class address_checker : public clsa::abstract_checker {
    public:
        using abstract_checker::abstract_checker;

        std::optional<clsa::violation> check_memory_access(const clsa::block *block, const clang::Expr *expr,
                                                            clsa::memory_access_type access_type,
                                                            const z3::expr &address) override;
    };

}


#endif //OPENCL_RESTRICT_ANALYZER_ADDRESS_CHECKER_H
