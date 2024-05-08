#include "ast_visitor.h"

#include <iostream>
#include <unordered_map>

#include <z3++.h>

#if defined(__APPLE__) || defined(__MACOSX)
#include <OpenCL/cl.h>
#else
#include <CL/cl.h>
#endif

#include "pseudocl.h"

namespace {
    z3::expr to_bool(z3::context& ctx, const z3::expr& expr) {
        if (expr.get_sort().is_arith() || expr.get_sort().is_fpa()) {
            return expr != ctx.num_val(0, expr.get_sort());
        }
        return expr;
    }

    z3::expr to_numeral(z3::context& ctx, const z3::expr& expr) {
        if (expr.is_bool()) {
            return z3::ite(expr, ctx.int_val(1), ctx.int_val(0));
        }
        return expr;
    }

    z3::expr cast(z3::context& ctx, const z3::expr& expr, const clang::QualType& type) {
        if (type->isBooleanType() && (expr.is_arith() || expr.is_fpa())) {
            return to_bool(ctx, expr);
        } else if (type->isIntegerType() && expr.is_bool()) {
            return to_numeral(ctx, expr);
        }
        return expr;
    }
}

clsa::ast_visitor::ast_visitor(analyzer_context& ctx) : ctx(ctx) {
    global_block = ctx.block.make_block();
}

void clsa::ast_visitor::set_violation_handler(std::function<void(clsa::violation)> handler) {
    violation_handler = std::move(handler);
}

void clsa::ast_visitor::add_checker(std::unique_ptr<clsa::abstract_checker> checker) {
    checkers.emplace_back(std::move(checker));
}

