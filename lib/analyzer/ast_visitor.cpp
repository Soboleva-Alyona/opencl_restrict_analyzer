#include "ast_visitor.h"

#include <iostream>
#include <unordered_map>

#include <z3++.h>

#include <OpenCL/cl.h>

/*ast_visitor::ast_visitor(clang::ASTContext& ctx, const analyzer_parameters& parameters)
: ast_ctx(ctx), solver(z3_ctx), mem(solver), scope_ctx(ctx, z3_ctx, mem), parameters(parameters),
ctx(analyzer_context{ parameters, ast_ctx, z3_ctx, mem,   } ) { }*/

ast_visitor::ast_visitor(analyzer_context& ctx) : ctx(ctx) {
    checkers.emplace_back(std::make_unique<restrict_checker>(ctx));
    ctx.solver.set("proof", true);
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

    z3::expr mem_write(z3::context& z3_ctx, const z3::expr& address, const z3::expr& value) {
        z3::expr predicate = z3_ctx.constant(("$int_mem!!" + std::to_string(mem_ver + 1)).c_str(), z3_ctx.array_sort(z3_ctx.int_sort(), z3_ctx.int_sort())) == z3::store(z3_ctx.constant(("$mem!!" + std::to_string(mem_ver)).c_str(), z3_ctx.array_sort(z3_ctx.int_sort(), z3_ctx.int_sort())), address, value);
        ++mem_ver;
        return predicate;
    }*/
}

bool ast_visitor::VisitFunctionDecl(clang::FunctionDecl* f) {
    if (!f->hasAttr<clang::OpenCLKernelAttr>() || !f->hasBody() || f->getName() != ctx.parameters.kernel_name) {
        return true;
    }
    scope* global_scope = ctx.scope_ctx.make_scope();
    scope* kernel_scope = global_scope->make_inner();
    for (uint32_t i = 0; i < ctx.parameters.work_dim; ++i) {
        const z3::expr global_id =
                ctx.mem.read_meta(META_GLOBAL_ID, ctx.z3_ctx.int_val(i), ctx.z3_ctx.int_sort());
        ctx.solver.add(global_id >= 0 && global_id < ctx.z3_ctx.int_val(uint64_t(ctx.parameters.global_work_size[i])));
    }
    for (unsigned i = 0; i < f->getNumParams(); ++i) {
        const auto& [size, data] = ctx.parameters.args[i];
        const clang::ParmVarDecl* decl = f->getParamDecl(i);
        const clang::QualType& type = decl->getType();
        const var* var = kernel_scope->decl_var(decl);
        if (type->isPointerType() && size == sizeof(cl_mem)) {
            const auto element_size = ctx.ast_ctx.getTypeSizeInChars(type->getPointeeType()).getQuantity();
            size_t mem_size;
            cl_int err = clGetMemObjectInfo(*reinterpret_cast<cl_mem*>(data), CL_MEM_SIZE, sizeof(mem_size), &mem_size, nullptr);
            if (err == 0) {
                const uint64_t array_size = mem_size / element_size;
                const uint64_t base = ctx.mem.allocate(array_size);
                ctx.mem.write(ctx.z3_ctx.int_val(var->address), ctx.z3_ctx.int_val(base));
                ctx.mem.write_meta(META_BASE, ctx.z3_ctx.int_val(var->address), ctx.z3_ctx.int_val(base));
                ctx.mem.write_meta(META_SIZE, ctx.z3_ctx.int_val(var->address), ctx.z3_ctx.int_val(array_size));
                //solver.add(var->to_z3_expr(z3_ctx) == z3_ctx.int_val(var->address));
                continue;
            }
        }
    }
    transformStmt(kernel_scope, f->getBody());
    for (const auto& assertion : ctx.solver.assertions()) {
        std::cout << assertion.to_string() << std::endl;
    }
    try {
        std::cout << ctx.solver.check() << std::endl;
        std::cout << ctx.solver.proof().to_string() << std::endl;
    } catch (z3::exception& ex) {
        std::cerr << ex.msg() << std::endl;
    }
    return false;
}

void ast_visitor::transformStmt(scope* scope, clang::Stmt* stmt) {
    std::cout << stmt->getStmtClassName() << std::endl;
    if (clang::isa<clang::CompoundStmt>(stmt)) {
        transformCompoundStmt(scope, clang::cast<clang::CompoundStmt>(stmt));
    } else if (clang::isa<clang::DeclStmt>(stmt)) {
        processDeclStmt(scope, clang::cast<clang::DeclStmt>(stmt));
    } else if (clang::isa<clang::ValueStmt>(stmt)) {
        transformValueStmt(scope, clang::cast<clang::ValueStmt>(stmt));
    }
}

