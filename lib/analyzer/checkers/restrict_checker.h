#ifndef OPENCL_RESTRICT_ANALYZER_RESTRICT_CHECKER_H
#define OPENCL_RESTRICT_ANALYZER_RESTRICT_CHECKER_H


#include "abstract_checker.h"

class restrict_checker : public abstract_checker {
public:
    explicit restrict_checker(analyzer_context& ctx);

    void check_memory_access(const block* block, const clang::Expr* expr, memory_access_type access_type, const z3::expr& address) override;
private:
    std::unordered_set<const block*> accessed_blocks;
    std::unordered_set<const block*> restrict_written_blocks;

    std::unordered_map<const block*, std::unordered_set<const var*>> restrict_writes;

    [[nodiscard]] z3::expr read_accesses(const block* block, const z3::expr& address);
    void write_access(const block* block, const z3::expr& address, const var* var);
    [[nodiscard]] z3::expr read_restrict_writes(const block* block, const z3::expr& address);
    void write_restrict_write(const block* block, const z3::expr& address, const var* var);
};


#endif //OPENCL_RESTRICT_ANALYZER_RESTRICT_CHECKER_H