bool clsa::ast_visitor::VisitFunctionDecl(clang::FunctionDecl* f) {
    if (!f->hasAttr<clang::OpenCLKernelAttr>() || !f->hasBody() || f->getName() != ctx.parameters.kernel_name) {
        return true;
    }
    clsa::block* kernel_block = global_block->make_inner();

    const bool is_local_size_specified = ctx.parameters.local_work_size.has_value();
    for (uint32_t i = 0; i < ctx.parameters.work_dim; ++i) {
        const std::function<std::string(std::string_view)> id = [id = std::to_string(i)](std::string_view name) {
            return std::string(name).append("#").append(id);
        };

        const std::uint64_t global_work_size = ctx.parameters.global_work_size[i];
        const std::uint64_t local_work_size = is_local_size_specified ? ctx.parameters.local_work_size->at(i) : 0;

        const bool is_uniform = is_local_size_specified && global_work_size % local_work_size == 0;

        const auto& global_size = global_block->value_decl(id("global_size"), ctx.z3.int_sort());
        global_block->assume(global_size->to_z3_expr() == ctx.z3.int_val(global_work_size));
        global_sizes.push_back(global_size);

        const auto& global_id = global_block->value_decl(id("global_id"), ctx.z3.int_sort());
        global_block->assume(
            global_id->to_z3_expr() >= 0 && global_id->to_z3_expr() < ctx.z3.int_val(global_work_size));
        global_ids.push_back(global_id);

        const auto& global_id_copy = global_block->value_decl(id("global_id_copy"), ctx.z3.int_sort());
        global_block->assume(
            global_id_copy->to_z3_expr() >= 0 && global_id_copy->to_z3_expr() < ctx.z3.int_val(global_work_size));
        global_ids_copy.push_back(global_id_copy);

        global_block->assume(global_id_copy->to_z3_expr() != global_id->to_z3_expr());

        const auto& local_size = global_block->value_decl(id("local_size"), ctx.z3.int_sort());
        if (is_uniform) {
            global_block->assume(local_size->to_z3_expr() == ctx.z3.int_val(local_work_size));
        } else {
            global_block->assume(local_size->to_z3_expr() >= 1 && local_size->to_z3_expr() <= ctx.z3.int_val(
                is_local_size_specified ? local_work_size : global_work_size));
        }
        local_sizes.push_back(local_size);

        const auto& enqueued_local_size = global_block->value_decl(id("enqueued_local_size"), ctx.z3.int_sort());
        if (is_local_size_specified) {
            global_block->assume(enqueued_local_size->to_z3_expr() == ctx.z3.int_val(local_work_size));
        } else {
            global_block->assume(enqueued_local_size->to_z3_expr() == local_size->to_z3_expr());
        }
        enqueued_local_sizes.push_back(enqueued_local_size);

        const auto& local_id = global_block->value_decl(id("local_id"), ctx.z3.int_sort());
        global_block->assume(local_id->to_z3_expr() >= 0 && local_id->to_z3_expr() < local_size->to_z3_expr());
        local_ids.push_back(local_id);

        const auto& local_id_copy = global_block->value_decl(id("local_id_copy"), ctx.z3.int_sort());
        global_block->assume(local_id_copy->to_z3_expr() >= 0 && local_id_copy->to_z3_expr() < local_size->to_z3_expr());
        local_ids_copy.push_back(local_id_copy);

        global_block->assume(local_id->to_z3_expr() != local_id_copy->to_z3_expr());

        const auto& num_groups = global_block->value_decl(id("num_groups"), ctx.z3.int_sort());
        global_block->assume(
            num_groups->to_z3_expr() * local_size->to_z3_expr() > global_size->to_z3_expr() - local_size->to_z3_expr());
        global_block->assume(
            num_groups->to_z3_expr() * local_size->to_z3_expr() < global_size->to_z3_expr() + local_size->to_z3_expr());
        group_nums.push_back(num_groups);

        const auto& group_id = global_block->value_decl(id("group_id"), ctx.z3.int_sort());
        global_block->assume(group_id->to_z3_expr() >= 0 && group_id->to_z3_expr() < num_groups->to_z3_expr());
        group_ids.push_back(group_id);

        const auto& global_offset = global_block->value_decl(id("global_offset"), ctx.z3.int_sort());
        if (ctx.parameters.global_work_offset.has_value()) {
            global_block->assume(
                global_offset->to_z3_expr() == ctx.z3.int_val(uint64_t(ctx.parameters.global_work_offset.value()[i])));
        } else {
            global_block->assume(global_offset->to_z3_expr() == 0);
        }
        global_offsets.push_back(global_offset);
    }

    for (unsigned i = 0; i < f->getNumParams(); ++i) {
        const clang::ParmVarDecl* decl = f->getParamDecl(i);
        const clang::QualType& type = decl->getType();
        const clsa::variable* var = kernel_block->var_decl(decl);
        if (i >= ctx.parameters.args.size()) {
            continue;
        }
        const auto& [size, data] = ctx.parameters.args[i];
        if (size != ctx.ast.getTypeSizeInChars(type).getQuantity()) {
            continue;
        }
        if (type->isPointerType() && size == sizeof(cl_mem)) {
            const auto element_size = ctx.ast.getTypeSizeInChars(type->getPointeeType()).getQuantity();
            size_t mem_size;
            if (clsa::pseudocl_is_valid_mem_object(*reinterpret_cast<clsa::pseudocl_mem*>(data))) {
                mem_size = clsa::pseudocl_get_mem_object_size(*reinterpret_cast<clsa::pseudocl_mem*>(data));
            } else {
                cl_int err = clGetMemObjectInfo(*reinterpret_cast<cl_mem*>(data), CL_MEM_SIZE, sizeof(mem_size),
                    &mem_size, nullptr);
                if (err != 0) {
                    continue;
                }
            }
            const uint64_t array_size = mem_size / element_size;
            const uint64_t base = ctx.block.allocate(array_size);
            kernel_block->var_set(decl, clsa::optional_value(ctx.z3.int_val(base), {
                {clsa::VAR_META_MEM_BASE, ctx.z3.int_val(base)},
                {clsa::VAR_META_MEM_SIZE, ctx.z3.int_val(array_size)}
            }));
            if (ctx.parameters.options.array_values) {
                for (uint64_t offset = 0; offset < array_size; ++offset) {
                    global_block->write(ctx.z3.int_val(base + offset), ctx.z3.int_val(0));
                }
            }
        } else if (type->isIntegerType()) {
            z3::expr value = ctx.z3.int_val(0);
            switch (size) {
                case 1:
                    if (type->isUnsignedIntegerType()) {
                        value = ctx.z3.int_val(*reinterpret_cast<uint8_t*>(data));
                    } else {
                        value = ctx.z3.int_val(*reinterpret_cast<int8_t*>(data));
                    }
                    break;
                case 2:
                    if (type->isUnsignedIntegerType()) {
                        value = ctx.z3.int_val(*reinterpret_cast<uint16_t*>(data));
                    } else {
                        value = ctx.z3.int_val(*reinterpret_cast<int16_t*>(data));
                    }
                    break;
                case 4:
                    if (type->isUnsignedIntegerType()) {
                        value = ctx.z3.int_val(*reinterpret_cast<uint32_t*>(data));
                    } else {
                        value = ctx.z3.int_val(*reinterpret_cast<int32_t*>(data));
                    }
                    break;
                case 8:
                    if (type->isUnsignedIntegerType()) {
                        value = ctx.z3.int_val(*reinterpret_cast<uint64_t*>(data));
                    } else {
                        value = ctx.z3.int_val(*reinterpret_cast<int64_t*>(data));
                    }
                    break;
                default:
                    continue;
            }
            kernel_block->var_set(decl, value);
        }
    }
    process_stmt(kernel_block, f->getBody(), *global_block->value_decl("return", f->getReturnType()));

    // TODO: remove for release
    for (const auto& assertion : ctx.solver.assertions()) {
        std::cout << assertion.to_string() << std::endl;
    }
    try {
        std::cout << ctx.solver.check() << std::endl;
    } catch (z3::exception& ex) {
        std::cerr << ex.msg() << std::endl;
    }
    //

    return false;
}

