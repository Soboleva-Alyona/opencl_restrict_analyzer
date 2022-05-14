#include "block.h"

namespace {
    z3::sort type_to_sort(z3::context& z3_ctx, const clang::QualType& type) {
        if (type->isIntegerType() || type->isPointerType()) {
            return z3_ctx.int_sort();
        } else if (type->isFloatingType()) {
            return z3_ctx.real_sort();
        } else if (type->isBooleanType()) {
            return z3_ctx.bool_sort();
        } else {
            return z3_ctx.uninterpreted_sort(type->getTypeClassName());
        }
    }

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
        } else {
            output.push_back('{');
            output += sort.name().str();
            output.push_back('}');
        }
    }
}

clsa::value_reference::value_reference(const clsa::block* block, std::string unique_id, std::string name,
                                       const z3::sort& sort)
    : block(block), unique_id(std::move(unique_id)), name(std::move(name)), sort(sort) {}

clsa::variable* clsa::value_reference::as_variable() {
    return const_cast<variable*>(const_cast<const value_reference*>(this)->as_variable());
}

const clsa::variable* clsa::value_reference::as_variable() const {
    return nullptr;
}

z3::expr clsa::value_reference::to_z3_expr() const {
    std::string z3_name = std::to_string(block->id).append("$").append(name);
    if (version > 0) {
        z3_name.append("!!").append(std::to_string(version));
    }
    return block->ctx.z3.constant(z3_name.c_str(), sort);
}

z3::expr clsa::value_reference::to_z3_expr(std::string_view key) const {
    std::string z3_name = std::to_string(block->id).append("$").append(name).append("#").append(key);
    if (version > 0) {
        z3_name.append("!!").append(std::to_string(version));
    }
    return block->ctx.z3.constant(z3_name.c_str(), sort);
}

clsa::optional_value clsa::value_reference::to_value() const {
    std::unordered_map<std::string, z3::expr> meta;
    for (const auto& key : meta_keys) {
        meta.emplace(key, to_z3_expr(key));
    }
    return {to_z3_expr(), std::move(meta)};
}

clsa::variable::variable(const class block* block, const clang::VarDecl* decl, const clang::QualType& type)
    : value_reference(block, std::to_string(decl->getID()), decl->getName().str(), type_to_sort(block->ctx.z3, type)),
      decl(decl), type(type) {}

const clsa::variable* clsa::variable::as_variable() const {
    return this;
}

clsa::block::block(block_context& ctx) : ctx(ctx), id(ctx.next_id++), parent(nullptr) {}

clsa::block::block(block* parent, std::optional<z3::expr> precondition) : ctx(parent->ctx), id(parent->ctx.next_id++),
                                                                          parent(parent),
                                                                          precondition(std::move(precondition)) {}

void clsa::block::join() {
    std::unordered_map<std::string, clsa::optional_value> values;
    std::optional<z3::expr> joined_assumption;
    for (const block* fork : forks) {
        if (fork->precondition.has_value()) {
            for (const auto& unique_id : fork->forked_decl_ids) {
                if (!values.count(unique_id)) {
                    values.emplace(unique_id, get(unique_id)->to_value());
                }
                const auto& forked_optional_value = fork->variables.at(unique_id)->to_value();
                auto& optional_value = values.at(unique_id);
                const auto& joined_value = optional_value.has_value() ? optional_value.value() : get(
                    unique_id)->to_z3_expr();
                optional_value.set_value(
                    z3::ite(fork->precondition.value(), forked_optional_value.value(), joined_value));
                for (const auto& [key, forked_meta] : forked_optional_value.metadata()) {
                    const auto& meta = optional_value.metadata(key);
                    const auto& joined_meta = meta.has_value() ? meta.value() : get(unique_id)->to_z3_expr(key);
                    optional_value.set_metadata(key, z3::ite(fork->precondition.value(), forked_meta, joined_meta));
                }
            }
        }
        const auto& forked_assumption = fork->get_assumption(this);
        if (forked_assumption.has_value()) {
            const auto assumption = fork->precondition.has_value() ?
                z3::implies(fork->precondition.value(), forked_assumption.value()) : forked_assumption.value();
            joined_assumption = !joined_assumption.has_value() ? assumption
                                                               : joined_assumption.value() && assumption;
        }
    }
    for (const auto& [unique_id, value] : values) {
        set(unique_id, value);
    }
    if (joined_assumption.has_value()) {
        assume(joined_assumption.value());
    }
    forks.clear();
}

