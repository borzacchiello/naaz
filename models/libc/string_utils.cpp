#include "string_utils.hpp"

#include "../../expr/ExprBuilder.hpp"
#include "../../state/State.hpp"

namespace naaz::models::libc
{

std::vector<resolved_string_t> resolve_string(state::StatePtr state,
                                              uint64_t str_addr, int max_forks,
                                              int max_size)
{
    // This helper `resolve` a string by forking the state

    std::vector<resolved_string_t> res;

    expr::BVExprPtr curr_str  = nullptr;
    expr::BVExprPtr zero_byte = expr::ExprBuilder::The().mk_const(0UL, 8);
    auto            curr      = str_addr;
    auto            start     = str_addr;
    while (max_size != 0) {
        max_size = max_size > 0 ? max_size - 1 : -1;

        auto b   = state->read(curr, 1);
        curr_str = curr_str == nullptr
                       ? b
                       : expr::ExprBuilder::The().mk_concat(curr_str, b);

        if (b->kind() == expr::Expr::Kind::CONST) {
            auto b_ = std::static_pointer_cast<const expr::ConstExpr>(b);
            if (b_->val().is_zero())
                break;
        } else {
            auto is_zero_expr = expr::ExprBuilder::The().mk_eq(b, zero_byte);
            if (state->solver().may_be_true(is_zero_expr) ==
                solver::CheckResult::SAT) {
                state::StatePtr succ = max_forks <= 0 ? state : state->clone();
                succ->solver().add(is_zero_expr);
                succ->write(curr, zero_byte);

                res.push_back(resolved_string_t(succ, curr_str));
                if (max_forks <= 0)
                    return res;

                if (state->solver().check_sat_and_add_if_sat(
                        expr::ExprBuilder::The().mk_not(is_zero_expr)) !=
                    solver::CheckResult::SAT)
                    // is symbolic but it is always zero
                    return res;
                max_forks--;
            } else {
                // is symbolic but cannot be zero
                state->write(curr, zero_byte);
            }
        }
        curr += 1;
    }

    res.push_back(resolved_string_t(state, curr_str));
    return res;
}

} // namespace naaz::models::libc
