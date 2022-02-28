#include <catch2/catch_all.hpp>

#include "../expr/Expr.hpp"
#include "../expr/ExprBuilder.hpp"
#include "../solver/ConstraintManager.hpp"
#include "../solver/Z3Solver.hpp"

using namespace naaz::solver;
using namespace naaz::expr;

#define exprBuilder naaz::expr::ExprBuilder::The()

TEST_CASE("ConstraintManager constuctor 1", "[solver]")
{
    ConstraintManager manager;

    auto sym1 = exprBuilder.mk_sym("sym1", 32);
    auto sym2 = exprBuilder.mk_sym("sym2", 32);
    auto sym3 = exprBuilder.mk_sym("sym3", 32);
    auto sym4 = exprBuilder.mk_sym("sym4", 32);
    auto sym5 = exprBuilder.mk_sym("sym5", 32);
    auto sym6 = exprBuilder.mk_sym("sym6", 32);

    manager.add(exprBuilder.mk_sgt(sym1, sym2));
    manager.add(exprBuilder.mk_sgt(sym2, sym3));

    manager.add(exprBuilder.mk_sgt(sym4, sym5));

    auto query1 = manager.build_query(exprBuilder.mk_sgt(sym3, sym6));
    auto query2 = manager.build_query(
        exprBuilder.mk_sgt(sym6, exprBuilder.mk_const(10, 32)));

    auto expected1 = exprBuilder.mk_bool_and(
        exprBuilder.mk_sgt(sym1, sym2),
        exprBuilder.mk_bool_and(exprBuilder.mk_sgt(sym2, sym3),
                                exprBuilder.mk_sgt(sym3, sym6)));
    auto expected2 = exprBuilder.mk_sgt(sym6, exprBuilder.mk_const(10, 32));

    REQUIRE(query1 == expected1);
    REQUIRE(query2 == expected2);
}

TEST_CASE("Z3Solver 1", "[solver]")
{
    ConstraintManager manager;

    auto sym1 = exprBuilder.mk_sym("sym1", 32);

    manager.add(exprBuilder.mk_sgt(sym1, exprBuilder.mk_const(0, 32)));

    CheckResult sat_res = Z3Solver::The().check(
        manager, exprBuilder.mk_slt(sym1, exprBuilder.mk_const(3, 32)));

    REQUIRE(sat_res == CheckResult::SAT);

    auto model = Z3Solver::The().model();
    REQUIRE(model.contains("sym1"));

    auto sym1_val = model["sym1"].as_s64();
    REQUIRE(sym1_val > 0);
    REQUIRE(sym1_val < 3);
}
