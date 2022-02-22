#include <catch2/catch_all.hpp>
#include <memory>

#include "../arch/Arch.hpp"
#include "../expr/Expr.hpp"
#include "../expr/ExprBuilder.hpp"
#include "../state/State.hpp"
#include "../loader/AddressSpace.hpp"
#include "../lifter/PCodeLifter.hpp"

using namespace naaz::state;
using namespace naaz::expr;
using namespace naaz::lifter;
using namespace naaz::loader;

#define exprBuilder ExprBuilder::The()

static std::shared_ptr<PCodeLifter> get_x86_64_lifter()
{
    static bool                         init = false;
    static std::shared_ptr<PCodeLifter> lifter;
    if (!init) {
        init   = true;
        lifter = std::make_shared<PCodeLifter>(naaz::arch::x86_64::The());
    }
    return lifter;
}

TEST_CASE("State Read/Write Mem 1", "[state]")
{
    auto lifter = get_x86_64_lifter();
    auto as     = std::make_shared<AddressSpace>();

    State s(as, lifter, 0);

    BVExprPtr sym = exprBuilder.mk_sym("sym", 32);
    s.write(0xaabbcc, sym);
    ExprPtr expr = s.read(0xaabbcc, 4);

    REQUIRE(expr == sym);
}

TEST_CASE("State Read/Write Mem 2", "[state]")
{
    auto lifter = get_x86_64_lifter();
    auto as     = std::make_shared<AddressSpace>();

    State s(as, lifter, 0);

    BVExprPtr sym = exprBuilder.mk_sym("sym", 32);
    s.write(0xaabbcc, sym);
    ExprPtr expr = s.read(0xaabbcc, 1);

    REQUIRE(expr == exprBuilder.mk_extract(sym, 31, 24));
}

TEST_CASE("State Read/Write Reg 1", "[state]")
{
    auto lifter = get_x86_64_lifter();
    auto as     = std::make_shared<AddressSpace>();

    State s(as, lifter, 0);

    BVExprPtr sym = exprBuilder.mk_sym("sym", 32);
    s.reg_write("EAX", sym);
    ExprPtr expr = s.reg_read("AL");

    REQUIRE(expr == exprBuilder.mk_extract(sym, 7, 0));
}
