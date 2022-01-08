#include "memory.h"

namespace {
    void sort_to_id(const z3::sort& sort, std::string& output) {
        if (sort.is_int()) {
            output.push_back('I');
        } else if (sort.is_real()) {
            output.push_back('R');
        } else if (sort.is_bool()) {
            output.push_back('B');
        } else if (sort.is_array() && sort.array_domain().is_int()) {
            output.push_back('[');
            sort_to_id(sort.array_range(), output);
            output.push_back(']');
        }
    }

    std::string get_name(const std::string& sort_id, uint64_t version) {
        return  "$" + sort_id + "!" + std::to_string(version);
    }
}

memory::memory(z3::solver &solver) : solver(solver) {}

uint64_t memory::allocate(size_t size) {
    const uint64_t address = ptr;
    ptr += size;
    return address;
}

void memory::write(const z3::expr& address, const z3::expr& value) {
    write_meta("", address, value);
}

z3::expr memory::read(const z3::expr& address, const z3::sort& sort) const {
    return read_meta("", address, sort);
}

void memory::write_meta(std::string_view meta, const z3::expr& address, const z3::expr& value) {
    std::string sort_id(meta);
    if (!meta.empty()) {
        sort_id.push_back('_');
    }
    sort_to_id(value.get_sort(), sort_id);

    z3::context& ctx = solver.ctx();
    z3::expr memory_before = ctx.constant(get_name(sort_id, versions[sort_id]++).c_str(), ctx.array_sort(ctx.int_sort(), value.get_sort()));
    z3::expr memory_after = ctx.constant(get_name(sort_id, versions[sort_id]).c_str(), ctx.array_sort(ctx.int_sort(), value.get_sort()));
    solver.add(memory_after == z3::store(memory_before, address, value));
}

z3::expr memory::read_meta(std::string_view meta, const z3::expr& address, const z3::sort& sort) const {
    return z3::select(read_meta(meta, sort), address);
}

z3::expr memory::read_meta(std::string_view meta, const z3::sort& sort) const {
    std::string sort_id(meta);
    if (!meta.empty()) {
        sort_id.push_back('_');
    }
    sort_to_id(sort, sort_id);

    z3::context& ctx = solver.ctx();
    const auto& version_it = versions.find(sort_id);
    const uint64_t version = version_it != versions.end() ? version_it->second : 0;
    return ctx.constant(get_name(sort_id, version).c_str(), ctx.array_sort(ctx.int_sort(), sort));
}

void memory::cond_write(const z3::expr& cond, const z3::expr& address, const z3::expr& value) {
    cond_write_meta("", cond, address, value);
}

void memory::cond_write_meta(std::string_view meta, const z3::expr& cond, const z3::expr& address, const z3::expr& value) {
    std::string sort_id(meta);
    if (!meta.empty()) {
        sort_id.push_back('_');
    }
    sort_to_id(value.get_sort(), sort_id);

    z3::context& ctx = solver.ctx();
    z3::expr memory_before = ctx.constant(get_name(sort_id, versions[sort_id]++).c_str(), ctx.array_sort(ctx.int_sort(), value.get_sort()));
    z3::expr memory_after = ctx.constant(get_name(sort_id, versions[sort_id]).c_str(), ctx.array_sort(ctx.int_sort(), value.get_sort()));
    solver.add(memory_after == z3::ite(cond, z3::store(memory_before, address, value), memory_before));
}
