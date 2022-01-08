#include "scope.h"

scope_context::scope_context(clang::ASTContext const& ast_ctx, z3::context& z3_ctx, memory& mem) : ast_ctx(ast_ctx), z3_ctx(z3_ctx), mem(mem) {}

scope* scope_context::make_scope() {
    scope scope(*this);
    return &scopes.emplace(scope.id, std::move(scope)).first->second;
}

scope* scope_context::make_scope(scope* parent) {
    scope scope(parent);
    return &scopes.emplace(scope.id, std::move(scope)).first->second;
}

const scope* scope_context::get_scope(int64_t scope_id) const {
    if (const auto& it = scopes.find(scope_id); it != scopes.end()) {
        return &it->second;
    }
    return nullptr;
}

const var* scope_context::get_var(const clang::ValueDecl* decl) const {
    return get_var(decl->getID());
}

const var* scope_context::get_var(int64_t decl_id) const {
    if (const auto& it = variables.find(decl_id); it != variables.end()) {
        return &it->second;
    }
    return nullptr;
}

scope::scope(scope_context& ctx) : ctx(ctx), id(ctx.next_id++), parent(nullptr) {}

scope::scope(scope* parent) : ctx(parent->ctx), id(parent->ctx.next_id++), parent(parent) {
    parent->children.push_back(this);
}

scope* scope::make_inner() {
    return ctx.make_scope(this);
}

const scope* const* scope::inner_begin() const {
    return children.data();
}

const scope* const* scope::inner_end() const {
    return children.data() + children.size();
}

var* scope::decl_var(const clang::VarDecl* decl) {
    if (ctx.variables.count(decl->getID())) {
        return nullptr;
    }
    var var(this, decl, decl->getType(), 1, ctx.mem.allocate(1));
    //var var(id, name, type, ctx.ast_ctx.getTypeSizeInChars(type).getQuantity(), ctx.mem_ptr);
    //ctx.mem_ptr += var.size;
    ctx.variables.emplace(decl->getID(), std::move(var));
    owned_variables.emplace(decl->getID());
    return &ctx.variables.at(decl->getID());
}

const var* scope::get_var(const clang::ValueDecl* decl) const {
    return get_var(decl->getID());
}

const var *scope::get_var(int64_t decl_id) const {
    return is_owned(decl_id) ? ctx.get_var(decl_id) : nullptr;
}

bool scope::is_owned(int64_t decl_id) const {
    if (owned_variables.count(decl_id)) {
        return true;
    }
    return parent != nullptr && parent->is_owned(decl_id);
}

/*z3::expr scope::deref(const z3::expr& address) const {
    z3::expr value = parent ? parent->deref(address) : ctx.z3_ctx.int_val(-100500);
    for (const auto& [name, var] : owned_variables) {
        value = z3::ite(address == ctx.z3_ctx.int_val(var.address), var.to_z3_expr(ctx.z3_ctx), value);
    }
    return value;
}*/

/*std::optional<z3::expr> scope::var_set(const std::string& name, const clang::QualType& type, std::optional<z3::expr> value) {
    if (!value.has_value()) {
        return std::nullopt;
    }
    std::string var_id = std::to_string(id) + "$" + name;
    if (auto it = versions.find(name); it != versions.end()) {
        var_id += "!!" + std::to_string(++it->second);
    } else {
        versions[name] = 0;
    }
    std::cout << value.value().to_string() << std::endl;
    return z3_ctx.constant(var_id.c_str(), type_to_sort(z3_ctx, type.getTypePtr())) == value.value();
}

std::optional<var> scope::var_get(const std::string& name, const clang::QualType& type) const {
    if (auto it = versions.find(name); it != versions.end()) {
        std::string var_id = std::to_string(id) + "$" + name;
        if (it->second > 0) {
            var_id += "!!" + std::to_string(it->second);
        }
        return z3_ctx.constant(var_id.c_str(), type_to_sort(z3_ctx, type.getTypePtr()));
    }
    return this->parent ? this->parent->get_var(name, type) : std::nullopt;
}*/
