#include "ast_visitor.h"

#include <iostream>
#include <unordered_map>

#include <z3++.h>

#include <CL/cl.h>

#include "pseudocl.h"

clsma::ast_visitor::ast_visitor(analyzer_context& ctx) : ctx(ctx) {
    global_block = ctx.block.make_block();
}

void clsma::ast_visitor::set_violation_handler(std::function<void(clsma::violation)> handler) {
    violation_handler = std::move(handler);
}

void clsma::ast_visitor::add_checker(std::unique_ptr<clsma::abstract_checker> checker) {
    checkers.emplace_back(std::move(checker));
}

bool clsma::ast_visitor::VisitFunctionDecl(clang::FunctionDecl* f) {
    if (!f->hasAttr<clang::OpenCLKernelAttr>() || !f->hasBody() || f->getName() != ctx.parameters.kernel_name) {
        return true;
    }
    clsma::block* kernel_block = global_block->make_inner();
    for (uint32_t i = 0; i < ctx.parameters.work_dim; ++i) {
        const auto& global_offset = kernel_block->value_decl("global_offset#" + std::to_string(i), ctx.z3.int_sort());
        kernel_block->assume(global_offset->to_z3_expr() == 0);
        global_offsets.push_back(global_offset);
        const auto& global_id = kernel_block->value_decl("global_id#" + std::to_string(i), ctx.z3.int_sort());
        kernel_block->assume(global_id->to_z3_expr() >= 0 &&
                             global_id->to_z3_expr() < ctx.z3.int_val(uint64_t(ctx.parameters.global_work_size[i])));
        global_ids.push_back(global_id);
        const auto& local_size = kernel_block->value_decl("local_size#" + std::to_string(i), ctx.z3.int_sort());
        if (ctx.parameters.local_work_size.has_value()) {
            kernel_block->assume(
                local_size->to_z3_expr() == ctx.z3.int_val(uint64_t(ctx.parameters.local_work_size.value()[i])));
        } else {
            kernel_block->assume(local_size->to_z3_expr() >= 1 && local_size->to_z3_expr() <= ctx.z3.int_val(
                uint64_t(ctx.parameters.global_work_size[i])));
        }
        local_sizes.push_back(local_size);
        const auto& local_id = kernel_block->value_decl("local_id#" + std::to_string(i), ctx.z3.int_sort());
        kernel_block->assume(local_id->to_z3_expr() >= 0 && local_id->to_z3_expr() < local_size->to_z3_expr());
        local_ids.push_back(local_id);
    }
    for (unsigned i = 0; i < f->getNumParams(); ++i) {
        const auto& [size, data] = ctx.parameters.args[i];
        const clang::ParmVarDecl* decl = f->getParamDecl(i);
        const clang::QualType& type = decl->getType();
        const clsma::variable* var = kernel_block->var_decl(decl);
        if (type->isPointerType() && size == sizeof(cl_mem)) {
            const auto element_size = ctx.ast.getTypeSizeInChars(type->getPointeeType()).getQuantity();
            size_t mem_size;
            if (clsma::pseudocl_is_valid_mem_object(*reinterpret_cast<clsma::pseudocl_mem*>(data))) {
                mem_size = clsma::pseudocl_get_mem_object_size(*reinterpret_cast<clsma::pseudocl_mem*>(data));
            } else {
                cl_int err = clGetMemObjectInfo(*reinterpret_cast<cl_mem*>(data), CL_MEM_SIZE, sizeof(mem_size),
                    &mem_size,
                    nullptr);
                if (err != 0) {
                    // TODO: warn
                    continue;
                }
            }
            const uint64_t array_size = mem_size / element_size;
            const uint64_t base = ctx.block.allocate(array_size);
            kernel_block->var_set(decl, clsma::optional_value(ctx.z3.int_val(base), {
                {clsma::VAR_META_MEM_BASE, ctx.z3.int_val(base)},
                {clsma::VAR_META_MEM_SIZE, ctx.z3.int_val(array_size)}
            }));
            if (ctx.parameters.options.array_values) {
                for (uint64_t offset = 0; offset < array_size; ++offset) {
                    global_block->write(ctx.z3.int_val(var->address + offset), ctx.z3.int_val(0));
                }
            }
        } else if (type->isIntegerType()) {
            // TODO
        }
    }
    process_stmt(kernel_block, f->getBody(), *kernel_block->value_decl("return", f->getReturnType()));
    for (const auto& assertion: ctx.solver.assertions()) {
        std::cout << assertion.to_string() << std::endl;
    }
    try {
        std::cout << ctx.solver.check() << std::endl;
    } catch (z3::exception& ex) {
        std::cerr << ex.msg() << std::endl;
    }
    return false;
}

