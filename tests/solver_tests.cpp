#include <catch2/catch_all.hpp>

#include "../expr/Expr.hpp"
#include "../expr/ExprBuilder.hpp"
#include "../solver/ConstraintManager.hpp"

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

    std::cerr << query1->to_string() << std::endl;
    std::cerr << query2->to_string() << std::endl;
}