void ast_visitor::transformCompoundStmt(scope* scope, clang::CompoundStmt* compound_stmt) {
    std::for_each(compound_stmt->body_begin(), compound_stmt->body_end(), [&](clang::Stmt* stmt) {
        transformStmt(scope, stmt);
    });
}

void ast_visitor::processDeclStmt(scope* scope, clang::DeclStmt* decl_stmt) {
    std::for_each(decl_stmt->decl_begin(), decl_stmt->decl_end(), [&](clang::Decl* decl) {
        processDecl(scope, decl);
    });
}

void ast_visitor::processDecl(scope* scope, clang::Decl* decl) {
    switch (decl->getKind()) {
        case clang::Decl::Var:
            processVarDecl(scope, clang::cast<clang::VarDecl>(decl));
            break;
        default:
            break;
    }
}

void ast_visitor::processVarDecl(scope* scope, clang::VarDecl* var_decl) {
    const var* var = scope->decl_var(var_decl);
    if (var_decl->getAnyInitializer()) {
        const std::optional<z3::expr> initializer = transformExpr(scope, var_decl->getAnyInitializer());
        if (initializer.has_value()) {
            const z3::expr address = ctx.z3_ctx.int_val(var->address);
            ctx.mem.write(address, ctx.mem.read(initializer.value(), ctx.z3_ctx.int_sort()));
            if (var_decl->getType()->isPointerType()) {
                ctx.mem.write_meta(META_BASE, address, ctx.mem.read_meta(META_BASE, initializer.value(), ctx.z3_ctx.int_sort()));
                ctx.mem.write_meta(META_SIZE, address, ctx.mem.read_meta(META_SIZE, initializer.value(), ctx.z3_ctx.int_sort()));
            }
            //solver.add(mem_write(z3_ctx, z3_ctx.int_val(var->address), initializer.value()));
            //solver.add(var->to_z3_expr(z3_ctx) == initializer.value());
        }
    }
}

std::optional<z3::expr> ast_visitor::transformValueStmt(scope* scope, const clang::ValueStmt* value_stmt) {
    if (clang::isa<clang::Expr>(value_stmt)) {
        return transformExpr(scope, clang::cast<clang::Expr>(value_stmt));
    } else {
        std::cout << "WARN: unknown value stmt type: " << value_stmt->getStmtClassName() << std::endl;
        return std::nullopt;
    }
}

std::optional<z3::expr> ast_visitor::transformExpr(scope* scope, const clang::Expr* expr) {
    if (clang::isa<clang::ArraySubscriptExpr>(expr)) {
        return transformArraySubscriptExpr(scope, clang::cast<clang::ArraySubscriptExpr>(expr));
    } else if (clang::isa<clang::BinaryOperator>(expr)) {
        return transformBinaryOperator(scope, clang::cast<clang::BinaryOperator>(expr));
    } else if (clang::isa<clang::UnaryOperator>(expr)) {
        return transformUnaryOperator(scope, clang::cast<clang::UnaryOperator>(expr));
    } else if (clang::isa<clang::CallExpr>(expr)) {
        return transformCallExpr(scope, clang::cast<clang::CallExpr>(expr));
    } else if (clang::isa<clang::DeclRefExpr>(expr)) {
        return transformDeclRefExpr(scope, clang::cast<clang::DeclRefExpr>(expr));
    } else if (clang::isa<clang::ImplicitCastExpr>(expr)) {
        return transformImplicitCastExpr(scope, clang::cast<clang::ImplicitCastExpr>(expr));
    } else if (clang::isa<clang::ParenExpr>(expr)) {
        return transformParenExpr(scope, clang::cast<clang::ParenExpr>(expr));
    } else if (clang::isa<clang::IntegerLiteral>(expr)) {
        const z3::expr address = ctx.z3_ctx.int_val(ctx.mem.allocate(1));
        ctx.mem.write(address, ctx.z3_ctx.int_val(clang::cast<clang::IntegerLiteral>(expr)->getValue().getLimitedValue()));
        return address;
    } else {
        std::cout << "WARN: unknown expr type: " << expr->getStmtClassName() << std::endl;
        return std::nullopt;
    }
}