bool clsma::ast_visitor::process_stmt(clsma::block* block, const clang::Stmt* stmt, clsma::value_reference& ret_ref) {
    //std::cout << stmt->getStmtClassName() << std::endl;
    if (clang::isa<clang::CompoundStmt>(stmt)) {
        return process_compound_stmt(block, clang::cast<clang::CompoundStmt>(stmt), ret_ref);
    } else if (clang::isa<clang::WhileStmt>(stmt)) {
        return process_while_stmt(block, clang::cast<clang::WhileStmt>(stmt), ret_ref);
    } else if (clang::isa<clang::DoStmt>(stmt)) {
        return process_do_stmt(block, clang::cast<clang::DoStmt>(stmt), ret_ref);
    } else if (clang::isa<clang::ForStmt>(stmt)) {
        return process_for_stmt(block, clang::cast<clang::ForStmt>(stmt), ret_ref);
    } else if (clang::isa<clang::IfStmt>(stmt)) {
        return process_if_stmt(block, clang::cast<clang::IfStmt>(stmt), ret_ref);
    } else if (clang::isa<clang::ReturnStmt>(stmt)) {
        return process_return_stmt(block, clang::cast<clang::ReturnStmt>(stmt), ret_ref);
    } else if (clang::isa<clang::DeclStmt>(stmt)) {
        process_decl_stmt(block, clang::cast<clang::DeclStmt>(stmt), ret_ref);
    } else if (clang::isa<clang::ValueStmt>(stmt)) {
        transform_value_stmt(block, clang::cast<clang::ValueStmt>(stmt));
    }
    return true;
}

bool clsma::ast_visitor::process_return_stmt(clsma::block* block, const clang::ReturnStmt* return_stmt,
                                             clsma::value_reference& ret_ref) {
    if (return_stmt->getRetValue()) {
        const auto expr = transform_expr(block, return_stmt->getRetValue());
        if (expr.has_value()) {
            block->value_set(&ret_ref, expr.value());
        }
    }
    return false;
}

bool clsma::ast_visitor::process_compound_stmt(clsma::block* block, const clang::CompoundStmt* compound_stmt,
                                               clsma::value_reference& ret_ref) {
    for (clang::Stmt* const* stmt_ptr = compound_stmt->body_begin();
         stmt_ptr != compound_stmt->body_end(); ++stmt_ptr) {
        if (!process_stmt(block, *stmt_ptr, ret_ref)) {
            return false;
        }
    }
    return true;
}

void clsma::ast_visitor::process_decl_stmt(clsma::block* block, const clang::DeclStmt* decl_stmt,
                                           clsma::value_reference& ret_ref) {
    std::for_each(decl_stmt->decl_begin(), decl_stmt->decl_end(), [&](clang::Decl* decl) {
        process_decl(block, decl, ret_ref);
    });
}

void clsma::ast_visitor::process_decl(clsma::block* block, const clang::Decl* decl, clsma::value_reference& ret_ref) {
    switch (decl->getKind()) {
        case clang::Decl::Var:
            process_var_decl(block, clang::cast<clang::VarDecl>(decl), ret_ref);
            break;
        default:
            break;
    }
}