// https://clang.llvm.org/doxygen/classclang_1_1Stmt.html
bool clsa::ast_visitor::process_stmt(clsa::block* block, const clang::Stmt* stmt, clsa::value_reference& ret_ref) {
    // todo: remove (for debug)
    // std::string str;
    // llvm::raw_string_ostream rso(str);
    // stmt->dump(rso, ctx.ast);
    // std::cout << "llvm dump: "<< str;
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

bool clsa::ast_visitor::process_compound_stmt(clsa::block* block, const clang::CompoundStmt* compound_stmt,
                                              clsa::value_reference& ret_ref) {
    for (clang::Stmt* const* stmt_ptr = compound_stmt->body_begin();
         stmt_ptr != compound_stmt->body_end(); ++stmt_ptr) {
        if (!process_stmt(block, *stmt_ptr, ret_ref)) {
            return false;
        }
    }
    return true;
}

bool clsa::ast_visitor::process_while_stmt(clsa::block* block, const clang::WhileStmt* while_stmt,
                                           clsa::value_reference& ret_ref) {
    for (int i = 0; i < ctx.parameters.options.loop_unwinding_iterations_limit; ++i) {
        clsa::optional_value condition = transform_expr(block, while_stmt->getCond(), false);
        z3::expr condition_expr = condition.has_value() ? to_bool(ctx.z3, condition.value()) : unknown(ctx.z3.bool_sort());
        if (block->check(condition_expr) != z3::sat) {
            break;
        }
        if (while_stmt->getBody() != nullptr) {
            clsa::block* body_block = block->make_inner(condition_expr);
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

bool clsa::ast_visitor::process_do_stmt(clsa::block* block, const clang::DoStmt* do_stmt,
                                        clsa::value_reference& ret_ref) {
    std::optional<z3::expr> condition_expr;
    for (int i = 0; i < ctx.parameters.options.loop_unwinding_iterations_limit; ++i) {
        clsa::block* body_block = block->make_inner();
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
        clsa::optional_value condition = transform_expr(block, do_stmt->getCond(), false);
        condition_expr = condition.has_value() ? to_bool(ctx.z3, condition.value()) : unknown(ctx.z3.bool_sort());
        if (block->check(condition_expr.value()) != z3::sat) {
            break;
        }
    }
    return true;
}

bool clsa::ast_visitor::process_for_stmt(clsa::block* block, const clang::ForStmt* for_stmt,
                                         clsa::value_reference& ret_ref) {
    clsa::block* for_block = block->make_inner();
    if (for_stmt->getInit() != nullptr) {
        process_stmt(for_block, for_stmt->getInit(), ret_ref);
    }
    for (int i = 0; i < ctx.parameters.options.loop_unwinding_iterations_limit; ++i) {
        std::optional<z3::expr> condition_expr;
        if (for_stmt->getCond() != nullptr) {
            clsa::optional_value condition = transform_expr(for_block, for_stmt->getCond(), false);
            condition_expr = condition.has_value() ? to_bool(ctx.z3, condition.value()) : unknown(ctx.z3.bool_sort());
            if (for_block->check(condition_expr.value()) != z3::sat) {
                break;
            }
        }
        clsa::block* body_block = for_block->make_inner(condition_expr);
        bool body_return = !process_stmt(body_block, for_stmt->getBody(), ret_ref);
        for_block->join();
        if (body_return) {
            if (condition_expr.has_value()) {
                for_block->assume(!condition_expr.value());
                break;
            } else {
                return false;
            }
        }
        if (for_stmt->getInc() != nullptr) {
            transform_expr(for_block, for_stmt->getInc(), false);
        }
    }
    block->join();
    return true;
}

bool clsa::ast_visitor::process_if_stmt(clsa::block* block, const clang::IfStmt* if_stmt,
                                        clsa::value_reference& ret_ref) {
    clsa::optional_value condition = transform_expr(block, if_stmt->getCond(), false);
    z3::expr condition_expr = condition.has_value() ? to_bool(ctx.z3, condition.value()) : unknown(ctx.z3.bool_sort());
    clsa::block* then_block = block->make_inner(condition_expr);
    bool then_return = !process_stmt(then_block, if_stmt->getThen(), ret_ref);
    bool else_return = false;
    if (if_stmt->hasElseStorage()) {
        clsa::block* else_block = block->make_inner(!condition_expr);
        else_return = !process_stmt(else_block, if_stmt->getElse(), ret_ref);
    }
    block->join();
    if (then_return != else_return) {
        block->assume(then_return ? !condition_expr : condition_expr);
    }
    return !then_return || !else_return;
}

bool clsa::ast_visitor::process_return_stmt(clsa::block* block, const clang::ReturnStmt* return_stmt,
                                            clsa::value_reference& ret_ref) {
    if (return_stmt->getRetValue()) {
        const auto expr = transform_expr(block, return_stmt->getRetValue(), false);
        if (expr.has_value()) {
            block->value_set(&ret_ref, expr.value());
        }
    }
    return false;
}

void clsa::ast_visitor::process_decl_stmt(clsa::block* block, const clang::DeclStmt* decl_stmt,
                                          clsa::value_reference& ret_ref) {
    std::for_each(decl_stmt->decl_begin(), decl_stmt->decl_end(), [&](clang::Decl* decl) {
        process_decl(block, decl, ret_ref);
    });
}

void clsa::ast_visitor::process_decl(clsa::block* block, const clang::Decl* decl, clsa::value_reference& ret_ref) {
    switch (decl->getKind()) {
        case clang::Decl::Var:
            process_var_decl(block, clang::cast<clang::VarDecl>(decl), ret_ref);
            break;
        default:
            break;
    }
}

void clsa::ast_visitor::process_var_decl(clsa::block* block, const clang::VarDecl* var_decl,
                                         clsa::value_reference& ret_ref) {
    if (var_decl->getType()->isConstantArrayType()) {
        const uint64_t size = ctx.ast.getConstantArrayElementCount(ctx.ast.getAsConstantArrayType(var_decl->getType()));
        const uint64_t base = ctx.block.allocate(size);
        block->var_decl(var_decl, clsa::optional_value(ctx.z3.int_val(base), {
            {clsa::VAR_META_MEM_BASE, ctx.z3.int_val(base)},
            {clsa::VAR_META_MEM_SIZE, ctx.z3.int_val(size)}
        }));
        if (ctx.parameters.options.array_values) {
            for (uint64_t offset = 0; offset < size; ++offset) {
                global_block->write(ctx.z3.int_val(base + offset), ctx.z3.int_val(0)); // TODO: respect actual values
            }
        }
    } else {
        block->var_decl(var_decl,
            var_decl->hasInit() ? transform_expr(block, var_decl->getInit(), false) : clsa::optional_value());
    }
}

clsa::optional_value clsa::ast_visitor::transform_value_stmt(clsa::block* block, const clang::ValueStmt* value_stmt) {
    if (clang::isa<clang::Expr>(value_stmt)) {
        return transform_expr(block, clang::cast<clang::Expr>(value_stmt), false);
    } else {
        return {};
    }
}

// https://clang.llvm.org/doxygen/classclang_1_1Expr.html
clsa::optional_value clsa::ast_visitor::transform_expr(clsa::block* block, const clang::Expr* expr, bool copy_thread) {
    expr = expr->IgnoreParens();
    if (clang::isa<clang::CastExpr>(expr)) {
        return transform_cast_expr(block, clang::cast<clang::CastExpr>(expr), copy_thread);
    } else if (clang::isa<clang::ArraySubscriptExpr>(expr)) {
        return transform_array_subscript_expr(block, clang::cast<clang::ArraySubscriptExpr>(expr), copy_thread);
    } else if (clang::isa<clang::BinaryOperator>(expr)) {
        return transform_binary_operator(block, clang::cast<clang::BinaryOperator>(expr), false);
    } else if (clang::isa<clang::CallExpr>(expr)) {
        return transform_call_expr(block, clang::cast<clang::CallExpr>(expr), copy_thread);
    } else if (clang::isa<clang::DeclRefExpr>(expr)) {
        return transform_decl_ref_expr(block, clang::cast<clang::DeclRefExpr>(expr), copy_thread);
    } else if (clang::isa<clang::IntegerLiteral>(expr)) {
        return ctx.z3.int_val(clang::cast<clang::IntegerLiteral>(expr)->getValue().getLimitedValue());
    } else if (clang::isa<clang::CXXBoolLiteralExpr>(expr)) {
        return ctx.z3.bool_val(clang::cast<clang::CXXBoolLiteralExpr>(expr)->getValue());
    } else if (clang::isa<clang::UnaryOperator>(expr)) {
        return transform_unary_operator(block, clang::cast<clang::UnaryOperator>(expr), copy_thread);
    } else {
        return {};
    }
}

clsa::optional_value clsa::ast_visitor::transform_cast_expr(clsa::block* block, const clang::CastExpr* cast_expr, bool copy_thread) {
    clsa::optional_value sub_expr = transform_expr(block, cast_expr->getSubExpr(), copy_thread);
    if (!sub_expr.has_value()) {
        return {};
    }
    sub_expr.set_value(cast(ctx.z3, sub_expr.value(), cast_expr->getType()));
    return sub_expr;
}

clsa::optional_value clsa::ast_visitor::transform_array_subscript_expr(clsa::block* block,
                                                                       const clang::ArraySubscriptExpr* array_subscript_expr, bool copy_thread) {
    auto&& lhs = transform_expr(block, array_subscript_expr->getBase(), copy_thread);
    auto&& rhs = transform_expr(block, array_subscript_expr->getIdx(), copy_thread);
    auto&& rhs_copy = transform_expr(block, array_subscript_expr->getIdx(), !copy_thread);
    if (!lhs.has_value() || !rhs.has_value()) {
        return {};
    }
    if (!copy_thread)
    {
        check_memory_access(block, array_subscript_expr, clsa::memory_access_type::read, lhs.value() + rhs.value(), {}, {}, lhs.value() + rhs_copy.value());
    }
    if (ctx.parameters.options.array_values) {
        return block->read(lhs.value() + rhs.value(), array_subscript_expr->getType());
    } else {
        return lhs.value() + rhs.value();
    }
}

clsa::optional_value clsa::ast_visitor::transform_binary_operator(clsa::block* block,
                                                                  const clang::BinaryOperator* binary_operator, bool copy_thread) {
    auto&& lhs = binary_operator->getOpcode() != clang::BO_Assign
        ? transform_expr(block, binary_operator->getLHS(), copy_thread) : clsa::optional_value();
    auto&& lhs_copy = binary_operator->getOpcode() != clang::BO_Assign
        ? transform_expr(block, binary_operator->getLHS(), !copy_thread) : clsa::optional_value();
    auto&& rhs = transform_expr(block, binary_operator->getRHS(), copy_thread);
    auto&& rhs_copy = transform_expr(block, binary_operator->getRHS(), !copy_thread);
    const clang::QualType type = binary_operator->getType();
    clsa::optional_value value;
    clsa::optional_value value_copy;
    if ((lhs.has_value() || binary_operator->getOpcode() == clang::BO_Assign) && rhs.has_value()) {
        switch (binary_operator->getOpcode()) {
            case clang::BO_Assign:
            case clang::BO_Comma: {
                value = rhs;
                value_copy = rhs_copy;
                break;
            }
            case clang::BO_Add:
            case clang::BO_AddAssign: {
                value = lhs.value() + rhs.value();
                value_copy = lhs_copy.value() + rhs_copy.value();
                break;
            }
            case clang::BO_Sub:
            case clang::BO_SubAssign: {
                value = lhs.value() - rhs.value();
                value_copy = lhs_copy.value() - rhs_copy.value();
                break;
            }
            case clang::BO_Mul:
            case clang::BO_MulAssign: {
                value = lhs.value() * rhs.value();
                value_copy = lhs_copy.value() * rhs_copy.value();
                break;
            }
            case clang::BO_Div:
            case clang::BO_DivAssign: {
                value = lhs.value() / rhs.value();
                value_copy = lhs_copy.value() / rhs_copy.value();
                break;
            }
            case clang::BO_Rem:
            case clang::BO_RemAssign: {
                value = lhs.value() % rhs.value();
                value_copy = lhs_copy.value() % rhs_copy.value();
                break;
            }
            case clang::BO_LAnd: {
                value = cast(ctx.z3, to_bool(ctx.z3, lhs.value()) && to_bool(ctx.z3, rhs.value()), type);
                value_copy = cast(ctx.z3, to_bool(ctx.z3, lhs_copy.value()) && to_bool(ctx.z3, rhs_copy.value()), type);
                break;
            }
            case clang::BO_LOr: {
                value = cast(ctx.z3, to_bool(ctx.z3, lhs.value()) || to_bool(ctx.z3, rhs.value()), type);
                value_copy = cast(ctx.z3, to_bool(ctx.z3, lhs_copy.value()) || to_bool(ctx.z3, rhs_copy.value()), type);
                break;
            }
            case clang::BO_EQ: {
                value = lhs.value() == rhs.value();
                value_copy = lhs_copy.value() == rhs_copy.value();
                break;
            }
            case clang::BO_GE: {
                value = to_numeral(ctx.z3, lhs.value()) >= to_numeral(ctx.z3, rhs.value());
                value_copy = to_numeral(ctx.z3, lhs_copy.value()) >= to_numeral(ctx.z3, rhs_copy.value());
                break;
            }
            case clang::BO_GT: {
                value = to_numeral(ctx.z3, lhs.value()) > to_numeral(ctx.z3, rhs.value());
                value_copy = to_numeral(ctx.z3, lhs_copy.value()) > to_numeral(ctx.z3, rhs_copy.value());
                break;
            }
            case clang::BO_LE: {
                value = to_numeral(ctx.z3, lhs.value()) <= to_numeral(ctx.z3, rhs.value());
                value_copy = to_numeral(ctx.z3, lhs_copy.value()) <= to_numeral(ctx.z3, rhs_copy.value());
                break;
            }
            case clang::BO_LT: {
                value = to_numeral(ctx.z3, lhs.value()) < to_numeral(ctx.z3, rhs.value());
                value_copy = to_numeral(ctx.z3, lhs_copy.value()) < to_numeral(ctx.z3, rhs_copy.value());
                break;
            }
            case clang::BO_NE: {
                value = lhs.value() != rhs.value();
                value_copy = lhs_copy.value() != rhs_copy.value();
                break;
            }
            default: {
                break;
            }
        }
    }
    if (type->isPointerType()) {
        auto&& ptr = binary_operator->getLHS()->getType()->isPointerType() ? lhs : rhs;
        value.set_metadata(clsa::VAR_META_MEM_BASE, ptr.metadata(clsa::VAR_META_MEM_BASE));
        value.set_metadata(clsa::VAR_META_MEM_SIZE, ptr.metadata(clsa::VAR_META_MEM_SIZE));
        value_copy.set_metadata(clsa::VAR_META_MEM_BASE, ptr.metadata(clsa::VAR_META_MEM_BASE));
        value_copy.set_metadata(clsa::VAR_META_MEM_SIZE, ptr.metadata(clsa::VAR_META_MEM_SIZE));
    }
    if (binary_operator->isAssignmentOp()) {
        assign(block, binary_operator->getLHS(), value, value_copy, copy_thread);
    }
    return value;
}

clsa::optional_value clsa::ast_visitor::transform_call_expr(clsa::block* block, const clang::CallExpr* call_expr, bool copy_thread) {
    static std::unordered_map<std::string, clsa::optional_value (clsa::ast_visitor::*)(
        // todo refactor (merge)
        const std::vector<clsa::optional_value>&)> builtin_handlers = {
        {"get_work_dim",            &clsa::ast_visitor::handle_get_work_dim},
        {"get_global_size",         &clsa::ast_visitor::handle_get_global_size},
        {"get_global_id",           &clsa::ast_visitor::handle_get_global_id},
        {"get_local_size",          &clsa::ast_visitor::handle_get_local_size},
        {"get_enqueued_local_size", &clsa::ast_visitor::handle_get_enqueued_local_size},
        {"get_local_id",            &clsa::ast_visitor::handle_get_local_id},
        {"get_num_groups",          &clsa::ast_visitor::handle_get_num_groups},
        {"get_group_id",            &clsa::ast_visitor::handle_get_group_id},
        {"get_global_offset",       &clsa::ast_visitor::handle_get_global_offset},
        {"get_global_linear_id",    &clsa::ast_visitor::handle_get_global_linear_id},
        {"get_local_linear_id",     &clsa::ast_visitor::handle_get_local_linear_id},
    };
    static std::unordered_map<std::string, clsa::optional_value (clsa::ast_visitor::*)(
        const std::vector<clsa::optional_value>&)> builtin_handlers_for_copy_thread = {
        {"get_work_dim",            &clsa::ast_visitor::handle_get_work_dim},
        {"get_global_size",         &clsa::ast_visitor::handle_get_global_size},
        {"get_global_id",           &clsa::ast_visitor::handle_get_global_id_copy},
        {"get_local_size",          &clsa::ast_visitor::handle_get_local_size},
        {"get_enqueued_local_size", &clsa::ast_visitor::handle_get_enqueued_local_size},
        {"get_local_id",            &clsa::ast_visitor::handle_get_local_id_copy},
        {"get_num_groups",          &clsa::ast_visitor::handle_get_num_groups},
        {"get_group_id",            &clsa::ast_visitor::handle_get_group_id},
        {"get_global_offset",       &clsa::ast_visitor::handle_get_global_offset},
        {"get_global_linear_id",    &clsa::ast_visitor::handle_get_global_linear_id},
        {"get_local_linear_id",     &clsa::ast_visitor::handle_get_local_linear_id},
    };
    const clang::Decl* callee_decl = call_expr->getCalleeDecl();
    std::vector<clsa::optional_value> args;
    std::transform(call_expr->getArgs(), call_expr->getArgs() + call_expr->getNumArgs(),
        std::inserter(args, args.begin()), [&](const clang::Expr* expr) {
            return transform_expr(block, expr, copy_thread);
        });
    if (clang::isa<clang::NamedDecl>(callee_decl)) {
        const std::string& name = clang::cast<clang::NamedDecl>(callee_decl)->getName().str();
        if (!copy_thread)
        {
            if (auto it = builtin_handlers.find(name); it != builtin_handlers.end()) {
                return (this->*it->second)(args);
            }
        } else
        {
            if (auto it = builtin_handlers_for_copy_thread.find(name); it != builtin_handlers_for_copy_thread.end()) {
                return (this->*it->second)(args);
            }
        }
    }
    if (clang::isa<clang::FunctionDecl>(callee_decl) && callee_decl->hasBody()) {
        if (func_stack_depth >= ctx.parameters.options.function_calls_depth_limit) {
            return {};
        }
        clsa::block* call_block = global_block->make_inner();
        const auto* function_decl = clang::cast<clang::FunctionDecl>(callee_decl);
        for (unsigned int i = 0; i < function_decl->getNumParams(); ++i) {
            const auto* var = call_block->var_decl(function_decl->getParamDecl(i));
            if (args[i].has_value()) {
                ctx.solver.add(var->to_z3_expr() == args[i].value());
            }
        }
        auto* ret_ref = global_block->value_decl("return", function_decl->getReturnType());
        ++func_stack_depth;
        process_stmt(call_block, callee_decl->getBody(), *ret_ref);
        --func_stack_depth;
        return ret_ref->to_value();
    }
    return {};
}

clsa::optional_value clsa::ast_visitor::transform_decl_ref_expr(clsa::block* block,
                                                                const clang::DeclRefExpr* decl_ref_expr, bool copy_thread) {
    const clsa::variable* var = block->var_get(decl_ref_expr->getDecl());
    return var ? var->to_value() : clsa::optional_value();
}

clsa::optional_value clsa::ast_visitor::transform_unary_operator(clsa::block* block,
                                                                 const clang::UnaryOperator* unary_operator, bool copy_thread) {
    clsa::optional_value sub_expr = transform_expr(block, unary_operator->getSubExpr(), copy_thread);
    if (!sub_expr.has_value()) {
        return {};
    }
    const z3::expr& sub_expr_value = sub_expr.value();
    switch (unary_operator->getOpcode()) {
        case clang::UO_AddrOf: {
            return {};
        }
        case clang::UO_Deref: {
            if (!copy_thread)
            {
                check_memory_access(block, unary_operator, clsa::memory_access_type::read, sub_expr.value(), {}, {}, z3::re_empty(ctx.z3.int_sort()));
            }
            if (ctx.parameters.options.array_values) {
                return block->read(sub_expr.value(), unary_operator->getType());
            }
            return {};
        }
        case clang::UO_LNot: {
            return cast(ctx.z3, !to_bool(ctx.z3, sub_expr_value), unary_operator->getType());
        }
        case clang::UO_Minus: {
            return -to_numeral(ctx.z3, sub_expr_value);
        }
        case clang::UO_Not: {
            if (sub_expr_value.is_bool()) {
                return z3::ite(sub_expr_value, ctx.z3.int_val(0), ctx.z3.int_val(1));
            }
            const clang::QualType sub_expr_type = unary_operator->getSubExpr()->getType();
            return z3::bv2int(~z3::int2bv(ctx.ast.getTypeSize(sub_expr_type), sub_expr_value),
                sub_expr_type->isSignedIntegerType());
        }
        case clang::UO_Plus: {
            return to_numeral(ctx.z3, sub_expr_value);
        }
        case clang::UO_PostDec: {
            // todo omplement value_copy
            assign(block, unary_operator->getSubExpr(), sub_expr.map_value([](auto value) { return value - 1; }), {}, copy_thread);
            return sub_expr;
        }
        case clang::UO_PostInc: {
            // todo omplement value_copy
            assign(block, unary_operator->getSubExpr(), sub_expr.map_value([](auto value) { return value + 1; }), {}, copy_thread);
            return sub_expr;
        }
        case clang::UO_PreDec: {
            sub_expr.set_value(sub_expr.value() - 1);
            // todo omplement value_copy
            assign(block, unary_operator->getSubExpr(), sub_expr, {}, copy_thread);
            return sub_expr;
        }
        case clang::UO_PreInc: {
            sub_expr.set_value(sub_expr.value() + 1);
            // todo omplement value_copy
            assign(block, unary_operator->getSubExpr(), sub_expr, {}, copy_thread);
            return sub_expr;
        }
        default: {
            return {};
        }
    }
}

z3::expr clsa::ast_visitor::unknown(const z3::sort& sort) {
    return ctx.z3.constant(("unknown_" + std::to_string(unknowns++)).c_str(), sort);
}

void clsa::ast_visitor::assign(clsa::block* block,
                                const clang::Expr* lhs,
                                const clsa::optional_value& value,
                                const clsa::optional_value& value_copy,
                                bool copy_thread)
{
    lhs = lhs->IgnoreParenCasts();
    if (clang::isa<clang::ArraySubscriptExpr>(lhs)) {
        const auto* array_subscript_expr = clang::cast<clang::ArraySubscriptExpr>(lhs);
        const auto base = transform_expr(block, array_subscript_expr->getBase(), copy_thread);
        const auto idx = transform_expr(block, array_subscript_expr->getIdx(), copy_thread);
        const auto idx_t2 = transform_expr(block, array_subscript_expr->getIdx(), !copy_thread);
        if (!base.has_value() || !idx.has_value()) {
            return;
        }
        if (!copy_thread)
        {
            check_memory_access(block, lhs, clsa::memory_access_type::write, base.value() + idx.value(), value, value_copy, base.value() + idx_t2.value());
        }
        if (ctx.parameters.options.array_values && value.has_value()) {
            block->write(base.value() + idx.value(), value.value());
        }
    } else if (clang::isa<clang::UnaryOperator>(lhs)) {
        const auto* unary_operator = clang::cast<clang::UnaryOperator>(lhs);
        if (unary_operator->getOpcode() == clang::UO_Deref) {
            const auto sub_expr = transform_expr(block, unary_operator->getSubExpr(), copy_thread);
            if (!sub_expr.has_value()) {
                return;
            }
            // todo omplement value_copy
            if (!copy_thread) {
                check_memory_access(block, lhs, clsa::memory_access_type::write, sub_expr.value(), value, {}, z3::re_empty(ctx.z3.int_sort()));
            }
            if (ctx.parameters.options.array_values && value.has_value()) {
                block->write(sub_expr.value(), value.value());
            }
        }
    } else if (clang::isa<clang::DeclRefExpr>(lhs)) {
        const auto* decl_ref_expr = clang::cast<clang::DeclRefExpr>(lhs);
        block->var_set(decl_ref_expr->getDecl(), value);
    }
}

void clsa::ast_visitor::check_memory_access(const clsa::block* block, const clang::Expr* expr,
                                            clsa::memory_access_type access_type, const z3::expr& address,
                                            const clsa::optional_value& value,
                                            const clsa::optional_value& value_copy, const z3::expr& address_copy) {
    for (auto& checker : checkers) {
        std::optional<clsa::violation> violation = checker->check_memory_access(block, expr, access_type, address, value, value_copy, address_copy);
        if (violation.has_value() && violation_handler) {
            violation_handler(std::move(violation.value()));
        }
    }
}

clsa::optional_value clsa::ast_visitor::get_dim_value(const std::vector<clsa::value_reference*>& values,
                                                      const std::vector<clsa::optional_value>& args,
                                                      const z3::expr& default_value) {
    if (args.size() != 1 || !args[0].has_value()) {
        z3::expr result = unknown(ctx.z3.int_sort());
        z3::expr condition = result == default_value;
        for (const clsa::value_reference* value : values) {
            condition = condition || result == value->to_z3_expr();
        }
        return result;
    }
    z3::expr dimidx = args[0].value();
    z3::expr result = default_value;
    for (uint64_t i = 0; i < values.size(); ++i) {
        result = z3::ite(dimidx == ctx.z3.int_val(i), values[i]->to_z3_expr(), result);
    }
    return result;
}

clsa::optional_value clsa::ast_visitor::handle_get_work_dim(const std::vector<clsa::optional_value>& args) {
    return ctx.z3.int_val(ctx.parameters.work_dim);
}

clsa::optional_value clsa::ast_visitor::handle_get_global_size(const std::vector<clsa::optional_value>& args) {
    return get_dim_value(global_sizes, args, ctx.z3.int_val(1));
}

clsa::optional_value clsa::ast_visitor::handle_get_global_id(const std::vector<clsa::optional_value>& args) {
    return get_dim_value(global_ids, args, ctx.z3.int_val(0));
}

clsa::optional_value clsa::ast_visitor::handle_get_global_id_copy(const std::vector<clsa::optional_value>& args) {
    return get_dim_value(global_ids_copy, args, ctx.z3.int_val(0));
}

clsa::optional_value clsa::ast_visitor::handle_get_local_size(const std::vector<clsa::optional_value>& args) {
    return get_dim_value(local_sizes, args, ctx.z3.int_val(1));
}

clsa::optional_value clsa::ast_visitor::handle_get_enqueued_local_size(
    const std::vector<clsa::optional_value>& args
) {
    return get_dim_value(enqueued_local_sizes, args, ctx.z3.int_val(1));
}

clsa::optional_value clsa::ast_visitor::handle_get_local_id(const std::vector<clsa::optional_value>& args) {
    return get_dim_value(local_ids, args, ctx.z3.int_val(0));
}

clsa::optional_value clsa::ast_visitor::handle_get_local_id_copy(const std::vector<clsa::optional_value>& args) {
    return get_dim_value(local_ids_copy, args, ctx.z3.int_val(0));
}

clsa::optional_value clsa::ast_visitor::handle_get_num_groups(const std::vector<clsa::optional_value>& args) {
    return get_dim_value(group_nums, args, ctx.z3.int_val(1));
}

clsa::optional_value clsa::ast_visitor::handle_get_group_id(const std::vector<clsa::optional_value>& args) {
    return get_dim_value(group_ids, args, ctx.z3.int_val(0));
}

clsa::optional_value clsa::ast_visitor::handle_get_global_offset(const std::vector<clsa::optional_value>& args) {
    return get_dim_value(global_offsets, args, ctx.z3.int_val(0));
}

clsa::optional_value clsa::ast_visitor::handle_get_global_linear_id(const std::vector<clsa::optional_value>& args) {
    if (!args.empty() || ctx.parameters.global_work_size.empty()) {
        return {};
    }
    z3::expr size = ctx.z3.int_val(1);
    z3::expr result = ctx.z3.int_val(0);
    for (uint64_t i = 0; i < ctx.parameters.global_work_size.size(); ++i) {
        result = result + global_ids[i]->to_z3_expr() * size;
        size = size * (global_sizes[i]->to_z3_expr() - global_offsets[i]->to_z3_expr());
    }
    return result;
}

clsa::optional_value clsa::ast_visitor::handle_get_local_linear_id(const std::vector<clsa::optional_value>& args) {
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
