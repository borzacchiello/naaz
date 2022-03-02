#include <catch2/catch_all.hpp>
#include <memory>

#include "../arch/Arch.hpp"
#include "../expr/Expr.hpp"
#include "../expr/ExprBuilder.hpp"
#include "../state/State.hpp"
#include "../loader/AddressSpace.hpp"
#include "../lifter/PCodeLifter.hpp"
#include "../executor/PCodeExecutor.hpp"

#define exprBuilder naaz::expr::ExprBuilder::The()

using namespace naaz;

static std::shared_ptr<lifter::PCodeLifter> get_x86_64_lifter()
{
    static bool                                 init = false;
    static std::shared_ptr<lifter::PCodeLifter> lifter;
    if (!init) {
        init = true;
        lifter =
            std::make_shared<lifter::PCodeLifter>(naaz::arch::x86_64::The());
    }
    return lifter;
}

static std::shared_ptr<state::State>
get_state_executing(const std::shared_ptr<lifter::PCodeLifter> lifter,
                    const uint8_t* code, size_t code_size)
{
    lifter->clear_block_cache();

    auto as = std::make_shared<loader::AddressSpace>();
    as->register_segment("code", 0x400000, code, code_size, 0);

    auto state = std::make_shared<state::State>(as, lifter, 0x400000);
    return state;
}

TEST_CASE("Execute Block 1", "[expr]")
{
    const uint8_t code[] = "\x48\xC7\xC0\x0A\x00\x00\x00" //   mov rax,0xa
                           "\x48\xC7\xC3\x0A\x00\x00\x00" //   mov rbx,0xa
                           "\x48\x39\xD8"                 //   cmp rax,rbx
                           "\x74\x00"                     //   je RET
                                                          // RET:
                           "\xC3";                        //   ret

    auto state = get_state_executing(get_x86_64_lifter(), code, sizeof(code));

    executor::PCodeExecutor executor(get_x86_64_lifter());
    auto                    successors = executor.execute_basic_block(state);
    REQUIRE(successors.size() == 1);
    REQUIRE(successors.at(0)->pc() == 0x400013);
}

TEST_CASE("Execute Block 2", "[expr]")
{
    const uint8_t code[] = "\x31\xC0"                 //   xor eax,eax
                           "\x81\xF1\xDD\xCC\xBB\xAA" //   xor ecx,0xaabbccdd
                           "\x83\xF9\xFF"             //   cmp ecx,0xffffffff
                           "\x75\x02"                 //   jne RET
                           "\xFF\xC0"                 //   inc eax
                                                      // RET:
                           "\xC3";                    //   ret

    auto state = get_state_executing(get_x86_64_lifter(), code, sizeof(code));
    auto sym   = exprBuilder.mk_sym("sym", 32);
    state->reg_write("ECX", sym);

    executor::PCodeExecutor executor(get_x86_64_lifter());
    auto                    successors = executor.execute_basic_block(state);
    REQUIRE(successors.size() == 2);
    REQUIRE(successors.at(0)->pc() == 0x40000f);

    expr::BVConst eval_sym = successors.at(1)->solver().evaluate(sym);
    REQUIRE(eval_sym.as_u64() == 0x55443322UL);
}
