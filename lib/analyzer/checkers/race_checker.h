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

        void sync_local_memory();

        void sync_global_memory();

    private:
        struct memory_access_data_race_condition {
            const clang::Expr* const expr;
            const clsa::memory_access_type access_type;
            const z3::expr address;
            const clsa::variable* const var;
            const z3::expr address_copy;
        };

        static const char* get_access_name(const memory_access_data_race_condition& access);

        std::optional<clsa::violation> check_inside_of_warp(const clsa::block * block,
                                                            const clang::Expr * expr,
                                                            clsa::memory_access_type access_type,
                                                            const z3::expr &address,
                                                            const clsa::optional_value& value,
                                                            const clsa::optional_value& value_copy, const z3::expr& address_copy);
        void fill_accesses(clsa::memory_access_type access_type, memory_access_data_race_condition access, const bool& is_global_space_mem);

        std::vector<memory_access_data_race_condition> local_memory_accesses = {};
        std::vector<memory_access_data_race_condition> local_writes = {};

        std::vector<memory_access_data_race_condition> global_memory_accesses = {};
        std::vector<memory_access_data_race_condition> global_writes = {};
    };
}


#endif // OPENCL_RESTRICT_ANALYZER_RACE_CHECKER_H