#include <catch2/catch_all.hpp>
#include <memory>

#include "../arch/Arch.hpp"
#include "../expr/Expr.hpp"
#include "../expr/ExprBuilder.hpp"
#include "../state/State.hpp"
#include "../loader/AddressSpace.hpp"
#include "../lifter/PCodeLifter.hpp"
#include "../executor/PCodeExecutor.hpp"

using namespace naaz;

TEST_CASE("Execute Block 1", "[expr]")
{
    auto lifter = std::make_shared<lifter::PCodeLifter>(arch::x86_64::The());

    const uint8_t code[] =
        "\x48\xC7\xC0\x0A\x00\x00\x00\x48\xC7\xC3\x0A\x00\x00"
        "\x00\x48\x39\xD8\x74\x00\xC3";
    size_t code_size = sizeof(code);

    auto as = std::make_shared<loader::AddressSpace>();
    as->register_segment("code", 0x400000, code, code_size, 0);

    auto state = std::make_shared<state::State>(as, lifter, 0x400000);

    executor::PCodeExecutor executor(lifter);
    executor.execute_basic_block(state);
}
