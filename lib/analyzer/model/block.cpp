#include "block.h"

namespace {
    z3::sort type_to_sort(z3::context& z3_ctx, const clang::QualType& type) {
        if (type->isIntegerType()) {
            return z3_ctx.int_sort();
        } else if (type->isFloatingType()) {
            return z3_ctx.real_sort();
        } else if (type->isBooleanType()) {
            return z3_ctx.bool_sort();
        } else if (type->isPointerType()) {
            return z3_ctx.array_sort(z3_ctx.int_sort(), type_to_sort(z3_ctx, type->getPointeeType()));
        } else {
            return z3_ctx.uninterpreted_sort(type->getTypeClassName());
        }
    }
}

var::var(const class block* block, const clang::VarDecl* decl, const clang::QualType& type, uint64_t size, uint64_t address)
        : block(block), decl(decl), type(type), size(size), address(address) {}

z3::expr var::to_z3_expr() const {
    std::string version_suffix;
    if (version > 0) {
        version_suffix = "!!" + std::to_string(version);
    }
    return block->ctx.z3_ctx.constant((std::to_string(block->id) + "$" + decl->getName().str() + version_suffix).c_str(), type_to_sort(block->ctx.z3_ctx, type));
}

block_context::block_context(clang::ASTContext const& ast_ctx, z3::context& z3_ctx, memory& mem) : ast_ctx(ast_ctx), z3_ctx(z3_ctx), mem(mem) {}

block* block_context::make_block() {
    block block(*this);
    return &blocks.emplace(block.id, std::move(block)).first->second;
}

block* block_context::make_block(block* parent) {
    block block(parent);
    return &blocks.emplace(block.id, std::move(block)).first->second;
}

const block* block_context::get_block(int64_t block_id) const {
    if (const auto& it = blocks.find(block_id); it != blocks.end()) {
        return &it->second;
    }
    return nullptr;
}

block::block(block_context& ctx) : ctx(ctx), id(ctx.next_id++), parent(nullptr) {}

block::block(block* parent) : ctx(parent->ctx), id(parent->ctx.next_id++), parent(parent) {
    parent->children.push_back(this);
}

block* block::make_inner() {
    return ctx.make_block(this);
}

const block* const* block::inner_begin() const {
    return children.data();
}

const block* const* block::inner_end() const {
    return children.data() + children.size();
}

var* block::decl_var(const clang::VarDecl* decl) {
    if (variables.count(decl->getID())) {
        return nullptr;
    }
    var var(this, decl, decl->getType(), 1, ctx.mem.allocate(1));
    //var var(id, name, type, ctx.ast_ctx.getTypeSizeInChars(type).getQuantity(), ctx.mem_ptr);
    //ctx.mem_ptr += var.size;
    return &variables.emplace(decl->getID(), std::move(var)).first->second;
    //return &ctx.variables.at(decl->getID());
}

var* block::get_var(const clang::ValueDecl* decl) {
    return get_var(decl->getID());
}

const var* block::get_var(const clang::ValueDecl* decl) const {
    return get_var(decl->getID());
}

var* block::get_var(int64_t decl_id) {
    return const_cast<var*>(const_cast<const block*>(this)->get_var(decl_id));
}

const var* block::get_var(int64_t decl_id) const {
    if (const auto& it = variables.find(decl_id); it != variables.end()) {
        return &it->second;
    }
    return parent ? parent->get_var(decl_id) : nullptr;
}

void var::next_version() {
    ++version;
}

/*z3::expr block::deref(const z3::expr& address) const {
    z3::expr value = parent ? parent->deref(address) : ctx.z3_ctx.int_val(-100500);
    for (const auto& [name, var] : owned_variables) {
        value = z3::ite(address == ctx.z3_ctx.int_val(var.address), var.to_z3_expr(ctx.z3_ctx), value);
    }
    return value;
}*/

/*std::optional<z3::expr> block::var_set(const std::string& name, const clang::QualType& type, std::optional<z3::expr> value) {
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

std::optional<var> block::var_get(const std::string& name, const clang::QualType& type) const {
    if (auto it = versions.find(name); it != versions.end()) {
        std::string var_id = std::to_string(id) + "$" + name;
        if (it->second > 0) {
            var_id += "!!" + std::to_string(it->second);
        }
        return z3_ctx.constant(var_id.c_str(), type_to_sort(z3_ctx, type.getTypePtr()));
    }
    return this->parent ? this->parent->get_var(name, type) : std::nullopt;
}*/
