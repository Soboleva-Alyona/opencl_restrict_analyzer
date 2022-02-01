#include "block.h"

namespace clsma {

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

    variable::variable(const class block* block, const clang::VarDecl* decl, const clang::QualType& type, uint64_t size,
                       uint64_t address)
            : value_reference(block, std::to_string(decl->getID()), decl->getName().str(), type_to_sort(block->ctx.z3_ctx, type)), decl(decl), type(type),
              size(size), address(address) {}

    z3::expr value_reference::to_z3_expr() const {
        std::string version_suffix;
        if (version > 0) {
            version_suffix = "!!" + std::to_string(version);
        }
        return block->ctx.z3_ctx.constant((std::to_string(block->id) + "$" + name + version_suffix).c_str(), sort);
    }

    z3::expr value_reference::to_z3_expr(const std::string& key) const {
        std::string version_suffix;
        if (version > 0) {
            version_suffix = "!!" + std::to_string(version);
        }
        return block->ctx.z3_ctx.constant((std::to_string(block->id) + "$" + name + "#" + key + version_suffix).c_str(), sort);
    }

    clsma::optional_value value_reference::to_value() const {
        std::unordered_map<std::string, z3::expr> meta;
        for (const auto& key : meta_keys) {
            meta.emplace(key, to_z3_expr(key));
        }
        return clsma::optional_value(to_z3_expr(), std::move(meta));
    }

    z3::expr variable::to_z3_storage_expr() const {
        std::string version_suffix;
        if (version > 0) {
            version_suffix = "!!" + std::to_string(version);
        }
        return block->ctx.z3_ctx.constant(
                ("*" + std::to_string(block->id) + "$" + decl->getName().str() + version_suffix).c_str(),
                block->ctx.z3_ctx.array_sort(block->ctx.z3_ctx.int_sort(),
                                             type_to_sort(block->ctx.z3_ctx, type->getPointeeType())));
        //return block->ctx.z3_ctx.constant(("&" + std::to_string(block->id) + "$" + decl->getName().str() + version_suffix).c_str(), block->ctx.z3_ctx.int_sort());
    }

    block_context::block_context(clang::ASTContext const& ast_ctx, z3::solver& solver, memory& mem) : ast_ctx(ast_ctx),
                                                                                                      solver(solver),
                                                                                                      z3_ctx(solver.ctx()),
                                                                                                      mem(mem) {}

    block* block_context::make_block() {
        block block(*this);
        return &blocks.emplace(block.id, std::move(block)).first->second;
    }

    block* block_context::make_block(block* parent, std::optional<z3::expr> precondition) {
        block block(parent, std::move(precondition));
        class block* ptr = &blocks.emplace(block.id, std::move(block)).first->second;
        parent->children.emplace_back(ptr);
        if (ptr->precondition.has_value()) {
            parent->forks.emplace_back(ptr);
        }
        return ptr;
    }

    const block* block_context::get_block(int64_t block_id) const {
        if (const auto& it = blocks.find(block_id); it != blocks.end()) {
            return &it->second;
        }
        return nullptr;
    }

    block::block(block_context& ctx) : ctx(ctx), id(ctx.next_id++), parent(nullptr) {}

    block::block(block* parent, std::optional<z3::expr> precondition) : ctx(parent->ctx), id(parent->ctx.next_id++),
                                                                        parent(parent),
                                                                        precondition(std::move(precondition)) {}

    block* block::make_inner() {
        return ctx.make_block(this, std::nullopt);
    }

    block* block::make_inner(z3::expr precondition) {
        return ctx.make_block(this, std::move(precondition));
    }

    const block* const* block::inner_begin() const {
        return children.data();
    }

    const block* const* block::inner_end() const {
        return children.data() + children.size();
    }

    variable* block::decl_var(const clang::VarDecl* decl, const clsma::optional_value& value) {
        if (variables.count(std::to_string(decl->getID()))) {
            return nullptr;
        }
        //variable variable(id, name, type, ctx.ast_ctx.getTypeSizeInChars(type).getQuantity(), ctx.mem_ptr);
        //ctx.mem_ptr += variable.size;
        variable* retvar = variables.emplace(std::to_string(decl->getID()), std::unique_ptr<variable>(
                new variable(this, decl, decl->getType(), 1, ctx.mem.allocate(1)))).first->second->as_variable();
        if (value.has_value()) {
            ctx.solver.add(retvar->to_z3_expr() == value.value());
            for (const auto& [key, meta] : value.metadata()) {
                ctx.solver.add(retvar->to_z3_expr(key) == meta);
                retvar->meta_keys.emplace(key);
            }
        }
        return retvar;
        //return &ctx.variables.at(decl->getID());
    }

