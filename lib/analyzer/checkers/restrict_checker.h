#ifndef OPENCL_RESTRICT_ANALYZER_RESTRICT_CHECKER_H
#define OPENCL_RESTRICT_ANALYZER_RESTRICT_CHECKER_H


#include "../core/abstract_checker.h"

namespace clsa {

    class restrict_checker : public clsa::abstract_checker {
    public:
        using abstract_checker::abstract_checker;

        std::optional<clsa::violation> check_memory_access(const clsa::block* block, const clang::Expr* expr,
                                                           clsa::memory_access_type access_type,
                                                           const z3::expr& address) override;

    private:
        struct memory_access_data {
            const clang::Expr* const expr;
            const clsa::memory_access_type access_type;
            const z3::expr address;
            const clsa::variable* const var;
        };

        static const char* get_access_name(const memory_access_data& access);

        std::unordered_map<const clsa::block*, std::vector<memory_access_data>> accesses = {};
        std::unordered_map<const clsa::block*, std::vector<memory_access_data>> accesses_restrict = {};
        std::unordered_map<const clsa::block*, std::vector<memory_access_data>> writes = {};
        std::unordered_map<const clsa::block*, std::vector<memory_access_data>> writes_restrict = {};
    };

}


#endif //OPENCL_RESTRICT_ANALYZER_RESTRICT_CHECKER_H
