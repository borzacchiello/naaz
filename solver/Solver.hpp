#pragma once

#include "ConstraintManager.hpp"

namespace naaz::solver
{

class Solver
{
  public:
    virtual bool check(const ConstraintManager& constraints,
                       expr::ExprPtr            query) = 0;

    virtual std::map<std::string, expr::ConstExprPtr> model() = 0;
};

} // namespace naaz::solver
