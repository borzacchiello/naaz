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

    ExprBuilder() {}

  public:
    static ExprBuilder& The()
    {
        static ExprBuilder eb;
        return eb;
    }

    void collect_garbage();

    BoolConstPtr mk_true();
    BoolConstPtr mk_false();

    SymExprPtr   mk_sym(const std::string& name, size_t size);
    ConstExprPtr mk_const(const BVConst& val);
    ConstExprPtr mk_const(uint64_t val, size_t size);
    BVExprPtr    mk_extract(BVExprPtr expr, uint32_t high, uint32_t low);
    BVExprPtr    mk_concat(BVExprPtr left, BVExprPtr right);
    BVExprPtr    mk_zext(BVExprPtr e, uint32_t n);
    BVExprPtr    mk_sext(BVExprPtr e, uint32_t n);
    BVExprPtr    mk_ite(BoolExprPtr guard, BVExprPtr iftrue, BVExprPtr iffalse);

    // arithmetic
    BVExprPtr mk_neg(BVExprPtr expr);
    BVExprPtr mk_add(BVExprPtr lhs, BVExprPtr rhs);
    BVExprPtr mk_sub(BVExprPtr lhs, BVExprPtr rhs);

    // logical
    BoolExprPtr mk_not(BoolExprPtr expr);
    BoolExprPtr mk_ult(BVExprPtr lhs, BVExprPtr rhs);
    BoolExprPtr mk_ule(BVExprPtr lhs, BVExprPtr rhs);
    BoolExprPtr mk_ugt(BVExprPtr lhs, BVExprPtr rhs);
    BoolExprPtr mk_uge(BVExprPtr lhs, BVExprPtr rhs);
    BoolExprPtr mk_slt(BVExprPtr lhs, BVExprPtr rhs);
    BoolExprPtr mk_sle(BVExprPtr lhs, BVExprPtr rhs);
    BoolExprPtr mk_sgt(BVExprPtr lhs, BVExprPtr rhs);
    BoolExprPtr mk_sge(BVExprPtr lhs, BVExprPtr rhs);
    BoolExprPtr mk_eq(BVExprPtr lhs, BVExprPtr rhs);

    BoolExprPtr mk_bool_and(BoolExprPtr e1, BoolExprPtr e2);

    // util
    BVExprPtr   sign_bit(BVExprPtr expr);
    BVExprPtr   bool_to_bv(BoolExprPtr expr);
    BoolExprPtr bv_to_bool(BVExprPtr expr);
};

} // namespace naaz::expr