void clsma::ast_visitor::process_var_decl(clsma::block* block, const clang::VarDecl* var_decl,
                                          clsma::value_reference& ret_ref) {
    block->var_decl(var_decl,
        var_decl->hasInit() ? transform_expr(block, var_decl->getInit()) : clsma::optional_value());
    /*const clsma::variable* var = block->var_decl(var_decl);
    if (var_decl->getAnyInitializer()) {
        const auto initializer = transform_expr(block, var_decl->getAnyInitializer());
        if (initializer.has_value()) {
            const z3::expr address = ctx.z3.int_val(var->address);
            ctx.mem.write(address, ctx.mem.read(initializer.value(), ctx.z3.int_sort()));
            if (var_decl->getType()->isPointerType()) {
                //ctx.mem.write_meta(META_BASE, address, ctx.mem.read_meta(META_BASE, initializer.optional_value(), ctx.z3.int_sort()));
                //ctx.mem.write_meta(META_SIZE, address, ctx.mem.read_meta(META_SIZE, initializer.optional_value(), ctx.z3.int_sort()));
            }
            //solver.add(mem_write(z3, z3.int_val(variable->address), initializer.optional_value()));
            //solver.add(variable->to_z3_expr(z3) == initializer.optional_value());
        }
    }*/
}

clsma::optional_value
clsma::ast_visitor::transform_value_stmt(clsma::block* block, const clang::ValueStmt* value_stmt) {
    if (clang::isa<clang::Expr>(value_stmt)) {
        return transform_expr(block, clang::cast<clang::Expr>(value_stmt));
    } else {
        std::cout << "WARN: unknown optional_value stmt type: " << value_stmt->getStmtClassName() << std::endl;
        return {};
    }
}

clsma::optional_value clsma::ast_visitor::transform_expr(clsma::block* block, const clang::Expr* expr) {
    if (clang::isa<clang::ArraySubscriptExpr>(expr)) {
        return transform_array_subscript_expr(block, clang::cast<clang::ArraySubscriptExpr>(expr));
    } else if (clang::isa<clang::BinaryOperator>(expr)) {
        return transform_binary_operator(block, clang::cast<clang::BinaryOperator>(expr));
    } else if (clang::isa<clang::UnaryOperator>(expr)) {
        return transform_unary_operator(block, clang::cast<clang::UnaryOperator>(expr));
    } else if (clang::isa<clang::CallExpr>(expr)) {
        return transform_call_expr(block, clang::cast<clang::CallExpr>(expr));
    } else if (clang::isa<clang::DeclRefExpr>(expr)) {
        return transform_decl_ref_expr(block, clang::cast<clang::DeclRefExpr>(expr));
    } else if (clang::isa<clang::ImplicitCastExpr>(expr)) {
        return transform_implicit_cast_expr(block, clang::cast<clang::ImplicitCastExpr>(expr));
    } else if (clang::isa<clang::ParenExpr>(expr)) {
        return transform_paren_expr(block, clang::cast<clang::ParenExpr>(expr));
    } else if (clang::isa<clang::IntegerLiteral>(expr)) {
        return ctx.z3.int_val(clang::cast<clang::IntegerLiteral>(expr)->getValue().getLimitedValue());
        //const z3::expr address = ctx.z3.int_val(ctx.mem.allocate(1));
        //ctx.mem.write(address, ctx.z3.int_val(clang::cast<clang::IntegerLiteral>(expr)->getValue().getLimitedValue()));
        //return address;
    } else if (clang::isa<clang::CXXBoolLiteralExpr>(expr)) {
        return ctx.z3.bool_val(clang::cast<clang::CXXBoolLiteralExpr>(expr)->getValue());
    } else {
        std::cout << "WARN: unknown expr type: " << expr->getStmtClassName() << std::endl;
        return {};
    }
}

clsma::optional_value clsma::ast_visitor::transform_array_subscript_expr(clsma::block* block,
                                                                         const clang::ArraySubscriptExpr* array_subscript_expr) {
    auto lhs = transform_expr(block, array_subscript_expr->getBase());
    auto rhs = transform_expr(block, array_subscript_expr->getIdx());
    if (!lhs.has_value() || !rhs.has_value()) {
        return {};
    }
    //const auto base_address = get_address(block, array_subscript_expr->getBase());
    //if (base_address.has_value()) {
    check_memory_access(block, array_subscript_expr, clsma::memory_access_type::read, lhs.value() + rhs.value());
    //}
    //std::cout << "base type: " << array_subscript_expr->getBase()->getType().getAsString() << std::endl;
    //std::cout << "restrict: " << array_subscript_expr->getBase()->getType().isRestrictQualified() << std::endl;
    if (ctx.parameters.options.array_values) {
        return block->read(lhs.value() + rhs.value(), array_subscript_expr->getType());
    } else {
        return {};
    }
}