std::optional<z3::expr> ast_visitor::transformArraySubscriptExpr(scope* scope, const clang::ArraySubscriptExpr *array_subscript_expr) {
    std::optional<z3::expr> lhs = transformExpr(scope, array_subscript_expr->getBase());
    std::optional<z3::expr> rhs = transformExpr(scope, array_subscript_expr->getIdx());
    if (!lhs.has_value() || !rhs.has_value() || !lhs->get_sort().is_int()) {
        return std::nullopt;
    }
    std::cout << "base type: " << array_subscript_expr->getBase()->getType().getAsString() << std::endl;
    std::cout << "restrict: " << array_subscript_expr->getBase()->getType().isRestrictQualified() << std::endl;
    const z3::expr result = ctx.z3_ctx.int_val(ctx.mem.allocate(1));
    const z3::expr base = ctx.mem.read_meta(META_BASE, lhs.value(), ctx.z3_ctx.int_sort());
    const z3::expr size = ctx.mem.read_meta(META_SIZE, lhs.value(), ctx.z3_ctx.int_sort());
    const z3::expr address = ctx.mem.read(lhs.value(), ctx.z3_ctx.int_sort()) + ctx.mem.read(rhs.value(), ctx.z3_ctx.int_sort());
    //const z3::expr array = mem.read(lhs.value(), z3_ctx.array_sort(z3_ctx.int_sort(), z3_ctx.int_sort()));
    ctx.mem.cond_write(address >= base && address < base + size, result, address);
    z3::expr error = address < base || address >= base + size;
    if (ctx.solver.check(1, &error) == z3::check_result::sat) {
        std::cout << "POSSIBLE OUT OF BOUNDS!!!" << std::endl;
    }
    return ctx.mem.read(result, ctx.z3_ctx.int_sort());
}

std::optional<z3::expr> ast_visitor::transformBinaryOperator(scope* scope, const clang::BinaryOperator* binary_operator) {
    switch (binary_operator->getOpcode()) {
        case clang::BO_Assign: {
            std::optional<z3::expr> lhs = transformExpr(scope, binary_operator->getLHS());
            std::optional<z3::expr> rhs = transformExpr(scope, binary_operator->getRHS());
            if (lhs.has_value() && rhs.has_value()) {
                //checker.check_memory_access(checker.READ, rhs.value());
                //checker.check_memory_access(checker.WRITE, lhs.value());
                write(scope, binary_operator->getLHS(), lhs.value(), read(scope, binary_operator->getRHS(), rhs.value()));
                ctx.mem.write_meta(META_BASE, lhs.value(), ctx.mem.read_meta(META_BASE, rhs.value(), ctx.z3_ctx.int_sort()));
                ctx.mem.write_meta(META_SIZE, lhs.value(), ctx.mem.read_meta(META_SIZE, rhs.value(), ctx.z3_ctx.int_sort()));
                //solver.add(lhs.value() == rhs.value());
            }
            return lhs;
        } case clang::BO_Add: {
            std::optional<z3::expr> lhs = transformExpr(scope, binary_operator->getLHS());
            std::optional<z3::expr> rhs = transformExpr(scope, binary_operator->getRHS());
            if (!lhs.has_value() || !rhs.has_value()) {
                return std::nullopt;
            }
            return push(read(scope, binary_operator->getLHS(), lhs.value()) + read(scope, binary_operator->getRHS(), rhs.value()));
        }  case clang::BO_Sub: {
            std::optional<z3::expr> lhs = transformExpr(scope, binary_operator->getLHS());
            std::optional<z3::expr> rhs = transformExpr(scope, binary_operator->getRHS());
            if (!lhs.has_value() || !rhs.has_value()) {
                return std::nullopt;
            }
            const z3::expr result = ctx.z3_ctx.int_val(ctx.mem.allocate(1));
            ctx.mem.write(result, ctx.mem.read(lhs.value(), ctx.z3_ctx.int_sort()) - ctx.mem.read(rhs.value(), ctx.z3_ctx.int_sort()));
            return result;
        } default: {
            std::cout << "WARN: unknown BO opcode: " << binary_operator->getOpcodeStr().str() << std::endl;
            return std::nullopt;
        }
    }
}


