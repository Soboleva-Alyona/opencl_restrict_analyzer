#include "ast_visitor.h"

#include <iostream>
#include <unordered_map>

#include <z3++.h>

#include <OpenCL/cl.h>

/*ast_visitor::ast_visitor(clang::ASTContext& ctx, const analyzer_parameters& parameters)
: ast_ctx(ctx), solver(z3_ctx), mem(solver), block_ctx(ctx, z3_ctx, mem), parameters(parameters),
ctx(analyzer_context{ parameters, ast_ctx, z3_ctx, mem,   } ) { }*/

ast_visitor::ast_visitor(analyzer_context& ctx) : ctx(ctx) {
    checkers.emplace_back(std::make_unique<restrict_checker>(ctx));
    global_block = ctx.block_ctx.make_block();
}

namespace {
    constexpr std::string_view META_BASE = "BASE";
    constexpr std::string_view META_SIZE = "SIZE";
    constexpr std::string_view META_GLOBAL_ID = "GID";
    constexpr std::string_view META_LOCAL_ID = "LID";

    /*static int mem_ver = 1;

    z3::expr cur_mem(z3::context& z3_ctx) {
        return z3_ctx.constant(("$int_mem!!" + std::to_string(mem_ver)).c_str(), z3_ctx.array_sort(z3_ctx.int_sort(), z3_ctx.int_sort()));
    }

    z3::expr mem_read(z3::context& z3_ctx, const z3::expr& address) {
        return z3::select(z3_ctx.constant(("$int_mem!!" + std::to_string(mem_ver)).c_str(), z3_ctx.array_sort(z3_ctx.int_sort(), z3_ctx.int_sort())), address);
    }

    z3::expr mem_write(z3::context& z3_ctx, const z3::expr& address, const z3::expr& optional_value) {
        z3::expr predicate = z3_ctx.constant(("$int_mem!!" + std::to_string(mem_ver + 1)).c_str(), z3_ctx.array_sort(z3_ctx.int_sort(), z3_ctx.int_sort())) == z3::store(z3_ctx.constant(("$mem!!" + std::to_string(mem_ver)).c_str(), z3_ctx.array_sort(z3_ctx.int_sort(), z3_ctx.int_sort())), address, optional_value);
        ++mem_ver;
        return predicate;
    }*/
}

bool ast_visitor::VisitFunctionDecl(clang::FunctionDecl* f) {
    if (!f->hasAttr<clang::OpenCLKernelAttr>() || !f->hasBody() || f->getName() != ctx.parameters.kernel_name) {
        return true;
    }
    clsma::block* kernel_block = global_block->make_inner();
    for (uint32_t i = 0; i < ctx.parameters.work_dim; ++i) {
        const z3::expr global_id =
                ctx.mem.read_meta(META_GLOBAL_ID, ctx.z3_ctx.int_val(i), ctx.z3_ctx.int_sort());
        ctx.solver.add(global_id >= 0 && global_id < ctx.z3_ctx.int_val(uint64_t(ctx.parameters.global_work_size[i])));
    }
    for (unsigned i = 0; i < f->getNumParams(); ++i) {
        const auto& [size, data] = ctx.parameters.args[i];
        const clang::ParmVarDecl* decl = f->getParamDecl(i);
        const clang::QualType& type = decl->getType();
        if (type->isPointerType() && size == sizeof(cl_mem)) {
            const auto element_size = ctx.ast_ctx.getTypeSizeInChars(type->getPointeeType()).getQuantity();
            size_t mem_size;
            cl_int err = clGetMemObjectInfo(*reinterpret_cast<cl_mem*>(data), CL_MEM_SIZE, sizeof(mem_size), &mem_size, nullptr);
            if (err == 0) {
                const uint64_t array_size = mem_size / element_size;
                const uint64_t base = ctx.mem.allocate(array_size);
                const clsma::variable* var = kernel_block->decl_var(decl, clsma::optional_value(ctx.z3_ctx.int_val(base), {
                        {"base", ctx.z3_ctx.int_val(base)},
                        {"size", ctx.z3_ctx.int_val(array_size)}
                }));
                //ctx.solver.add(variable->to_z3_expr() == ctx.z3_ctx.int_val(variable->address));
                if (ctx.parameters.options.array_values) {
                    for (uint64_t i = 0; i < array_size; ++i) {
                        global_block->write(ctx.z3_ctx.int_val(var->address + i), ctx.z3_ctx.int_val(0));
                    }
                }
                //ctx.solver.add(
                //        variable->to_z3_storage_expr() == z3::const_array(ctx.z3_ctx.int_sort(), ctx.z3_ctx.int_val(0)));
                //ctx.mem.write(ctx.z3_ctx.int_val(variable->address), ctx.z3_ctx.int_val(base));
                //ctx.mem.write_meta(META_BASE, ctx.z3_ctx.int_val(variable->address), ctx.z3_ctx.int_val(base));
                //ctx.mem.write_meta(META_SIZE, ctx.z3_ctx.int_val(variable->address), ctx.z3_ctx.int_val(array_size));
                //solver.add(variable->to_z3_expr(z3_ctx) == z3_ctx.int_val(variable->address));
                continue;
            }
        }
    }
    process_stmt(kernel_block, f->getBody(), *kernel_block->value_decl("return", f->getReturnType()));
    for (const auto& assertion : ctx.solver.assertions()) {
        std::cout << assertion.to_string() << std::endl;
    }
    try {
        std::cout << ctx.solver.check() << std::endl;
        //std::cout << ctx.solver.proof().to_string() << std::endl;
    } catch (z3::exception& ex) {
        std::cerr << ex.msg() << std::endl;
    }
    return false;
}