clsma::optional_value clsma::ast_visitor::transform_binary_operator(clsma::block* block,
                                                                    const clang::BinaryOperator* binary_operator) {
    switch (binary_operator->getOpcode()) {
        case clang::BO_Assign: {
            //std::optional<z3::expr> lhs = transform_expr(block, binary_operator->getLHS());
            auto rhs = transform_expr(block, binary_operator->getRHS());
            //if (lhs.has_value() && rhs.has_value()) {
            //checker.check_memory_access(checker.read, rhs.optional_value());
            //checker.check_memory_access(checker.write, lhs.optional_value());
            //write(block, binary_operator->getLHS(), lhs.optional_value(), read(block, binary_operator->getRHS(), rhs.optional_value()));
            //ctx.mem.write_meta(META_BASE, lhs.optional_value(), ctx.mem.read_meta(META_BASE, rhs.optional_value(), ctx.z3.int_sort()));
            //ctx.mem.write_meta(META_SIZE, lhs.optional_value(), ctx.mem.read_meta(META_SIZE, rhs.optional_value(), ctx.z3.int_sort()));
            //solver.add(lhs.optional_value() == rhs.optional_value());
            //}
            assign(block, binary_operator->getLHS(), rhs);
            //const auto lhs_address = get_address(block, binary_operator->getLHS());
            //const auto rhs_address = get_address(block, binary_operator->getRHS());
            //if (lhs_address.has_value() && rhs_address.has_value()) {
            //    ctx.solver.add(lhs_address.optional_value() == rhs_address.optional_value());
            //}
            return rhs;
            //return lhs; // memory-core
        }
        case clang::BO_Add: {
            auto lhs = transform_expr(block, binary_operator->getLHS());
            auto rhs = transform_expr(block, binary_operator->getRHS());
            if (!lhs.has_value() || !rhs.has_value()) {
                return {};
            }
            return lhs.value() + rhs.value();
            //return push(read(block, binary_operator->getLHS(), lhs.optional_value()) + read(block, binary_operator->getRHS(), rhs.optional_value()));
        }
        case clang::BO_Sub: {
            auto lhs = transform_expr(block, binary_operator->getLHS());
            auto rhs = transform_expr(block, binary_operator->getRHS());
            if (!lhs.has_value() || !rhs.has_value()) {
                return {};
            }
            return lhs.value() - rhs.value();
            //const z3::expr result = ctx.z3.int_val(ctx.mem.allocate(1));
            //ctx.mem.write(result, ctx.mem.read(lhs.optional_value(), ctx.z3.int_sort()) - ctx.mem.read(rhs.optional_value(), ctx.z3.int_sort()));
            //return result;
        }
        case clang::BO_LT: {
            auto lhs = transform_expr(block, binary_operator->getLHS());
            auto rhs = transform_expr(block, binary_operator->getRHS());
            if (!lhs.has_value() || !rhs.has_value()) {
                return {};
            }
            return lhs.value() < rhs.value();
        }
        case clang::BO_EQ: {
            auto lhs = transform_expr(block, binary_operator->getLHS());
            auto rhs = transform_expr(block, binary_operator->getRHS());
            if (!lhs.has_value() || !rhs.has_value()) {
                return {};
            }
            return lhs.value() == rhs.value();
        }
        default: {
            std::cout << "WARN: unknown BO opcode: " << binary_operator->getOpcodeStr().str() << std::endl;
            return {};
        }
    }
}


