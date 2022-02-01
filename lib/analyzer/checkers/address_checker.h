#ifndef OPENCL_RESTRICT_ANALYZER_ADDRESS_CHECKER_H
#define OPENCL_RESTRICT_ANALYZER_ADDRESS_CHECKER_H


#include "abstract_checker.h"

class address_checker : public abstract_checker {
public:
    std::optional<clsma::violation> check_memory_access(const clsma::block* block, const clang::Expr* expr, memory_access_type access_type, const z3::expr& address) override;
};


#endif //OPENCL_RESTRICT_ANALYZER_ADDRESS_CHECKER_H
