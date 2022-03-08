#include <catch2/catch_all.hpp>

#include "../util/ioutil.hpp"
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

TEST_CASE("NegExpr 1", "[expr]")
{
    BVExprPtr c = exprBuilder.mk_const(1, 8);
    c           = exprBuilder.mk_neg(c);

    REQUIRE(c->kind() == Expr::Kind::CONST);
    auto c_ = std::static_pointer_cast<const ConstExpr>(c);

    REQUIRE(c_->val().as_s64() == -1);
    REQUIRE(c_->val().as_u64() == 255);
}

TEST_CASE("NegExpr 2", "[expr]")
{
    ExprPtr c =
        exprBuilder.mk_add(exprBuilder.mk_const(42, 8),
                           exprBuilder.mk_neg(exprBuilder.mk_const(1, 8)));

    REQUIRE(c->kind() == Expr::Kind::CONST);
    auto c_ = std::static_pointer_cast<const ConstExpr>(c);

    REQUIRE(c_->val().as_u64() == 41);
}

TEST_CASE("AddExpr 1", "[expr]")
{
    BVExprPtr  s = exprBuilder.mk_sym("sym", 32);
    BVExprPtr  c = exprBuilder.mk_const(42, 32);
    AddExprPtr e =
        std::static_pointer_cast<const AddExpr>(exprBuilder.mk_add(s, c));

    auto c1 = e->addends().at(0);
    auto c2 = e->addends().at(1);

    if (c1 == s) {
        REQUIRE(c2 == c);
    } else {
        REQUIRE(c1 == c);
        REQUIRE(c2 == s);
    }
}

TEST_CASE("AddExpr 2", "[expr]")
{
    BVExprPtr s = exprBuilder.mk_sym("sym", 32);

    BVExprPtr e = exprBuilder.mk_sub(s, s);
    REQUIRE(std::static_pointer_cast<const ConstExpr>(e)->val().is_zero());
}

TEST_CASE("MulExpr 1", "[expr]")
{
    BVExprPtr e = exprBuilder.mk_const(21, 32);
    e           = exprBuilder.mk_mul(exprBuilder.mk_const(2, 32), e);

    REQUIRE(std::static_pointer_cast<const ConstExpr>(e)->val().as_u64() == 42);
}

TEST_CASE("MulExpr 2", "[expr]")
{
    BVExprPtr s = exprBuilder.mk_sym("sym", 32);
    BVExprPtr e = exprBuilder.mk_mul(exprBuilder.mk_const(2, 32), s);

    REQUIRE(e->to_string() == "(sym * 0x2)");
}

TEST_CASE("SDivExpr 1", "[expr]")
{
    BVExprPtr e = exprBuilder.mk_const(21, 32);
    e           = exprBuilder.mk_sdiv(e, exprBuilder.mk_const(2, 32));

    REQUIRE(std::static_pointer_cast<const ConstExpr>(e)->val().as_u64() == 10);
}

TEST_CASE("SDivExpr 2", "[expr]")
{
    BVExprPtr s = exprBuilder.mk_sym("sym", 32);
    BVExprPtr e = exprBuilder.mk_sdiv(s, exprBuilder.mk_const(2, 32));

    REQUIRE(e->to_string() == "(sym s/ 0x2)");
}

TEST_CASE("SDivExpr 3", "[expr]")
{
    BVExprPtr s = exprBuilder.mk_sym("sym", 32);
    BVExprPtr e = exprBuilder.mk_sdiv(s, s);

    REQUIRE(std::static_pointer_cast<const ConstExpr>(e)->val().as_u64() == 1);
}

TEST_CASE("AddExpr 3", "[expr]")
{
    BVExprPtr s1 = exprBuilder.mk_sym("sym1", 32);
    BVExprPtr s2 = exprBuilder.mk_sym("sym2", 32);
    BVExprPtr s3 = exprBuilder.mk_sym("sym3", 32);

    BVExprPtr e = exprBuilder.mk_add(
        s1, exprBuilder.mk_add(s2, exprBuilder.mk_sub(s3, s1)));
    REQUIRE(e == exprBuilder.mk_add(s2, s3));
}

TEST_CASE("AddExpr 4", "[expr]")
{
    BVExprPtr e = exprBuilder.mk_const(42, 32);
    e           = exprBuilder.mk_add(e, exprBuilder.mk_const(38, 32));

    REQUIRE(std::static_pointer_cast<const ConstExpr>(e)->val().as_u64() == 80);
}

TEST_CASE("AndExpr 1", "[expr]")
{
    BVExprPtr c1 = exprBuilder.mk_const(0xf0f0f0f0, 32);
    BVExprPtr c2 = exprBuilder.mk_const(0x0f0f0f0f, 32);
    BVExprPtr e  = exprBuilder.mk_and(c1, c2);

    REQUIRE(std::static_pointer_cast<const ConstExpr>(e)->val().is_zero());
}

TEST_CASE("AndExpr 2", "[expr]")
{
    BVExprPtr s = exprBuilder.mk_sym("sym", 32);
    BVExprPtr c = exprBuilder.mk_const(0UL, 32);
    BVExprPtr e = exprBuilder.mk_and(s, c);

    REQUIRE(std::static_pointer_cast<const ConstExpr>(e)->val().is_zero());
}

