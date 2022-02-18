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

TEST_CASE("ConcatExpr 1", "[expr]")
{
    ExprPtr s1 = exprBuilder.mk_sym("sym1", 32);
    ExprPtr s2 = exprBuilder.mk_sym("sym2", 32);

    ExprPtr c = exprBuilder.mk_concat(s1, s2);
    REQUIRE(c->kind() == Expr::Kind::CONCAT);

    ConcatExprPtr c_ = std::static_pointer_cast<const ConcatExpr>(c);
    REQUIRE(c_->lhs() == s1);
    REQUIRE(c_->rhs() == s2);
}

TEST_CASE("ExtractExpr 1", "[expr]")
{
    ExprPtr s = exprBuilder.mk_sym("sym", 32);
    ExprPtr e = exprBuilder.mk_extract(s, 7, 0);

    auto e_ = std::static_pointer_cast<const ExtractExpr>(e);
    REQUIRE(e_->expr() == s);
    REQUIRE(e_->high() == 7);
    REQUIRE(e_->low() == 0);
}

TEST_CASE("ExtractExpr 2", "[expr]")
{
    ExprPtr c = exprBuilder.mk_const(0xaabb, 32);
    ExprPtr e = exprBuilder.mk_extract(c, 7, 0);

    REQUIRE(e->kind() == Expr::Kind::CONST);
    auto e_ = std::static_pointer_cast<const ConstExpr>(e);
    REQUIRE(e_->val() == 0xbb);
}

TEST_CASE("ExtractExpr 3", "[expr]")
{
    ExprPtr c = exprBuilder.mk_const(0xaabb, 32);
    ExprPtr e = exprBuilder.mk_extract(c, 11, 4);

    REQUIRE(e->kind() == Expr::Kind::CONST);
    auto e_ = std::static_pointer_cast<const ConstExpr>(e);
    REQUIRE(e_->val() == 0xab);
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