    clsma::value_reference* block::get(const std::string& unique_id) {
        return const_cast<clsma::value_reference*>(const_cast<const block*>(this)->get(unique_id));
    }

    const clsma::value_reference* block::get(const std::string& unique_id) const {
        if (const auto& it = variables.find(unique_id); it != variables.end()) {
            return it->second.get();
        }
        return parent ? parent->get(unique_id) : nullptr;
    }

    variable* block::get_var(const clang::ValueDecl* decl) {
        return decl ? get_var(decl->getID()) : nullptr;
    }

    const variable* block::get_var(const clang::ValueDecl* decl) const {
        return decl ? get_var(decl->getID()) : nullptr;
    }

    variable* block::get_var(int64_t decl_id) {
        return const_cast<variable*>(const_cast<const block*>(this)->get_var(decl_id));
    }

    const variable* block::get_var(int64_t decl_id) const {
        const auto* value = get(std::to_string(decl_id));
        return value ? value->as_variable() : nullptr;
    }

    std::optional<z3::expr> block::get_assumption() const {
        std::optional<z3::expr> parent_assumption = parent ? parent->get_assumption() : std::nullopt;

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

    variable* block::set_var(const clang::ValueDecl* decl, const clsma::optional_value& value) {
        return set_var(decl->getID(), value);
    }

    variable* block::set_var(int64_t decl_id, const clsma::optional_value& value) {
        auto* var = get_var(decl_id);
        if (!var) {
            throw std::invalid_argument("trying to assign a optional_value to an undeclared variable");
        }
        return set_var(decl_id, value, var->to_z3_expr());
    }

    void block::join() {
        std::unordered_map<std::string, clsma::optional_value> values;
        for (const block* fork: forks) {
            for (const auto& unique_id: fork->forked_decl_ids) {
                if (!values.count(unique_id)) {
                    values.emplace(unique_id, get(unique_id)->to_value());
                }
                const auto& forked_optional_value = fork->variables.at(unique_id)->to_value();
                auto& optional_value = values.at(unique_id);
                const auto& joined_value = optional_value.has_value() ? optional_value.value() : get(unique_id)->to_z3_expr();
                optional_value.set_value(z3::ite(fork->precondition.value(), forked_optional_value.value(), joined_value));
                for (const auto& [key, forked_meta] : forked_optional_value.metadata()) {
                    const auto& meta = optional_value.get_meta(key);
                    const auto& joined_meta = meta.has_value() ? meta.value() : get(unique_id)->to_z3_expr(key);
                    optional_value.set_meta(key, z3::ite(fork->precondition.value(), forked_meta, joined_meta));
                }
            }
        }
        for (const auto& [unique_id, value]: values) {
            set(unique_id, value);
        }
        forks.clear();
    }

    clsma::value_reference* block::set(const std::string& unique_id, const clsma::optional_value& value) {
        auto* versioned_value = get(unique_id);
        if (!versioned_value) {
            throw std::invalid_argument("trying to assign a value to an undeclared variable");
        }
        var_set(versioned_value, value);
        return versioned_value;
    }

    variable* block::set_var(int64_t decl_id, const clsma::optional_value& value, const z3::expr& address) {
        return set(std::to_string(decl_id), value)->as_variable();
        /*auto* variable = get_var(decl_id);
        if (!variable) {
            throw std::invalid_argument("trying to assign a optional_value to an undeclared variable");
        }
        if (precondition.has_value() && variable->block != this) {
            forked_decl_ids.emplace(decl_id);
            variable = decl_var(variable->decl);
        } else {
            ++variable->version;
        }
        ctx.solver.add(variable->to_z3_expr() == optional_value);
        ctx.solver.add(variable->to_z3_storage_expr() == address);
        return variable;*/
    }

    variable* block::set_var(const clang::ValueDecl* decl, const clsma::optional_value& value, const z3::expr& address) {
        return set_var(decl->getID(), value, address);
    }

    void block::write(const z3::expr& address, const z3::expr& value) {
        std::string unique_id;
        sort_to_id(value.get_sort(), unique_id);
        auto* versioned_value = get(unique_id);
        if (versioned_value == nullptr) {
            auto* global = this;
            while (global->parent) {
                global = global->parent;
            }
            versioned_value = global->variables.emplace(unique_id, std::unique_ptr<clsma::value_reference>(new clsma::value_reference(global, unique_id, unique_id, ctx.z3_ctx.array_sort(ctx.z3_ctx.int_sort(), value.get_sort())))).first->second.get();
        }
        set(unique_id, z3::store(versioned_value->to_z3_expr(), address, value));
    }

    z3::expr block::read(const z3::expr& address, const clang::QualType& type) {
        return read(address, type_to_sort(ctx.z3_ctx, type));
    }

    z3::expr block::read(const z3::expr& address, const z3::sort& sort) {
        std::string unique_id;
        sort_to_id(sort, unique_id);
        return z3::select(get(unique_id)->to_z3_expr(), address);
    }

    void block::var_set(value_reference* versioned_value, const optional_value& value) {
        if (precondition.has_value() && versioned_value->block != this) {
            forked_decl_ids.emplace(versioned_value->unique_id);
            if (auto* var = versioned_value->as_variable(); var != nullptr) {
                versioned_value = decl_var(var->decl);
            } else {
                versioned_value =
                        variables.emplace(versioned_value->unique_id, std::unique_ptr<clsma::value_reference>(new clsma::value_reference(this, versioned_value->unique_id, versioned_value->name, versioned_value->sort))).first->second.get();
            }
        } else {
            ++versioned_value->version;
        }
        if (value.has_value()) {
            ctx.solver.add(versioned_value->to_z3_expr() == value.value());
            for (const auto& [key, meta] : value.metadata()) {
                ctx.solver.add(versioned_value->to_z3_expr(key) == meta);
                versioned_value->meta_keys.emplace(key);
            }
        }
    }

    value_reference* block::value_decl(std::string name, const clang::QualType& type) {
        const auto unique_id = std::to_string(ctx.versioned_values++) + "%" + std::move(name);
        return variables.emplace(unique_id, std::unique_ptr<clsma::value_reference>(new clsma::value_reference(this, unique_id, unique_id, type_to_sort(ctx.z3_ctx, type)))).first->second.get();
    }

    void block::assume(const z3::expr& assumption) {
        assumptions = assumptions.has_value() ? assumptions.value() && assumption : assumption;
    }

    z3::check_result block::check(const z3::expr& assumption) const {
        auto env = get_assumption();
        z3::expr to_check = env.has_value() ? env.value() && assumption : assumption;
        return ctx.solver.check(1, &to_check);
    }

    value_reference::value_reference(const clsma::block* block, std::string unique_id, std::string name, z3::sort sort)
            : block(block), unique_id(std::move(unique_id)), name(std::move(name)), sort(std::move(sort)) {}

    clsma::variable* value_reference::as_variable() {
        return const_cast<variable*>(const_cast<const value_reference*>(this)->as_variable());
    }

    const clsma::variable* value_reference::as_variable() const {
        return nullptr;
    }

    const clsma::variable* variable::as_variable() const {
        return this;
    }

/*z3::expr block::deref(const z3::expr& address) const {
    z3::expr optional_value = parent ? parent->deref(address) : ctx.z3_ctx.int_val(-100500);
    for (const auto& [name, variable] : owned_variables) {
        optional_value = z3::ite(address == ctx.z3_ctx.int_val(variable.address), variable.to_z3_expr(ctx.z3_ctx), optional_value);
    }
    return optional_value;
}*/

/*std::optional<z3::expr> block::var_set(const std::string& name, const clang::QualType& type, std::optional<z3::expr> optional_value) {
    if (!optional_value.has_value()) {
        return std::nullopt;
    }
    std::string var_id = std::to_string(id) + "$" + name;
    if (auto it = versions.find(name); it != versions.end()) {
        var_id += "!!" + std::to_string(++it->second);
    } else {
        versions[name] = 0;
    }
    std::cout << optional_value.optional_value().to_string() << std::endl;
    return z3_ctx.constant(var_id.c_str(), type_to_sort(z3_ctx, type.getTypePtr())) == optional_value.optional_value();
}

std::optional<variable> block::var_get(const std::string& name, const clang::QualType& type) const {
    if (auto it = versions.find(name); it != versions.end()) {
        std::string var_id = std::to_string(id) + "$" + name;
        if (it->second > 0) {
            var_id += "!!" + std::to_string(it->second);
        }
        return z3_ctx.constant(var_id.c_str(), type_to_sort(z3_ctx, type.getTypePtr()));
    }
    return this->parent ? this->parent->get_var(name, type) : std::nullopt;
}*/
}