clsma::optional_value clsma::ast_visitor::transform_unary_operator(clsma::block* block,
                                                                   const clang::UnaryOperator* unary_operator) {
    clsma::optional_value sub_expr = transform_expr(block, unary_operator->getSubExpr());
    if (!sub_expr.has_value()) {
        return {};
    }
    switch (unary_operator->getOpcode()) {
        case clang::UO_AddrOf: {
            return {};
        }
        case clang::UO_Deref: {
            check_memory_access(block, unary_operator, clsma::memory_access_type::read, sub_expr.value());
            if (ctx.parameters.options.array_values) {
                return block->read(sub_expr.value(), unary_operator->getType());
            }
            return {};
        }
        case clang::UO_PreInc: {
            sub_expr.set_value(sub_expr.value() + 1);
            assign(block, unary_operator->getSubExpr(), sub_expr);
            return sub_expr;
        }
        case clang::UO_PostInc: {
            assign(block, unary_operator->getSubExpr(), sub_expr.map_value([](auto value) { return value + 1; }));
            return sub_expr;
        }
        default: {
            std::cout << "WARN: unknown UO opcode: " << unary_operator->getOpcode() << std::endl;
            return {};
        }
    }
}

clsma::optional_value clsma::ast_visitor::transform_call_expr(clsma::block* block, const clang::CallExpr* call_expr) {
    static std::unordered_map<std::string, clsma::optional_value (clsma::ast_visitor::*)(const std::vector<clsma::optional_value>&)> builtin_handlers = {
        {"get_work_dim",         &clsma::ast_visitor::handle_get_work_dim},
        {"get_global_size",      &clsma::ast_visitor::handle_get_global_size},
        {"get_global_id",        &clsma::ast_visitor::handle_get_global_id},
        {"get_local_size",       &clsma::ast_visitor::handle_get_local_size},
        {"get_local_id",         &clsma::ast_visitor::handle_get_local_id},
        {"get_global_offset",    &clsma::ast_visitor::handle_get_global_offset},
        {"get_global_linear_id", &clsma::ast_visitor::handle_get_global_linear_id},
        {"get_local_linear_id",  &clsma::ast_visitor::handle_get_local_linear_id},
    };
    const clang::Decl* callee_decl = call_expr->getCalleeDecl();
    std::vector<clsma::optional_value> args;
    std::transform(call_expr->getArgs(), call_expr->getArgs() + call_expr->getNumArgs(),
        std::inserter(args, args.begin()), [&](const clang::Expr* expr) {
            return transform_expr(block, expr);
        });
    if (clang::isa<clang::NamedDecl>(callee_decl)) {
        const std::string& name = clang::cast<clang::NamedDecl>(callee_decl)->getName().str();
        if (auto it = builtin_handlers.find(name); it != builtin_handlers.end()) {
            return (this->*it->second)(args);
        }
    }
    if (clang::isa<clang::FunctionDecl>(callee_decl)) {
        clsma::block* call_block = global_block->make_inner();
        const auto* function_decl = clang::cast<clang::FunctionDecl>(callee_decl);
        for (unsigned int i = 0; i < function_decl->getNumParams(); ++i) {
            const auto* var = call_block->var_decl(function_decl->getParamDecl(i));
            if (args[i].has_value()) {
                ctx.solver.add(var->to_z3_expr() == args[i].value());
            }
        }
        auto* ret_ref = block->value_decl("return", function_decl->getReturnType());
        process_stmt(call_block, callee_decl->getBody(), *ret_ref);
        return ret_ref->to_value();
    }
    return {};
}

clsma::optional_value clsma::ast_visitor::transform_decl_ref_expr(clsma::block* block,
                                                                  const clang::DeclRefExpr* decl_ref_expr) {
    const clsma::variable* var = block->var_get(decl_ref_expr->getDecl());
    return var ? var->to_value() : clsma::optional_value();
    //return variable ? std::make_optional(ctx.z3.int_val(variable->address)) : std::nullopt;
    //return variable ? std::make_optional(variable->to_z3_expr(z3)) : std::nullopt;
};

clsma::optional_value clsma::ast_visitor::transform_implicit_cast_expr(clsma::block* block,
                                                                       const clang::ImplicitCastExpr* implicit_cast_expr) {
    return transform_expr(block, implicit_cast_expr->getSubExpr());
}

clsma::optional_value
clsma::ast_visitor::transform_paren_expr(clsma::block* block, const clang::ParenExpr* paren_expr) {
    return transform_expr(block, paren_expr->getSubExpr());
}

void clsma::ast_visitor::assign(clsma::block* block, const clang::Expr* expr, const clsma::optional_value& value) {
    return assign(block, expr, value, std::nullopt);
}