bool ast_visitor::process_stmt(clsma::block* block, const clang::Stmt* stmt, clsma::value_reference& ret_ref) {
    //std::cout << stmt->getStmtClassName() << std::endl;
    if (clang::isa<clang::CompoundStmt>(stmt)) {
        return process_compound_stmt(block, clang::cast<clang::CompoundStmt>(stmt), ret_ref);
    } else if (clang::isa<clang::WhileStmt>(stmt)) {
        return process_while_stmt(block, clang::cast<clang::WhileStmt>(stmt), ret_ref);
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

bool ast_visitor::process_return_stmt(clsma::block* block, const clang::ReturnStmt* return_stmt, clsma::value_reference& ret_ref) {
    if (return_stmt->getRetValue()) {
        const auto expr = transform_expr(block, return_stmt->getRetValue());
        if (expr.has_value()) {
            block->var_set(&ret_ref, expr.value());
        }
    }
    return false;
}

bool ast_visitor::process_compound_stmt(clsma::block* block, const clang::CompoundStmt* compound_stmt, clsma::value_reference& ret_ref) {
    for (clang::Stmt* const* stmt_ptr = compound_stmt->body_begin(); stmt_ptr != compound_stmt->body_end(); ++stmt_ptr) {
        if (!process_stmt(block, *stmt_ptr, ret_ref)) {
            return false;
        }
    }
    return true;
}

void ast_visitor::process_decl_stmt(clsma::block* block, const clang::DeclStmt* decl_stmt, clsma::value_reference& ret_ref) {
    std::for_each(decl_stmt->decl_begin(), decl_stmt->decl_end(), [&](clang::Decl* decl) {
        process_decl(block, decl, ret_ref);
    });
}

void ast_visitor::process_decl(clsma::block* block, const clang::Decl* decl, clsma::value_reference& ret_ref) {
    switch (decl->getKind()) {
        case clang::Decl::Var:
            process_var_decl(block, clang::cast<clang::VarDecl>(decl), ret_ref);
            break;
        default:
            break;
    }
}

void ast_visitor::process_var_decl(clsma::block* block, const clang::VarDecl* var_decl, clsma::value_reference& ret_ref) {
    block->decl_var(var_decl, var_decl->hasInit() ? transform_expr(block, var_decl->getInit()) : clsma::optional_value());
    /*const clsma::variable* var = block->decl_var(var_decl);
    if (var_decl->getAnyInitializer()) {
        const auto initializer = transform_expr(block, var_decl->getAnyInitializer());
        if (initializer.has_value()) {
            const z3::expr address = ctx.z3_ctx.int_val(var->address);
            ctx.mem.write(address, ctx.mem.read(initializer.value(), ctx.z3_ctx.int_sort()));
            if (var_decl->getType()->isPointerType()) {
                //ctx.mem.write_meta(META_BASE, address, ctx.mem.read_meta(META_BASE, initializer.optional_value(), ctx.z3_ctx.int_sort()));
                //ctx.mem.write_meta(META_SIZE, address, ctx.mem.read_meta(META_SIZE, initializer.optional_value(), ctx.z3_ctx.int_sort()));
            }
            //solver.add(mem_write(z3_ctx, z3_ctx.int_val(variable->address), initializer.optional_value()));
            //solver.add(variable->to_z3_expr(z3_ctx) == initializer.optional_value());
        }
    }*/
}

clsma::optional_value ast_visitor::transform_value_stmt(clsma::block* block, const clang::ValueStmt* value_stmt) {
    if (clang::isa<clang::Expr>(value_stmt)) {
        return transform_expr(block, clang::cast<clang::Expr>(value_stmt));
    } else {
        std::cout << "WARN: unknown optional_value stmt type: " << value_stmt->getStmtClassName() << std::endl;
        return {};
    }
}

clsma::optional_value ast_visitor::transform_expr(clsma::block* block, const clang::Expr* expr) {
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
        return ctx.z3_ctx.int_val(clang::cast<clang::IntegerLiteral>(expr)->getValue().getLimitedValue());
        //const z3::expr address = ctx.z3_ctx.int_val(ctx.mem.allocate(1));
        //ctx.mem.write(address, ctx.z3_ctx.int_val(clang::cast<clang::IntegerLiteral>(expr)->getValue().getLimitedValue()));
        //return address;
    } else if (clang::isa<clang::CXXBoolLiteralExpr>(expr)) {
        return ctx.z3_ctx.bool_val(clang::cast<clang::CXXBoolLiteralExpr>(expr)->getValue());
    } else {
        std::cout << "WARN: unknown expr type: " << expr->getStmtClassName() << std::endl;
        return {};
    }
}

clsma::optional_value ast_visitor::transform_array_subscript_expr(clsma::block* block, const clang::ArraySubscriptExpr* array_subscript_expr) {
    auto lhs = transform_expr(block, array_subscript_expr->getBase());
    auto rhs = transform_expr(block, array_subscript_expr->getIdx());
    if (!lhs.has_value() || !rhs.has_value()) {
        return {};
    }
    //const auto base_address = get_address(block, array_subscript_expr->getBase());
    //if (base_address.has_value()) {
    check_memory_access(block, array_subscript_expr, abstract_checker::READ, lhs.value() + rhs.value());
    //}
    //std::cout << "base type: " << array_subscript_expr->getBase()->getType().getAsString() << std::endl;
    //std::cout << "restrict: " << array_subscript_expr->getBase()->getType().isRestrictQualified() << std::endl;
    if (ctx.parameters.options.array_values) {
        return block->read(lhs.value() + rhs.value(), array_subscript_expr->getType());
    } else {
        return {};
    }
    const auto array = get_address(block, array_subscript_expr->getBase());
    if (array.has_value()) {
        return z3::select(array.value(), rhs.value());
    }
    return {};
    const z3::expr result = ctx.z3_ctx.int_val(ctx.mem.allocate(1));
    const z3::expr base = ctx.mem.read_meta(META_BASE, lhs.value(), ctx.z3_ctx.int_sort());
    const z3::expr size = ctx.mem.read_meta(META_SIZE, lhs.value(), ctx.z3_ctx.int_sort());
    const z3::expr address = ctx.mem.read(lhs.value(), ctx.z3_ctx.int_sort()) + ctx.mem.read(rhs.value(), ctx.z3_ctx.int_sort());
    //const z3::expr array = mem.read(lhs.optional_value(), z3_ctx.array_sort(z3_ctx.int_sort(), z3_ctx.int_sort()));
    //ctx.mem.cond_write(address >= base && address < base + size, result, address);
    z3::expr error = address < base || address >= base + size;
    if (ctx.solver.check(1, &error) == z3::check_result::sat) {
        std::cout << "POSSIBLE OUT OF BOUNDS!!!" << std::endl;
    }
    return ctx.mem.read(result, ctx.z3_ctx.int_sort());
}

clsma::optional_value ast_visitor::transform_binary_operator(clsma::block* block, const clang::BinaryOperator* binary_operator) {
    switch (binary_operator->getOpcode()) {
        case clang::BO_Assign: {
            //std::optional<z3::expr> lhs = transform_expr(block, binary_operator->getLHS());
            auto rhs = transform_expr(block, binary_operator->getRHS());
            //if (lhs.has_value() && rhs.has_value()) {
                //checker.check_memory_access(checker.READ, rhs.optional_value());
                //checker.check_memory_access(checker.WRITE, lhs.optional_value());
                //write(block, binary_operator->getLHS(), lhs.optional_value(), read(block, binary_operator->getRHS(), rhs.optional_value()));
                //ctx.mem.write_meta(META_BASE, lhs.optional_value(), ctx.mem.read_meta(META_BASE, rhs.optional_value(), ctx.z3_ctx.int_sort()));
                //ctx.mem.write_meta(META_SIZE, lhs.optional_value(), ctx.mem.read_meta(META_SIZE, rhs.optional_value(), ctx.z3_ctx.int_sort()));
                //solver.add(lhs.optional_value() == rhs.optional_value());
            //}
            assign(block, binary_operator->getLHS(), rhs);
            //const auto lhs_address = get_address(block, binary_operator->getLHS());
            //const auto rhs_address = get_address(block, binary_operator->getRHS());
            //if (lhs_address.has_value() && rhs_address.has_value()) {
            //    ctx.solver.add(lhs_address.optional_value() == rhs_address.optional_value());
            //}
            return rhs;
            //return lhs; // memory-model
        } case clang::BO_Add: {
            auto lhs = transform_expr(block, binary_operator->getLHS());
            auto rhs = transform_expr(block, binary_operator->getRHS());
            if (!lhs.has_value() || !rhs.has_value()) {
                return {};
            }
            return lhs.value() + rhs.value();
            //return push(read(block, binary_operator->getLHS(), lhs.optional_value()) + read(block, binary_operator->getRHS(), rhs.optional_value()));
        } case clang::BO_Sub: {
            auto lhs = transform_expr(block, binary_operator->getLHS());
            auto rhs = transform_expr(block, binary_operator->getRHS());
            if (!lhs.has_value() || !rhs.has_value()) {
                return {};
            }
            return lhs.value() - rhs.value();
            //const z3::expr result = ctx.z3_ctx.int_val(ctx.mem.allocate(1));
            //ctx.mem.write(result, ctx.mem.read(lhs.optional_value(), ctx.z3_ctx.int_sort()) - ctx.mem.read(rhs.optional_value(), ctx.z3_ctx.int_sort()));
            //return result;
        } case clang::BO_LT: {
            auto lhs = transform_expr(block, binary_operator->getLHS());
            auto rhs = transform_expr(block, binary_operator->getRHS());
            if (!lhs.has_value() || !rhs.has_value()) {
                return {};
            }
            return lhs.value() < rhs.value();
        } case clang::BO_EQ: {
            auto lhs = transform_expr(block, binary_operator->getLHS());
            auto rhs = transform_expr(block, binary_operator->getRHS());
            if (!lhs.has_value() || !rhs.has_value()) {
                return {};
            }
            return lhs.value() == rhs.value();
         }default: {
            std::cout << "WARN: unknown BO opcode: " << binary_operator->getOpcodeStr().str() << std::endl;
            return {};
        }
    }
}


clsma::optional_value ast_visitor::transform_unary_operator(clsma::block* block, const clang::UnaryOperator* unary_operator) {
    switch (unary_operator->getOpcode()) {
        case clang::UO_AddrOf: {
            auto sub_expr = transform_expr(block, unary_operator->getSubExpr());
            if (!sub_expr.has_value()) {
                return {};
            }
            const z3::expr result = push(sub_expr.value());
            ctx.mem.write_meta(META_BASE, result, sub_expr.value());
            ctx.mem.write_meta(META_SIZE, result, ctx.z3_ctx.int_val(1));
            return result;
        } case clang::UO_Deref: {
            std::cout << "deref type: " << unary_operator->getType().getAsString() << std::endl;
            std::cout << "restrict deref: " << unary_operator->getType().isRestrictQualified() << std::endl;
            std::cout << "subexpr type: " << unary_operator->getSubExpr()->getType().getAsString() << std::endl;
            std::cout << "restrict subexpr: " << unary_operator->getSubExpr()->getType().isRestrictQualified() << std::endl;
            auto sub_expr = transform_expr(block, unary_operator->getSubExpr());
            return sub_expr.has_value() ? std::make_optional(ctx.mem.read(sub_expr.value(), ctx.z3_ctx.int_sort())) : std::nullopt;
            //return sub_expr.has_value() ? std::make_optional(mem_read(z3_ctx, mem_read(z3_ctx, sub_expr.optional_value()))) : std::nullopt;
            //return sub_expr.has_value() ? std::make_optional(block.deref(sub_expr.optional_value())) : std::nullopt;
        } case clang::UO_PreInc: {
            auto sub_expr = transform_expr(block, unary_operator->getSubExpr());
            return sub_expr.has_value() ? std::make_optional(sub_expr.value() + ctx.z3_ctx.int_val(1)) : std::nullopt;
            //return sub_expr.has_value() ? std::make_optional(mem_read(z3_ctx, mem_read(z3_ctx, sub_expr.optional_value()))) : std::nullopt;
            //return sub_expr.has_value() ? std::make_optional(block.deref(sub_expr.optional_value())) : std::nullopt;
        } default: {
            std::cout << "WARN: unknown UO opcode: " << unary_operator->getOpcode() << std::endl;
            return {};
        }
    }
}

std::unordered_map<std::string, std::function<clsma::optional_value(analyzer_context&, const std::vector<clsma::optional_value>&)>> builtin_constraints = {
        {"get_global_id", [](analyzer_context& ctx, const std::vector<clsma::optional_value>& arguments) -> clsma::optional_value {
            if (arguments.size() != 1 || !arguments[0].has_value()) {
                return {};
            }
            return ctx.z3_ctx.int_val(0);

            const z3::expr idx = ctx.mem.read(arguments[0].value(), ctx.z3_ctx.int_sort());
            //const z3::expr address = ctx.z3_ctx.int_val(ctx.mem.allocate(1));
            const z3::expr result = ctx.mem.read_meta(META_GLOBAL_ID, idx, ctx.z3_ctx.int_sort());
            //ctx.mem.write(address, result);
            return result;
            //z3::expr& arg = arguments[0].optional_value();
            //return std::nullopt;
            //return z3::ite(z3::select(cur_mem(z3_ctx), arg) == 0, z3_ctx.int_val(10), z3_ctx.int_val(12));
        }},
        {"get_local_id", [](analyzer_context& ctx, const std::vector<clsma::optional_value>& arguments) -> clsma::optional_value {
            return {};
        }}
};

clsma::optional_value ast_visitor::transform_call_expr(clsma::block* block, const clang::CallExpr* call_expr) {
    const clang::Decl* callee_decl = call_expr->getCalleeDecl();
    std::vector<clsma::optional_value> args;
    std::transform(call_expr->getArgs(), call_expr->getArgs() + call_expr->getNumArgs(), std::inserter(args, args.begin()), [&](const clang::Expr* expr) {
        return transform_expr(block, expr);
    });
    //const variable* container = block2.decl_var("return@" + std::to_string(callee_decl->getID()), call_expr->getType());
    const z3::expr return_address = ctx.z3_ctx.int_val(ctx.mem.allocate(1));
    if (clang::isa<clang::NamedDecl>(callee_decl)) {
        const auto name = clang::cast<clang::NamedDecl>(callee_decl)->getName().str();
        if (auto it = builtin_constraints.find(name); it != builtin_constraints.end()) {
            const auto predicate = it->second(ctx, args);
            if (predicate.has_value()) {
                ctx.mem.write(return_address, predicate.value());
                return return_address;
                //solver.add(container->to_z3_expr(z3_ctx) == predicate.optional_value());
                //return container->to_z3_expr(z3_ctx);
            }
        }
    }
    if (clang::isa<clang::FunctionDecl>(callee_decl)) {
        clsma::block* call_block = global_block->make_inner();
        const auto* function_decl = clang::cast<clang::FunctionDecl>(callee_decl);
        for (unsigned int i = 0; i < function_decl->getNumParams(); ++i) {
            const auto* var = call_block->decl_var(function_decl->getParamDecl(i));
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

clsma::optional_value ast_visitor::transform_decl_ref_expr(clsma::block* block, const clang::DeclRefExpr* decl_ref_expr) {
    const clsma::variable* var = block->get_var(decl_ref_expr->getDecl());
    return var ? var->to_value() : clsma::optional_value();
    //return variable ? std::make_optional(ctx.z3_ctx.int_val(variable->address)) : std::nullopt;
    //return variable ? std::make_optional(variable->to_z3_expr(z3_ctx)) : std::nullopt;
};

clsma::optional_value ast_visitor::transform_implicit_cast_expr(clsma::block* block, const clang::ImplicitCastExpr *implicit_cast_expr) {
    return transform_expr(block, implicit_cast_expr->getSubExpr());
}

clsma::optional_value ast_visitor::transform_paren_expr(clsma::block* block, const clang::ParenExpr *paren_expr) {
    return transform_expr(block, paren_expr->getSubExpr());
}

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
}

z3::expr ast_visitor::read(const clsma::block* block, const clang::Expr* expr, const z3::expr& address) {
    for (auto& checker : checkers) {
        checker->check_memory_access(block, expr, abstract_checker::READ, address);
    }
    return ctx.mem.read(address, type_to_sort(ctx.z3_ctx, expr->getType()));
}

void ast_visitor::write(const clsma::block* block, const clang::Expr* expr, const z3::expr& address, const z3::expr& value) {
    for (auto& checker : checkers) {
        checker->check_memory_access(block, expr, abstract_checker::WRITE, address);
    }
    ctx.mem.write(address, value);
}

z3::expr ast_visitor::push(const z3::expr& value) {
    z3::expr address = ctx.z3_ctx.int_val(ctx.mem.allocate(1));
    ctx.mem.write(address, value);
    return address;
}

std::optional<z3::expr> ast_visitor::get_address(clsma::block *block, const clang::Expr* expr) {
    expr = expr->IgnoreParenCasts();
    if (clang::isa<clang::DeclRefExpr>(expr)) {
        const auto* decl_ref_expr = clang::cast<clang::DeclRefExpr>(expr);
        auto* var = block->get_var(decl_ref_expr->getDecl());
        if (var != nullptr) {
            return var->to_z3_storage_expr();
        }
    }
    return std::nullopt;
}

void ast_visitor::assign(clsma::block* block, const clang::Expr* expr, const clsma::optional_value& value) {
    return assign(block, expr, value, std::nullopt);
}

void ast_visitor::assign(clsma::block* block, const clang::Expr* lhs, const clsma::optional_value& value, const std::optional<z3::expr>& storage) {
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
        check_memory_access(block, lhs, abstract_checker::WRITE, base.value() + idx.value());
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
        //    block->set_var(decl_ref_expr->getDecl(), optional_value, *address);
        // } else {
        block->set_var(decl_ref_expr->getDecl(), value);

        /*if (storage.has_value()) {
                block->set_var(decl_ref_expr->getDecl(), optional_value, *storage);
            } else {
                block->set_var(decl_ref_expr->getDecl(), optional_value);
        }*/


        //}
        //variable->next_version();
        //ctx.solver.add(variable->to_z3_expr() == optional_value);
    }
}

void ast_visitor::check_memory_access(const clsma::block *block, const clang::Expr *expr,
                                      abstract_checker::memory_access_type access_type, const z3::expr &address) {
    for (auto& checker: checkers) {
        const auto& violation = checker->check_memory_access(block, expr, access_type, address);
        if (violation.has_value()) {
            std::cerr << "violation at " << violation->location.printToString(ctx.ast_ctx.getSourceManager())
            << ": " << violation->message;
        }
    }
}

bool ast_visitor::process_if_stmt(clsma::block* block, const clang::IfStmt* if_stmt, clsma::value_reference& ret_ref) {
    clsma::optional_value condition = transform_expr(block, if_stmt->getCond());
    z3::expr condition_expr = condition.has_value() ? condition.value() : unknown(ctx.z3_ctx.bool_sort());
    clsma::block* then_block = block->make_inner(condition_expr);
    bool then_return = !process_stmt(then_block, if_stmt->getThen(), ret_ref);
    bool else_return = false;
    if (if_stmt->hasElseStorage()) {
        clsma::block* else_block = block->make_inner(!condition_expr);
        else_return = !process_stmt(else_block, if_stmt->getElse(), ret_ref);
    }
    block->join();
    if (then_return != else_return) {
        get_call_block(block)->assume(then_return ? !condition_expr : condition_expr);
    }
    return !then_return || !else_return;
}

z3::expr ast_visitor::unknown(const z3::sort& sort) {
    return ctx.z3_ctx.constant(("unknown_" + std::to_string(unknowns++)).c_str(), sort);
}

bool ast_visitor::process_while_stmt(clsma::block* block, const clang::WhileStmt* while_stmt, clsma::value_reference& ret_ref) {
    for (int i = 0; i < ctx.parameters.options.loop_unwinding_iterations_limit; ++i) {
        clsma::optional_value condition = transform_expr(block, while_stmt->getCond());
        z3::expr condition_expr = condition.has_value() ? condition.value() : unknown(ctx.z3_ctx.bool_sort());
        if (block->check(condition_expr) != z3::sat) {
            break;
        }
        clsma::block* body_block = block->make_inner(condition_expr);
        bool body_return = !process_stmt(body_block, while_stmt->getBody(), ret_ref);
        block->join();
        if (body_return) {
            get_call_block(block)->assume(!condition_expr);
            break;
        }
    }
    return true;
}

clsma::block* ast_visitor::get_call_block(clsma::block* block) {
    while (block->parent && block->parent != global_block) {
        block = block->parent;
    }
    return block;
}
