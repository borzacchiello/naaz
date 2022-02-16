#include <catch2/catch_all.hpp>

#include "../expr/Expr.hpp"
#include "../expr/ExprBuilder.hpp"

using namespace naaz::expr;

#define exprBuilder ExprBuilder::The()

TEST_CASE("SymExpr 1", "[expr]")
{
    SymExprPtr e = exprBuilder.mk_sym("sym", 32);
    REQUIRE(e->size() == 32);
    REQUIRE(e->name().compare("sym") == 0);
}

TEST_CASE("AddExpr 1", "[expr]")
{
    ExprPtr    s = exprBuilder.mk_sym("sym", 32);
    ExprPtr    c = exprBuilder.mk_const(42, 32);
    AddExprPtr e =
        std::static_pointer_cast<const AddExpr>(exprBuilder.mk_add(s, c));

    REQUIRE(e->lhs().get() == s.get());
    REQUIRE(e->rhs().get() == c.get());
}

TEST_CASE("AddExpr 2", "[expr]")
{
    ExprPtr e = exprBuilder.mk_const(42, 32);
    e         = exprBuilder.mk_add(e, exprBuilder.mk_const(38, 32));

    REQUIRE(std::static_pointer_cast<const ConstExpr>(e)->val() == 80);
}

TEST_CASE("Equality 1", "[expr]")
{
    ExprPtr e1 = exprBuilder.mk_sym("sym", 32);
    e1         = exprBuilder.mk_add(e1, exprBuilder.mk_sym("sym1", 32));
    e1         = exprBuilder.mk_sub(e1, exprBuilder.mk_const(42, 32));

    ExprPtr e2 = exprBuilder.mk_sym("sym", 32);
    e2         = exprBuilder.mk_add(e2, exprBuilder.mk_sym("sym1", 32));
    e2         = exprBuilder.mk_sub(e2, exprBuilder.mk_const(42, 32));

    REQUIRE(e1 == e2);
}