void clsma::ast_visitor::assign(clsma::block* block, const clang::Expr* lhs, const clsma::optional_value& value,
                                const std::optional<z3::expr>& storage) {
    lhs = lhs->IgnoreParenCasts();
    if (clang::isa<clang::ArraySubscriptExpr>(lhs)) {
        const auto* array_subscript_expr = clang::cast<clang::ArraySubscriptExpr>(lhs);
        const auto base = transform_expr(block, array_subscript_expr->getBase());
        const auto idx = transform_expr(block, array_subscript_expr->getIdx());
        if (!base.has_value() || !idx.has_value()) {
            return;
        }
        //const auto base_address = get_address(block, array_subscript_expr->getBase());
        //if (base_address.has_value()) {
        check_memory_access(block, lhs, clsma::memory_access_type::write, base.value() + idx.value());
        //}
        if (ctx.parameters.options.array_values && value.has_value()) {
            block->write(base.value() + idx.value(), value.value());
        }
        //const auto array = get_address(block, array_subscript_expr->getBase());
        //if (array.has_value()) {
        //    assign(block, array_subscript_expr->getBase(), base.optional_value(), z3::store(array.optional_value(), idx.optional_value(), optional_value));
        //}
        //const auto new_base_address = get_address(block, array_subscript_expr->getBase());
        //if (base_address.has_value() && new_base_address.has_value()) {
        //    ctx.solver.add(new_base_address.optional_value() == base_address.optional_value());
        //}
    } else if (clang::isa<clang::DeclRefExpr>(lhs)) {
        const auto* decl_ref_expr = clang::cast<clang::DeclRefExpr>(lhs);
        //const auto address = get_address(block, rhs);
        //if (address.has_value()) {
        //    block->var_set(decl_ref_expr->getDecl(), optional_value, *address);
        // } else {
        block->var_set(decl_ref_expr->getDecl(), value);

        /*if (storage.has_value()) {
                block->var_set(decl_ref_expr->getDecl(), optional_value, *storage);
            } else {
                block->var_set(decl_ref_expr->getDecl(), optional_value);
        }*/


        //}
        //variable->next_version();
        //ctx.solver.add(variable->to_z3_expr() == optional_value);
    }
}

void clsma::ast_visitor::check_memory_access(const clsma::block* block, const clang::Expr* expr,
                                             clsma::memory_access_type access_type, const z3::expr& address) {
    for (auto& checker: checkers) {
        std::optional<clsma::violation> violation = checker->check_memory_access(block, expr, access_type, address);
        if (violation.has_value() && violation_handler) {
            violation_handler(std::move(violation.value()));
        }
    }
}

bool clsma::ast_visitor::process_if_stmt(clsma::block* block, const clang::IfStmt* if_stmt,
                                         clsma::value_reference& ret_ref) {
    clsma::optional_value condition = transform_expr(block, if_stmt->getCond());
    z3::expr condition_expr = condition.has_value() ? condition.value() : unknown(ctx.z3.bool_sort());
    clsma::block* then_block = block->make_inner(condition_expr);
    bool then_return = !process_stmt(then_block, if_stmt->getThen(), ret_ref);
    bool else_return = false;
    if (if_stmt->hasElseStorage()) {
        clsma::block* else_block = block->make_inner(!condition_expr);
        else_return = !process_stmt(else_block, if_stmt->getElse(), ret_ref);
    }
    block->join();
    if (then_return != else_return) {
        block->assume(then_return ? !condition_expr : condition_expr);
    }
    return !then_return || !else_return;
}

z3::expr clsma::ast_visitor::unknown(const z3::sort& sort) {
    return ctx.z3.constant(("unknown_" + std::to_string(unknowns++)).c_str(), sort);
}

