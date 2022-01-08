#include "abstract_checker.h"

abstract_checker::abstract_checker(analyzer_context& ctx)
: z3_ctx(ctx.z3_ctx), scope_ctx(ctx.scope_ctx), ctx(ctx), assumptions(z3_ctx) { }

void abstract_checker::write_meta(std::string_view meta, const z3::expr& address, const z3::expr& value) {
    ctx.mem.write_meta(meta, address, value);
}

z3::expr abstract_checker::read_meta(std::string_view meta, const z3::expr& address, const z3::sort& sort) const {
    return ctx.mem.read_meta(meta, address, sort);
}

z3::expr abstract_checker::read_meta(std::string_view meta, const z3::sort& sort) const {
    return ctx.mem.read_meta(meta, sort);
}

void abstract_checker::assume(const z3::expr& assumption) {
    assumptions.push_back(assumption);
}

std::optional<z3::model> abstract_checker::check(const z3::expr& assumption) {
    assumptions.push_back(assumption);
    const bool result = ctx.solver.check(assumptions) == z3::check_result::sat;
    assumptions.pop_back();
    const auto retval = result ? std::make_optional(ctx.solver.get_model()) : std::nullopt;
    if (const auto r2 = ctx.solver.check(assumptions); r2 != z3::check_result::sat) {
        std::cout << "ass broken" << std::endl;
        for (const auto& assertion : ctx.solver.assertions()) {
            std::cout << assertion.to_string() << std::endl;
        }
        for (const auto ass : assumptions) {
            std::cout << ass.to_string() << std::endl;
        }
        std::cout << "ass end" << std::endl;
        std::cout << "res: " << r2 << std::endl;
        std::cout << "reason: " << ctx.solver.reason_unknown() << std::endl;
        std::cout << ctx.solver.proof().to_string() << std::endl;
    }
    return retval;
}
