#pragma once

#include <string>
#include <map>
#include "Expr.hpp"
#include "BVConst.hpp"

namespace naaz::expr
{

ExprPtr evaluate(ExprPtr e, const std::map<uint32_t, BVConst>& assignments,
                 bool model_completion = false);
bool    is_true_const(BoolExprPtr e);
bool    is_false_const(BoolExprPtr e);

std::string expr_to_string(ExprPtr e);

} // namespace naaz::expr
