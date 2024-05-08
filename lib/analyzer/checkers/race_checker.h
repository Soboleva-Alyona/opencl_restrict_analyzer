#ifndef OPENCL_RESTRICT_ANALYZER_RACE_CHECKER_H
#define OPENCL_RESTRICT_ANALYZER_RACE_CHECKER_H


#include "../core/abstract_checker.h"

namespace clsa {

    class race_checker : public clsa::abstract_checker {
    public:
        using abstract_checker::abstract_checker;

        std::optional<clsa::violation> check_memory_access(const clsa::block* block, const clang::Expr* expr,
                                                           clsa::memory_access_type access_type,
                                                           const z3::expr& address,
                                                           const clsa::optional_value& value,
                                                           const clsa::optional_value& value_copy, const z3::expr& address_copy) override;

    private:
        struct memory_access_data {
            const clang::Expr* const expr;
            const clsa::memory_access_type access_type;
            const z3::expr address;
            const clsa::variable* const var;
        };

        static const char* get_access_name(const memory_access_data& access);

        std::optional<clsa::violation> check_inside_of_warp(const clsa::block * block,
                                                            const clang::Expr * expr,
                                                            clsa::memory_access_type access_type,
                                                            const z3::expr &address,
                                                            const clsa::optional_value& value,
                                                            const clsa::optional_value& value_copy, const z3::expr& address_copy);

        std::unordered_map<const clsa::block*, std::vector<memory_access_data>> accesses = {};
        std::unordered_map<const clsa::block*, std::vector<memory_access_data>> writes = {};
    };
}


#endif // OPENCL_RESTRICT_ANALYZER_RACE_CHECKER_H