bool clsma::ast_visitor::process_while_stmt(clsma::block* block, const clang::WhileStmt* while_stmt,
                                            clsma::value_reference& ret_ref) {
    for (int i = 0; i < ctx.parameters.options.loop_unwinding_iterations_limit; ++i) {
        clsma::optional_value condition = transform_expr(block, while_stmt->getCond());
        z3::expr condition_expr = condition.has_value() ? condition.value() : unknown(ctx.z3.bool_sort());
        if (block->check(condition_expr) != z3::sat) {
            break;
        }
        if (while_stmt->getBody() != nullptr) {
            clsma::block* body_block = block->make_inner(condition_expr);
            bool body_return = !process_stmt(body_block, while_stmt->getBody(), ret_ref);
            block->join();
            if (body_return) {
                block->assume(!condition_expr);
                break;
            }
        }
    }
    return true;
}

bool clsma::ast_visitor::process_do_stmt(clsma::block* block, const clang::DoStmt* do_stmt,
                                         clsma::value_reference& ret_ref) {
    std::optional<z3::expr> condition_expr;
    for (int i = 0; i < ctx.parameters.options.loop_unwinding_iterations_limit; ++i) {
        clsma::block* body_block = block->make_inner();
        bool body_return = !process_stmt(body_block, do_stmt->getBody(), ret_ref);
        block->join();
        if (body_return) {
            if (condition_expr.has_value()) {
                block->assume(!condition_expr.value());
                break;
            } else {
                return false;
            }
        }
        clsma::optional_value condition = transform_expr(block, do_stmt->getCond());
        condition_expr = condition.has_value() ? condition.value() : unknown(ctx.z3.bool_sort());
        if (block->check(condition_expr.value()) != z3::sat) {
            break;
        }
    }
    return true;
}

bool clsma::ast_visitor::process_for_stmt(clsma::block* block, const clang::ForStmt* for_stmt,
                                          clsma::value_reference& ret_ref) {
    clsma::block* for_block = block->make_inner();
    if (for_stmt->getInit() != nullptr) {
        process_stmt(for_block, for_stmt->getInit(), ret_ref);
    }
    for (int i = 0; i < ctx.parameters.options.loop_unwinding_iterations_limit; ++i) {
        std::optional<z3::expr> condition_expr;
        if (for_stmt->getCond() != nullptr) {
            clsma::optional_value condition = transform_expr(block, for_stmt->getCond());
            condition_expr = condition.has_value() ? condition.value() : unknown(ctx.z3.bool_sort());
            if (block->check(condition_expr.value()) != z3::sat) {
                break;
            }
        }
        clsma::block* body_block = block->make_inner(condition_expr);
        bool body_return = !process_stmt(body_block, for_stmt->getBody(), ret_ref);
        block->join();
        if (body_return) {
            if (condition_expr.has_value()) {
                block->assume(!condition_expr.value());
                break;
            } else {
                return false;
            }
        }
        if (for_stmt->getInc() != nullptr) {
            transform_expr(for_block, for_stmt->getInc());
        }
    }
    return true;
}

clsma::optional_value clsma::ast_visitor::handle_get_work_dim(const std::vector<clsma::optional_value>& args) {
    return ctx.z3.int_val(ctx.parameters.work_dim);
}

clsma::optional_value clsma::ast_visitor::handle_get_global_size(const std::vector<clsma::optional_value>& args) {
    if (args.size() != 1 || !args[0].has_value()) {
        z3::expr result = unknown(ctx.z3.int_sort());
        z3::expr condition = result == ctx.z3.int_val(1);
        for (auto&& global_size: ctx.parameters.global_work_size) {
            condition = condition || result == ctx.z3.int_val(uint64_t(global_size));
        }
        return result;
    }
    z3::expr idx = args[0].value();
    z3::expr result = ctx.z3.int_val(1);
    for (uint64_t i = 0; i < ctx.parameters.global_work_size.size(); ++i) {
        result = z3::ite(idx == ctx.z3.int_val(i), ctx.z3.int_val(uint64_t(ctx.parameters.global_work_size[i])),
            result);
    }
    return result;
}

clsma::optional_value clsma::ast_visitor::handle_get_global_id(const std::vector<clsma::optional_value>& args) {
    if (args.size() != 1 || !args[0].has_value()) {
        z3::expr result = unknown(ctx.z3.int_sort());
        z3::expr condition = result == ctx.z3.int_val(0);
        for (auto&& global_id: global_ids) {
            condition = condition || result == global_id->to_z3_expr();
        }
        return result;
    }
    z3::expr idx = args[0].value();
    z3::expr result = ctx.z3.int_val(0);
    for (uint64_t i = 0; i < global_ids.size(); ++i) {
        result = z3::ite(idx == ctx.z3.int_val(i), global_ids[i]->to_z3_expr(), result);
    }
    return result;
}