std::optional<z3::expr> ast_visitor::transformUnaryOperator(scope* scope, const clang::UnaryOperator* unary_operator) {
    switch (unary_operator->getOpcode()) {
        case clang::UO_AddrOf: {
            std::optional<z3::expr> sub_expr = transformExpr(scope, unary_operator->getSubExpr());
            if (!sub_expr.has_value()) {
                return std::nullopt;
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
            std::optional<z3::expr> sub_expr = transformExpr(scope, unary_operator->getSubExpr());
            return sub_expr.has_value() ? std::make_optional(ctx.mem.read(sub_expr.value(), ctx.z3_ctx.int_sort())) : std::nullopt;
            //return sub_expr.has_value() ? std::make_optional(mem_read(z3_ctx, mem_read(z3_ctx, sub_expr.value()))) : std::nullopt;
            //return sub_expr.has_value() ? std::make_optional(scope.deref(sub_expr.value())) : std::nullopt;
        } default: {
            std::cout << "WARN: unknown UO opcode: " << unary_operator->getOpcode() << std::endl;
            return std::nullopt;
        }
    }
}

std::unordered_map<std::string, std::function<std::optional<z3::expr>(analyzer_context&, std::vector<std::optional<z3::expr>>)>> builtin_constraints = {
        {"get_global_id", [](analyzer_context& ctx, std::vector<std::optional<z3::expr>> arguments) -> std::optional<z3::expr> {
            if (arguments.size() != 1 || !arguments[0].has_value()) {
                return std::nullopt;
            }
            const z3::expr idx = ctx.mem.read(arguments[0].value(), ctx.z3_ctx.int_sort());
            //const z3::expr address = ctx.z3_ctx.int_val(ctx.mem.allocate(1));
            const z3::expr result = ctx.mem.read_meta(META_GLOBAL_ID, idx, ctx.z3_ctx.int_sort());
            //ctx.mem.write(address, result);
            return result;
            //z3::expr& arg = arguments[0].value();
            //return std::nullopt;
            //return z3::ite(z3::select(cur_mem(z3_ctx), arg) == 0, z3_ctx.int_val(10), z3_ctx.int_val(12));
        }},
        {"get_local_id", [](analyzer_context& ctx, std::vector<std::optional<z3::expr>> arguments){
            return std::nullopt;
        }}
};

std::optional<z3::expr> ast_visitor::transformCallExpr(scope* scope2, const clang::CallExpr* call_expr) {
    const clang::Decl* callee_decl = call_expr->getCalleeDecl();
    std::vector<std::optional<z3::expr>> args;
    std::transform(call_expr->getArgs(), call_expr->getArgs() + call_expr->getNumArgs(), std::inserter(args, args.begin()), [&](const clang::Expr* expr) {
        return transformExpr(scope2, expr);
    });
    //const var* container = scope2.decl_var("return@" + std::to_string(callee_decl->getID()), call_expr->getType());
    const z3::expr return_address = ctx.z3_ctx.int_val(ctx.mem.allocate(1));
    if (clang::isa<clang::NamedDecl>(callee_decl)) {
        const auto name = clang::cast<clang::NamedDecl>(callee_decl)->getName().str();
        if (auto it = builtin_constraints.find(name); it != builtin_constraints.end()) {
            const auto predicate = it->second(ctx, args);
            if (predicate.has_value()) {
                ctx.mem.write(return_address, predicate.value());
                return return_address;
                //solver.add(container->to_z3_expr(z3_ctx) == predicate.value());
                //return container->to_z3_expr(z3_ctx);
            }
        }
    }
    if (callee_decl->hasBody()) {
        transformStmt(scope2, callee_decl->getBody());
    }
    return std::optional<z3::expr>();
}

std::optional<z3::expr> ast_visitor::transformDeclRefExpr(scope* scope, const clang::DeclRefExpr* decl_ref_expr) {
    const var* var = scope->get_var(decl_ref_expr->getDecl());
    return var ? std::make_optional(ctx.z3_ctx.int_val(var->address)) : std::nullopt;
    //return var ? std::make_optional(var->to_z3_expr(z3_ctx)) : std::nullopt;
};

std::optional<z3::expr> ast_visitor::transformImplicitCastExpr(scope* scope, const clang::ImplicitCastExpr *implicit_cast_expr) {
    return transformExpr(scope, implicit_cast_expr->getSubExpr());
}

std::optional<z3::expr> ast_visitor::transformParenExpr(scope* scope, const clang::ParenExpr *paren_expr) {
    return transformExpr(scope, paren_expr->getSubExpr());
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

z3::expr ast_visitor::read(const scope* scope, const clang::Expr* expr, const z3::expr& address) {
    for (auto& checker : checkers) {
        checker->check_memory_access(scope, expr, abstract_checker::READ, address);
    }
    return ctx.mem.read(address, type_to_sort(ctx.z3_ctx, expr->getType()));
}

void ast_visitor::write(const scope* scope, const clang::Expr* expr, const z3::expr& address, const z3::expr& value) {
    for (auto& checker : checkers) {
        checker->check_memory_access(scope, expr, abstract_checker::WRITE, address);
    }
    ctx.mem.write(address, value);
}

z3::expr ast_visitor::push(const z3::expr& value) {
    z3::expr address = ctx.z3_ctx.int_val(ctx.mem.allocate(1));
    ctx.mem.write(address, value);
    return address;
}