TEST_CASE("OrExpr 1", "[expr]")
{
    BVExprPtr c1 = exprBuilder.mk_const(0xf0f0f0f0, 32);
    BVExprPtr c2 = exprBuilder.mk_const(0x0f0f0f0f, 32);
    BVExprPtr e  = exprBuilder.mk_or(c1, c2);

    REQUIRE(
        std::static_pointer_cast<const ConstExpr>(e)->val().has_all_bit_set());
}

TEST_CASE("OrExpr 2", "[expr]")
{
    BVExprPtr s = exprBuilder.mk_sym("sym", 32);
    BVExprPtr c = exprBuilder.mk_const(BVConst("-1", 32));
    BVExprPtr e = exprBuilder.mk_or(s, c);

    REQUIRE(
        std::static_pointer_cast<const ConstExpr>(e)->val().has_all_bit_set());
}

TEST_CASE("XorExpr 1", "[expr]")
{
    BVExprPtr c1 = exprBuilder.mk_const(0xa, 32);
    BVExprPtr c2 = exprBuilder.mk_const(0xb, 32);
    BVExprPtr e  = exprBuilder.mk_xor(c1, c2);

    REQUIRE(std::static_pointer_cast<const ConstExpr>(e)->val().as_u64() == 1);
}

TEST_CASE("XorExpr 2", "[expr]")
{
    BVExprPtr s1 = exprBuilder.mk_sym("sym1", 32);
    BVExprPtr e  = exprBuilder.mk_xor(s1, s1);

    REQUIRE(std::static_pointer_cast<const ConstExpr>(e)->val().is_zero());
}

TEST_CASE("ShlExpr 1", "[expr]")
{
    BVExprPtr e = exprBuilder.mk_const(1, 32);
    e           = exprBuilder.mk_shl(e, exprBuilder.mk_const(3, 32));

    REQUIRE(std::static_pointer_cast<const ConstExpr>(e)->val().as_u64() == 8);
}

TEST_CASE("AShrExpr 1", "[expr]")
{
    BVExprPtr e = exprBuilder.mk_const(0xf0, 8);
    e           = exprBuilder.mk_ashr(e, exprBuilder.mk_const(1, 8));

    REQUIRE(std::static_pointer_cast<const ConstExpr>(e)->val().as_u64() ==
            0xf8);
}

TEST_CASE("LShrExpr 1", "[expr]")
{
    BVExprPtr e = exprBuilder.mk_const(0xf0, 8);
    e           = exprBuilder.mk_lshr(e, exprBuilder.mk_const(1, 8));

    REQUIRE(std::static_pointer_cast<const ConstExpr>(e)->val().as_u64() ==
            0x78);
}

TEST_CASE("ConcatExpr 1", "[expr]")
{
    BVExprPtr s1 = exprBuilder.mk_sym("sym1", 32);
    BVExprPtr s2 = exprBuilder.mk_sym("sym2", 32);

    ExprPtr c = exprBuilder.mk_concat(s1, s2);
    REQUIRE(c->kind() == Expr::Kind::CONCAT);

    ConcatExprPtr c_ = std::static_pointer_cast<const ConcatExpr>(c);
    REQUIRE(c_->lhs() == s1);
    REQUIRE(c_->rhs() == s2);
}

TEST_CASE("ExtractExpr 1", "[expr]")
{
    BVExprPtr s = exprBuilder.mk_sym("sym", 32);
    BVExprPtr e = exprBuilder.mk_extract(s, 7, 0);

    auto e_ = std::static_pointer_cast<const ExtractExpr>(e);
    REQUIRE(e_->expr() == s);
    REQUIRE(e_->high() == 7);
    REQUIRE(e_->low() == 0);
}

TEST_CASE("ExtractExpr 2", "[expr]")
{
    BVExprPtr c = exprBuilder.mk_const(0xaabb, 32);
    BVExprPtr e = exprBuilder.mk_extract(c, 7, 0);

    REQUIRE(e->kind() == Expr::Kind::CONST);
    auto e_ = std::static_pointer_cast<const ConstExpr>(e);
    REQUIRE(e_->val().as_u64() == 0xbb);
}

TEST_CASE("ExtractExpr 3", "[expr]")
{
    BVExprPtr c = exprBuilder.mk_const(0xaabb, 32);
    BVExprPtr e = exprBuilder.mk_extract(c, 11, 4);

    REQUIRE(e->kind() == Expr::Kind::CONST);
    auto e_ = std::static_pointer_cast<const ConstExpr>(e);
    REQUIRE(e_->val().as_u64() == 0xab);
}

TEST_CASE("Equality 1", "[expr]")
{
    BVExprPtr e1 = exprBuilder.mk_sym("sym", 32);
    e1           = exprBuilder.mk_add(e1, exprBuilder.mk_sym("sym1", 32));
    e1           = exprBuilder.mk_sub(e1, exprBuilder.mk_const(42, 32));

    BVExprPtr e2 = exprBuilder.mk_sym("sym", 32);
    e2           = exprBuilder.mk_add(e2, exprBuilder.mk_sym("sym1", 32));
    e2           = exprBuilder.mk_sub(e2, exprBuilder.mk_const(42, 32));

    REQUIRE(e1 == e2);
}

TEST_CASE("Sgt 1", "[expr]")
{
    BoolExprPtr e = exprBuilder.mk_sgt(exprBuilder.mk_const(-10, 8),
                                       exprBuilder.mk_const(0, 8));
    REQUIRE(e == exprBuilder.mk_false());
}
