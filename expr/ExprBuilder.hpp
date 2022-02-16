#pragma once

#include "Expr.hpp"

#include <vector>
#include <map>

namespace naaz::expr
{

class ExprBuilder
{
  private:
    std::map<uint64_t, std::vector<WeakExprPtr>> m_exprs;

    ExprPtr get_or_create(const Expr& e);

  public:
    static ExprBuilder& The()
    {
        static ExprBuilder eb;
        return eb;
    }

    void collect_garbage();

    SymExprPtr   mk_sym(const std::string& name, size_t size);
    ConstExprPtr mk_const(__uint128_t val, size_t size);
    ExprPtr      mk_add(ExprPtr lhs, ExprPtr rhs);
    ExprPtr      mk_sub(ExprPtr lhs, ExprPtr rhs);
};

} // namespace naaz::expr
