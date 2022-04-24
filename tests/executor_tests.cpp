#include <catch2/catch_all.hpp>
#include <memory>

#include "../util/config.hpp"
#include "../arch/x86_64.hpp"
#include "../expr/Expr.hpp"
#include "../expr/ExprBuilder.hpp"
#include "../state/State.hpp"
#include "../loader/AddressSpace.hpp"
#include "../lifter/PCodeLifter.hpp"
#include "../executor/PCodeExecutor.hpp"
#include "../executor/BFSExplorationTechnique.hpp"
#include "../executor/DFSExplorationTechnique.hpp"
#include "../executor/RandDFSExplorationTechnique.hpp"

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

TEST_CASE("Execute Block 1", "[executor]")
{
    const uint8_t code[] = "\x48\xC7\xC0\x0A\x00\x00\x00" //   mov rax,0xa
                           "\x48\xC7\xC3\x0A\x00\x00\x00" //   mov rbx,0xa
                           "\x48\x39\xD8"                 //   cmp rax,rbx
                           "\x74\x00"                     //   je RET
                                                          // RET:
                           "\xC3";                        //   ret

    auto state = get_state_executing(get_x86_64_lifter(), code, sizeof(code));

    executor::PCodeExecutor executor(get_x86_64_lifter());
    auto successors = executor.execute_basic_block(state).active;
    REQUIRE(successors.size() == 1);
    REQUIRE(successors.at(0)->pc() == 0x400013);
}

TEST_CASE("Execute Block 2", "[executor]")
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
    auto successors = executor.execute_basic_block(state).active;
    REQUIRE(successors.size() == 2);
    REQUIRE(successors.at(0)->pc() == 0x40000f);
    REQUIRE(successors.at(1)->pc() == 0x40000d);

    expr::BVConst eval_sym = successors.at(1)->solver().evaluate(sym).value();
    REQUIRE(eval_sym.as_u64() == 0x55443322UL);
}

TEST_CASE("Explore BFS 1", "[executor]")
{
    const uint8_t code[] = "\x31\xC0"             // 0x400000:    xor eax, eax
                                                  //           L:
                           "\x83\xFF\x0A"         // 0x400002:    cmp edi, 0xa
                           "\x73\x06"             // 0x400005:    jae OUT
                           "\xFF\xC0"             // 0x400007:    inc eax
                           "\xFF\xC7"             // 0x400009:    inc edi
                           "\xEB\xF5"             // 0x40000b:    jmp L
                                                  //         OUT:
                           "\x83\xF8\x07"         // 0x40000d:    cmp eax, 7
                           "\x75\x05"             // 0x400010:    jne RET
                           "\xB8\x2A\x00\x00\x00" // 0x400012:    mov eax, 42
                                                  //         RET:
                           "\xC3";                // 0x400017:    ret

    auto state = get_state_executing(get_x86_64_lifter(), code, sizeof(code));
    auto sym   = exprBuilder.mk_sym("sym", 32);
    state->reg_write("EDI", sym);

    executor::BFSExecutorManager em(state);

    std::vector<uint64_t> find;
    find.push_back(0x400012);
    std::vector<uint64_t> avoid;
    avoid.push_back(0x400017);
    std::optional<state::StatePtr> s = em.explore(find, avoid);

    REQUIRE(s.has_value());
    REQUIRE(s.value()->solver().evaluate(sym).value().as_u64() == 3);
}

TEST_CASE("Explore DFS 1", "[executor]")
{
    const uint8_t code[] = "\x31\xC0"             // 0x400000:    xor eax, eax
                                                  //           L:
                           "\x83\xFF\x0A"         // 0x400002:    cmp edi, 0xa
                           "\x73\x06"             // 0x400005:    jae OUT
                           "\xFF\xC0"             // 0x400007:    inc eax
                           "\xFF\xC7"             // 0x400009:    inc edi
                           "\xEB\xF5"             // 0x40000b:    jmp L
                                                  //         OUT:
                           "\x83\xF8\x07"         // 0x40000d:    cmp eax, 7
                           "\x75\x05"             // 0x400010:    jne RET
                           "\xB8\x2A\x00\x00\x00" // 0x400012:    mov eax, 42
                                                  //         RET:
                           "\xC3";                // 0x400017:    ret

    auto state = get_state_executing(get_x86_64_lifter(), code, sizeof(code));
    auto sym   = exprBuilder.mk_sym("sym", 32);
    state->reg_write("EDI", sym);

    // Infinite loop with lazy solving + DFS
    g_config.lazy_solving = false;
    executor::DFSExecutorManager em(state);

    std::vector<uint64_t> find;
    find.push_back(0x400012);
    std::vector<uint64_t> avoid;
    avoid.push_back(0x400017);
    std::optional<state::StatePtr> s = em.explore(find, avoid);

    REQUIRE(s.has_value());
    REQUIRE(s.value()->solver().evaluate(sym).value().as_u64() == 3);
}

TEST_CASE("Explore RandDFS 1", "[executor]")
{
    const uint8_t code[] = "\x31\xC0"             // 0x400000:    xor eax, eax
                                                  //           L:
                           "\x83\xFF\x0A"         // 0x400002:    cmp edi, 0xa
                           "\x73\x06"             // 0x400005:    jae OUT
                           "\xFF\xC0"             // 0x400007:    inc eax
                           "\xFF\xC7"             // 0x400009:    inc edi
                           "\xEB\xF5"             // 0x40000b:    jmp L
                                                  //         OUT:
                           "\x83\xF8\x07"         // 0x40000d:    cmp eax, 7
                           "\x75\x05"             // 0x400010:    jne RET
                           "\xB8\x2A\x00\x00\x00" // 0x400012:    mov eax, 42
                                                  //         RET:
                           "\xC3";                // 0x400017:    ret

    auto state = get_state_executing(get_x86_64_lifter(), code, sizeof(code));
    auto sym   = exprBuilder.mk_sym("sym", 32);
    state->reg_write("EDI", sym);

    executor::RandDFSExecutorManager em(state);

    std::vector<uint64_t> find;
    find.push_back(0x400012);
    std::vector<uint64_t> avoid;
    avoid.push_back(0x400017);
    std::optional<state::StatePtr> s = em.explore(find, avoid);

    REQUIRE(s.has_value());
    REQUIRE(s.value()->solver().evaluate(sym).value().as_u64() == 3);
}