clsma::optional_value clsma::ast_visitor::handle_get_local_size(const std::vector<clsma::optional_value>& args) {
    if (args.size() != 1 || !args[0].has_value()) {
        z3::expr result = unknown(ctx.z3.int_sort());
        z3::expr condition = result == ctx.z3.int_val(1);
        for (auto&& local_size: local_sizes) {
            condition = condition || result == local_size->to_z3_expr();
        }
        return result;
    }
    z3::expr idx = args[0].value();
    z3::expr result = ctx.z3.int_val(1);
    for (uint64_t i = 0; i < local_sizes.size(); ++i) {
        result = z3::ite(idx == ctx.z3.int_val(i), ctx.z3.int_val(uint64_t(local_sizes[i]->to_z3_expr())), result);
    }
    return result;
}

clsma::optional_value clsma::ast_visitor::handle_get_local_id(const std::vector<clsma::optional_value>& args) {
    if (args.size() != 1 || !args[0].has_value()) {
        z3::expr result = unknown(ctx.z3.int_sort());
        z3::expr condition = result == ctx.z3.int_val(0);
        for (auto&& local_id: local_ids) {
            condition = condition || result == local_id->to_z3_expr();
        }
        return result;
    }
    z3::expr idx = args[0].value();
    z3::expr result = ctx.z3.int_val(0);
    for (uint64_t i = 0; i < local_ids.size(); ++i) {
        result = z3::ite(idx == ctx.z3.int_val(i), local_ids[i]->to_z3_expr(), result);
    }
    return result;
}

clsma::optional_value clsma::ast_visitor::handle_get_group_id(const std::vector<clsma::optional_value>& args) {
    if (args.size() != 1 || !args[0].has_value()) {
        return {};
    }
    z3::expr dimindx = args[0].value();
    return clsma::optional_value();
}

clsma::optional_value clsma::ast_visitor::handle_get_global_offset(const std::vector<clsma::optional_value>& args) {
    if (args.size() != 1 || !args[0].has_value()) {
        z3::expr result = unknown(ctx.z3.int_sort());
        z3::expr condition = result == ctx.z3.int_val(0);
        for (auto&& global_offset : global_offsets) {
            condition = condition || result == global_offset->to_z3_expr();
        }
        return result;
    }
    z3::expr idx = args[0].value();
    z3::expr result = ctx.z3.int_val(0);
    for (uint64_t i = 0; i < global_offsets.size(); ++i) {
        result = z3::ite(idx == ctx.z3.int_val(i), ctx.z3.int_val(uint64_t(global_offsets[i]->to_z3_expr())), result);
    }
    return result;
}

clsma::optional_value clsma::ast_visitor::handle_get_global_linear_id(const std::vector<clsma::optional_value>& args) {
    if (!args.empty() || ctx.parameters.global_work_size.empty()) {
        return {};
    }
    z3::expr size = ctx.z3.int_val(1);
    z3::expr result = ctx.z3.int_val(0);
    for (uint64_t i = 0; i < ctx.parameters.global_work_size.size(); ++i) {
        result = result + global_ids[i]->to_z3_expr() * size;
        size = size * (ctx.z3.int_val(ctx.parameters.global_work_size[i]) - global_offsets[i]->to_z3_expr());
    }
    return result;
}

clsma::optional_value clsma::ast_visitor::handle_get_local_linear_id(const std::vector<clsma::optional_value>& args) {
    if (!args.empty() || local_sizes.empty()) {
        return {};
    }
    z3::expr size = ctx.z3.int_val(1);
    z3::expr result = ctx.z3.int_val(0);
    for (uint64_t i = 0; i < local_sizes.size(); ++i) {
        result = result + local_ids[i]->to_z3_expr() * size;
        size = size * local_sizes[i]->to_z3_expr();
    }
    return result;
}
