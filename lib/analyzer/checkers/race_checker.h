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

        void sync_image_memory();

    private:
        struct memory_access_data_race_condition {
            const clang::Expr* const expr;
            const clsa::memory_access_type access_type;
            const z3::expr address;
            const clsa::variable* const var;
            const z3::expr address_copy;
        };

        std::optional<clsa::violation> check_inside_of_warp(const clsa::block * block,
                                                            const clang::Expr * expr,
                                                            clsa::memory_access_type access_type,
                                                            const z3::expr &address,
                                                            const clsa::optional_value& value,
                                                            const clsa::optional_value& value_copy, const z3::expr& address_copy);

        void fill_accesses(clsa::memory_access_type access_type,
                           const memory_access_data_race_condition& access,
                           const bool& is_global_space_mem);

        void get_workgroup_race_message(const clang::Expr* expr,
                                        clsa::memory_access_type access_type,
                                        const clsa::variable* var,
                                        const std::vector<clsa::race_checker::memory_access_data_race_condition>::value_type& other_access,
                                        std::ostringstream& message);

        std::vector<memory_access_data_race_condition> local_memory_accesses = {};
        std::vector<memory_access_data_race_condition> local_writes = {};

        std::vector<memory_access_data_race_condition> global_memory_accesses = {};
        std::vector<memory_access_data_race_condition> global_writes = {};

        std::vector<memory_access_data_race_condition> image_memory_accesses = {};
        std::vector<memory_access_data_race_condition> image_memory_writes = {};
    };
}


#endif // OPENCL_RESTRICT_ANALYZER_RACE_CHECKER_H