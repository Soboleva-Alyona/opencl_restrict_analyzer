#ifndef OPENCL_RESTRICT_ANALYZER_RESTRICT_CHECKER_H
#define OPENCL_RESTRICT_ANALYZER_RESTRICT_CHECKER_H


#include "abstract_checker.h"

class restrict_checker : public abstract_checker {
public:
    explicit restrict_checker(analyzer_context& ctx);

    std::optional<clsma::violation> check_memory_access(const clsma::block* block, const clang::Expr* expr, memory_access_type access_type, const z3::expr& address) override;
private:
    struct memory_access_data {
        const clang::Expr* const expr;
        const memory_access_type access_type;
        const z3::expr address;
        const clsma::variable* const var;
    };

    std::unordered_set<const clsma::block*> accessed_blocks;
    std::unordered_set<const clsma::block*> restrict_written_blocks;

    std::unordered_map<const clsma::block*, std::vector<memory_access_data>> accesses;
    std::unordered_map<const clsma::block*, std::vector<memory_access_data>> restrict_writes;

    [[nodiscard]] z3::expr read_accesses(const clsma::block* block, const z3::expr& address);
    void write_access(const clsma::block* block, const z3::expr& address, const clsma::variable* var);
    [[nodiscard]] z3::expr read_restrict_writes(const clsma::block* block, const z3::expr& address);
    void write_restrict_write(const clsma::block* block, const z3::expr& address, const clsma::variable* var);
};


#endif //OPENCL_RESTRICT_ANALYZER_RESTRICT_CHECKER_H
