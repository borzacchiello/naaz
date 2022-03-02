#pragma once

#include <string>
#include <map>
#include "Expr.hpp"
#include "BVConst.hpp"

namespace naaz::expr
{

ExprPtr evaluate(ExprPtr e, const std::map<uint32_t, BVConst>& assignments,
                 bool model_completion = false);

}