clsa::block* clsa::block::make_inner() {
    return ctx.make_block(this, std::nullopt);
}

clsa::block* clsa::block::make_inner(std::optional<z3::expr> precondition) {
    return ctx.make_block(this, std::move(precondition));
}

const clsa::block* const* clsa::block::inner_begin() const {
    return children.data();
}

const clsa::block* const* clsa::block::inner_end() const {
    return children.data() + children.size();
}

void clsa::block::write(const z3::expr& address, const z3::expr& value) {
    std::string unique_id;
    sort_to_id(value.get_sort(), unique_id);
    auto* versioned_value = get(unique_id);
    if (versioned_value == nullptr) {
        auto* global = this;
        while (global->parent) {
            global = global->parent;
        }
        versioned_value = global->variables.emplace(unique_id, std::unique_ptr<clsa::value_reference>(
            new clsa::value_reference(global, unique_id, unique_id,
                ctx.z3.array_sort(ctx.z3.int_sort(), value.get_sort())))).first->second.get();
    }
    set(unique_id, z3::store(versioned_value->to_z3_expr(), address, value));
}

z3::expr clsa::block::read(const z3::expr& address, const clang::QualType& type) {
    return read(address, type_to_sort(ctx.z3, type));
}

z3::expr clsa::block::read(const z3::expr& address, const z3::sort& sort) {
    std::string unique_id;
    sort_to_id(sort, unique_id);
    return z3::select(get(unique_id)->to_z3_expr(), address);
}

clsa::value_reference* clsa::block::value_decl(std::string name, const clang::QualType& type) {
    return value_decl(std::move(name), type_to_sort(ctx.z3, type));
}

clsa::value_reference* clsa::block::value_decl(std::string name, const z3::sort& sort) {
    const auto unique_id = std::to_string(ctx.versioned_values++) + "%" + std::move(name);
    return variables.emplace(unique_id, std::unique_ptr<clsa::value_reference>(
        new clsa::value_reference(this, unique_id, unique_id, sort))).first->second.get();
}

namespace {
    std::optional<z3::expr> get_assignment(const std::optional<z3::expr>& assumption, const z3::expr& value_ref,
                                           const std::optional<z3::expr>& value,
                                           const std::optional<z3::expr>& previous_value) {
        if (value.has_value()) {
            if (assumption.has_value()) {
                if (previous_value.has_value()) {
                    return value_ref == z3::ite(assumption.value(), value.value(), previous_value.value());
                }
                return z3::implies(assumption.value(), value_ref == value.value());
            }
            return value_ref == value.value();
        }
        if (assumption.has_value() && previous_value.has_value()) {
            return z3::implies(!assumption.value(), value_ref == previous_value.value());
        }
        return std::nullopt;
    }
}

void clsa::block::value_set(value_reference* value_ref, const clsa::optional_value& value) {
    clsa::optional_value previous_value = value_ref->to_value();

    if (precondition.has_value() && value_ref->block != this) {
        forked_decl_ids.emplace(value_ref->unique_id);
        if (auto* var = value_ref->as_variable(); var != nullptr) {
            value_ref = var_decl(var->decl);
        } else {
            value_ref = variables.emplace(value_ref->unique_id, std::unique_ptr<clsa::value_reference>(
                new clsa::value_reference(this, value_ref->unique_id, value_ref->name, value_ref->sort)))
                    .first->second.get();
        }
    } else {
        ++value_ref->version;
    }

    std::optional<z3::expr> assumption = get_assumption(value_ref->block);

    std::optional<z3::expr> assignment = get_assignment(assumption, value_ref->to_z3_expr(), value, previous_value);
    if (assignment.has_value()) {
        ctx.solver.add(assignment.value());
    }
    for (const auto& [key, meta] : value.metadata()) {
        std::optional<z3::expr> meta_assignment = get_assignment(assumption, value_ref->to_z3_expr(key),
            meta, previous_value.metadata(key));
        if (meta_assignment.has_value()) {
            ctx.solver.add(meta_assignment.value());
        }
        value_ref->meta_keys.emplace(key);
    }
}

