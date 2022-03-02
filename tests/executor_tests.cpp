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

static std::shared_ptr<state::State>
get_state_executing(const std::shared_ptr<lifter::PCodeLifter> lifter,
                    const uint8_t* code, size_t code_size)
{
    auto as = std::make_shared<loader::AddressSpace>();
    as->register_segment("code", 0x400000, code, code_size, 0);

    auto state = std::make_shared<state::State>(as, lifter, 0x400000);
    return state;
}

TEST_CASE("Execute Block 1", "[expr]")
{
    auto lifter_x64 =
        std::make_shared<lifter::PCodeLifter>(naaz::arch::x86_64::The());
    const uint8_t code[] = "\x48\xC7\xC0\x0A\x00\x00\x00" //   mov rax,0xa
                           "\x48\xC7\xC3\x0A\x00\x00\x00" //   mov rbx,0xa
                           "\x48\x39\xD8"                 //   cmp rax,rbx
                           "\x74\x00"                     //   je RET
                                                          // RET:
                           "\xC3";                        //   ret

    auto state = get_state_executing(lifter_x64, code, sizeof(code));

    executor::PCodeExecutor executor(lifter_x64);
    auto                    successors = executor.execute_basic_block(state);
    REQUIRE(successors.size() == 1);
    REQUIRE(successors.at(0)->pc() == 0x400013);
}
