#ifndef OPENCL_RESTRICT_ANALYZER_MEMORY_H
#define OPENCL_RESTRICT_ANALYZER_MEMORY_H


#include <unordered_map>

#include <z3++.h>

class memory {
public:
    explicit memory(z3::solver& solver);
    uint64_t allocate(size_t size);
    void write(const z3::expr& address, const z3::expr& value);
    [[nodiscard]] z3::expr read(const z3::expr& address, const z3::sort& sort) const;
    void write_meta(std::string_view meta, const z3::expr& address, const z3::expr& value);
    [[nodiscard]] z3::expr read_meta(std::string_view meta, const z3::expr& address, const z3::sort& sort) const;
    [[nodiscard]] z3::expr read_meta(std::string_view meta, const z3::sort& sort) const;
    void cond_write(const z3::expr& cond, const z3::expr& address, const z3::expr& value);
    void cond_write_meta(std::string_view meta, const z3::expr& cond, const z3::expr& address, const z3::expr& value);
private:
    z3::solver& solver;
    std::unordered_map<std::string, uint64_t> versions;
    uint64_t ptr = 1;
};


#endif //OPENCL_RESTRICT_ANALYZER_MEMORY_H
