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

    ConstExprPtr mk_true();
    ConstExprPtr mk_false();

    SymExprPtr   mk_sym(const std::string& name, size_t size);
    ConstExprPtr mk_const(__uint128_t val, size_t size);
    ExprPtr      mk_extract(ExprPtr expr, uint32_t high, uint32_t low);
    ExprPtr      mk_concat(ExprPtr left, ExprPtr right);
    ExprPtr      mk_zext(ExprPtr e, uint32_t n);

    // arithmetic
    ExprPtr mk_neg(ExprPtr expr);
    ExprPtr mk_add(ExprPtr lhs, ExprPtr rhs);
    ExprPtr mk_sub(ExprPtr lhs, ExprPtr rhs);

    // logical
    ExprPtr mk_not(ExprPtr expr);
    ExprPtr mk_ult(ExprPtr lhs, ExprPtr rhs);
    ExprPtr mk_ule(ExprPtr lhs, ExprPtr rhs);
    ExprPtr mk_ugt(ExprPtr lhs, ExprPtr rhs);
    ExprPtr mk_uge(ExprPtr lhs, ExprPtr rhs);
    ExprPtr mk_slt(ExprPtr lhs, ExprPtr rhs);
    ExprPtr mk_sle(ExprPtr lhs, ExprPtr rhs);
    ExprPtr mk_sgt(ExprPtr lhs, ExprPtr rhs);
    ExprPtr mk_sge(ExprPtr lhs, ExprPtr rhs);
    ExprPtr mk_eq(ExprPtr lhs, ExprPtr rhs);
};

} // namespace naaz::expr
