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
    ExprPtr s = exprBuilder.mk_sym("sym", 32);
    ExprPtr c = exprBuilder.mk_const(42, 32);
    AddExprPtr e = exprBuilder.mk_add(s, c);

    REQUIRE(e->lhs().get() == s.get());
    REQUIRE(e->rhs().get() == c.get());
}