clsa::variable* clsa::block::var_decl(const clang::VarDecl* decl, const clsa::optional_value& value) {
    if (variables.count(std::to_string(decl->getID()))) {
        return nullptr;
    }
    clsa::variable* retvar = variables.emplace(std::to_string(decl->getID()), std::unique_ptr<clsa::variable>(
        new clsa::variable(this, decl, decl->getType()))).first->second->as_variable();
    if (value.has_value()) {
        ctx.solver.add(retvar->to_z3_expr() == value.value());
        for (const auto& [key, meta] : value.metadata()) {
            ctx.solver.add(retvar->to_z3_expr(key) == meta);
            retvar->meta_keys.emplace(key);
        }
    }
    return retvar;
}

clsa::variable* clsa::block::var_set(const clang::ValueDecl* decl, const clsa::optional_value& value) {
    return var_set(decl->getID(), value);
}

clsa::variable* clsa::block::var_set(int64_t decl_id, const clsa::optional_value& value) {
    auto* var = var_get(decl_id);
    if (!var) {
        throw std::invalid_argument("trying to assign a value to an undeclared variable");
    }
    return set(std::to_string(decl_id), value)->as_variable();
}

clsa::variable* clsa::block::var_get(const clang::ValueDecl* decl) {
    return decl ? var_get(decl->getID()) : nullptr;
}

const clsa::variable* clsa::block::var_get(const clang::ValueDecl* decl) const {
    return decl ? var_get(decl->getID()) : nullptr;
}

clsa::variable* clsa::block::var_get(int64_t decl_id) {
    return const_cast<variable*>(const_cast<const block*>(this)->var_get(decl_id));
}

const clsa::variable* clsa::block::var_get(int64_t decl_id) const {
    const auto* value = get(std::to_string(decl_id));
    return value ? value->as_variable() : nullptr;
}

void clsa::block::assume(const z3::expr& assumption) {
    assumptions = assumptions.has_value() ? assumptions.value() && assumption : assumption;
}

z3::check_result clsa::block::check(const z3::expr& assumption) const {
    auto env = get_assumption();
    z3::expr to_check = env.has_value() ? env.value() && assumption : assumption;
    return ctx.solver.check(1, &to_check);
}

std::optional<z3::expr> clsa::block::get_assumption() const {
    return get_assumption(nullptr);
}

std::optional<z3::expr> clsa::block::get_assumption(const clsa::block* up_to) const {
    if (this == up_to) {
        return std::nullopt;
    }
    std::optional<z3::expr> parent_assumption = parent ? parent->get_assumption(up_to) : std::nullopt;
    std::optional<z3::expr> assumption = !precondition.has_value() ? assumptions :
                                         !assumptions.has_value() ? precondition : precondition.value() && assumptions.value();
    if (assumption.has_value()) {
        if (parent_assumption.has_value()) {
            return assumption.value() && parent_assumption.value();
        }
        return assumption;
    }
    return parent_assumption;
}

clsa::value_reference* clsa::block::get(const std::string& unique_id) {
    return const_cast<clsa::value_reference*>(const_cast<const block*>(this)->get(unique_id));
}

const clsa::value_reference* clsa::block::get(const std::string& unique_id) const {
    if (const auto& it = variables.find(unique_id); it != variables.end()) {
        return it->second.get();
    }
    return parent ? parent->get(unique_id) : nullptr;
}

clsa::value_reference* clsa::block::set(const std::string& unique_id, const clsa::optional_value& value) {
    auto* value_ref = get(unique_id);
    if (!value_ref) {
        throw std::invalid_argument("trying to assign a value to an inexistent value reference");
    }
    value_set(value_ref, value);
    return value_ref;
}

clsa::block_context::block_context(z3::solver& solver) : solver(solver), z3(solver.ctx()) {}

clsa::block* clsa::block_context::make_block() {
    block block(*this);
    return &blocks.emplace(block.id, std::move(block)).first->second;
}

clsa::block* clsa::block_context::make_block(clsa::block* parent, std::optional<z3::expr> precondition) {
    block block(parent, std::move(precondition));
    class block* ptr = &blocks.emplace(block.id, std::move(block)).first->second;
    parent->children.emplace_back(ptr);
    parent->forks.emplace_back(ptr);
    return ptr;
}

uint64_t clsa::block_context::allocate(uint64_t size) {
    const uint64_t ptr = mem_ptr;
    mem_ptr += size;
    return ptr;
}