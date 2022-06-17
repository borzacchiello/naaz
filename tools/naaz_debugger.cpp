#include <cstdlib>
#include <cstdio>
#include <string>

#include <readline/readline.h>
#include <readline/history.h>

#include "../loader/BFDLoader.hpp"
#include "../executor/PCodeExecutor.hpp"

static void usage(const char* prog_name)
{
    printf("USAGE: %s <bin> [<arg1> ...]\n", prog_name);
    exit(1);
}

struct exec_context_t {
    naaz::state::StatePtr              state;
    naaz::executor::PCodeExecutor      executor;
    std::vector<naaz::state::StatePtr> deferred_states;
    std::vector<naaz::state::StatePtr> exited_states;

    exec_context_t(naaz::state::StatePtr _state)
        : state(_state), executor(_state->lifter())
    {
    }
};

struct cmd_t {
    void (*handler)(exec_context_t&);
    const char* name;
    const char* description;
};

static std::string get_state_info(naaz::state::StatePtr s)
{
    std::string res = "";
    if (s->is_linked_function(s->pc()))
        res += s->get_linked_model(s->pc())->name().c_str();
    return res;
}

static void cmd_context(exec_context_t& ctx)
{
    if (!ctx.state)
        return;

    printf("\ncode:\n=====\n");
    const naaz::lifter::PCodeBlock* block = ctx.state->curr_block();
    if (!block)
        printf(" ** unable disassemble at PC **\n");
    else
        block->pp(false);

    if (ctx.state->is_linked_function(ctx.state->pc()))
        printf(" in linked function: %s\n",
               ctx.state->get_linked_model(ctx.state->pc())->name().c_str());

    printf("\nstack:\n======\n");
    auto stack_ptr = ctx.state->reg_read(ctx.state->arch().get_stack_ptr_reg());
    if (stack_ptr->kind() != naaz::expr::Expr::Kind::CONST)
        printf(" ** stack pointer is symbolic **\n");
    else {
        auto stack_ptr_ =
            std::static_pointer_cast<const naaz::expr::ConstExpr>(stack_ptr);
        auto curr = stack_ptr_->val().as_u64();
        for (int i = 0; i < 10; ++i) {
            printf(" 0x%08llx  %s\n", curr,
                   ctx.state->read(curr, ctx.state->arch().ptr_size() / 8UL)
                       ->to_string()
                       .c_str());
            curr = curr - ctx.state->arch().ptr_size() / 8UL;
        }
    }

    printf("\nregisters:\n==========\n");
    int max_num_regs = 16;

    std::vector<csleigh_Register> regs = ctx.state->lifter()->regs();
    for (const auto& reg : regs) {
        if (reg.varnode.size * 8 != ctx.state->arch().ptr_size())
            continue;
        if (max_num_regs-- <= 0)
            break;
        printf(" %-13s %s\n", reg.name,
               ctx.state->reg_read(reg.name)->to_string().c_str());
    }
}

static void cmd_pcode(exec_context_t& ctx)
{
    if (!ctx.state)
        return;

    const naaz::lifter::PCodeBlock* block = ctx.state->curr_block();
    if (!block)
        printf(" ** unable disassemble at PC **\n");
    else
        block->pp();
}

static void cmd_list_states(exec_context_t& ctx)
{
    printf("current state:\n==============\n");
    if (ctx.state)
        printf(" [*] 0x%08llx %s\n", ctx.state->pc(),
               get_state_info(ctx.state).c_str());
    else
        printf(" * no state selected (use 'select' command to set it)\n");

    int i = 0;
    if (ctx.deferred_states.size() > 0) {
        printf("\ndeferred:\n=========\n");
        for (const auto state : ctx.deferred_states)
            printf(" [%d] 0x%08llx - %s \n", i++, state->pc(),
                   get_state_info(ctx.state).c_str());
    }

    if (ctx.exited_states.size() > 0) {
        printf("\nexited:\n=======\n");
        for (const auto state : ctx.exited_states)
            printf(" [%d] 0x%08llx %s\n", i++, state->pc(),
                   get_state_info(ctx.state).c_str());
    }
}

static void cmd_select_state(exec_context_t& ctx)
{
    char* arg = strtok(NULL, " ");
    if (!arg) {
        printf("!Err argument expected\n");
        return;
    }

    int id = atoi(arg);

    int i = 0;
    for (auto it = ctx.deferred_states.begin(); it != ctx.deferred_states.end();
         ++it) {
        if (i++ == id) {
            auto state = *it;
            ctx.deferred_states.erase(it);
            if (ctx.state != nullptr) {
                if (ctx.state->exited)
                    ctx.exited_states.push_back(ctx.state);
                else
                    ctx.deferred_states.push_back(ctx.state);
            }
            ctx.state = state;
            return;
        }
    }

    for (auto it = ctx.exited_states.begin(); it != ctx.exited_states.end();
         ++it) {
        if (i++ == id) {
            auto state = *it;
            ctx.exited_states.erase(it);
            if (ctx.state != nullptr) {
                if (ctx.state->exited)
                    ctx.exited_states.push_back(ctx.state);
                else
                    ctx.deferred_states.push_back(ctx.state);
            }
            ctx.state = state;
            return;
        }
    }

    printf("!Err invalid id %d\n", id);
}

static void cmd_step_state(exec_context_t& ctx)
{
    if (!ctx.state)
        return;

    if (ctx.state->exited) {
        printf("!Err cannot execute an exited state\n");
        return;
    }

    printf("executing... ");
    auto exec_return = ctx.executor.execute_basic_block(ctx.state);

    ctx.state = nullptr;
    for (int i = 0; i < exec_return.active.size(); ++i) {
        if (i == 0)
            ctx.state = exec_return.active[0];
        else
            ctx.deferred_states.push_back(exec_return.active[i]);
    }

    for (auto state : exec_return.exited)
        ctx.exited_states.push_back(state);

    printf("done\n\n");
    cmd_list_states(ctx);
}

static void cmd_exec_until(exec_context_t& ctx)
{
    if (!ctx.state)
        return;

    if (ctx.state->exited) {
        printf("!Err cannot execute an exited state\n");
        return;
    }

    char* arg = strtok(NULL, " ");
    if (!arg) {
        printf("!Err argument expected\n");
        return;
    }

    if (strcmp(arg, "fork") == 0) {
        printf("executing until fork... ");

        naaz::executor::ExecutorResult exec_return;
        while (1) {
            exec_return = ctx.executor.execute_basic_block(ctx.state);
            ctx.state   = nullptr;
            for (int i = 0; i < exec_return.active.size(); ++i) {
                if (i == 0)
                    ctx.state = exec_return.active[0];
                else
                    ctx.deferred_states.push_back(exec_return.active[i]);
            }
            for (auto state : exec_return.exited)
                ctx.exited_states.push_back(state);

            if (exec_return.active.size() == 0)
                break;
            if (exec_return.active.size() > 1)
                break;
        }

        printf("done\n\n");
        cmd_list_states(ctx);
    } else if (strcmp(arg, "nextblock") == 0) {
        printf("executing until next block... ");
        uint64_t old_pc = ctx.state->pc();

        naaz::executor::ExecutorResult exec_return;
        while (1) {
            exec_return = ctx.executor.execute_basic_block(ctx.state);
            ctx.state   = nullptr;
            for (int i = 0; i < exec_return.active.size(); ++i) {
                if (i == 0)
                    ctx.state = exec_return.active[0];
                else
                    ctx.deferred_states.push_back(exec_return.active[i]);
            }
            for (auto state : exec_return.exited)
                ctx.exited_states.push_back(state);

            if (ctx.state->pc() != old_pc)
                break;
            if (exec_return.active.size() == 0)
                break;
            if (exec_return.active.size() > 1)
                break;
        }

        printf("done\n\n");
        cmd_list_states(ctx);
    } else
        printf("!Err unknown arg \"%s\"\n", arg);
}

static void cmd_read(exec_context_t& ctx)
{
    if (!ctx.state)
        return;

    char* where = strtok(NULL, " ");
    char* len   = strtok(NULL, " ");
    if (!where || !len) {
        printf("!Err two arguments expected\n");
        return;
    }

    uint64_t addr;
    if (*where == '$') {
        std::string reg_name(where + 1);
        if (!ctx.state->lifter()->has_reg(reg_name)) {
            printf("!Err unknown register %s\n", reg_name.c_str());
            return;
        }
        auto reg_val = ctx.state->reg_read(reg_name);

        if (reg_val->kind() != naaz::expr::Expr::Kind::CONST) {
            printf("!Err register %s is symbolic\n", where + 1);
            return;
        }
        addr = std::static_pointer_cast<const naaz::expr::ConstExpr>(reg_val)
                   ->val()
                   .as_u64();
    } else
        addr = strtoul(where, NULL, 0);
    int n = atoi(len);

    auto data = ctx.state->read(addr, n);
    printf("%s\n", data->to_string().c_str());
}

static void cmd_pi(exec_context_t& ctx)
{
    if (!ctx.state)
        return;

    auto pi = ctx.state->solver().manager().pi();
    if (pi->kind() == naaz::expr::Expr::Kind::BOOL_AND) {
        auto pi_ = std::static_pointer_cast<const naaz::expr::BoolAndExpr>(pi);
        for (auto c : pi_->exprs())
            printf(" * %s\n", c->to_string().c_str());
    } else
        printf(" * %s\n", pi->to_string().c_str());
}

static void  cmd_help(exec_context_t& ctx);
static cmd_t handlers[] = {
    {cmd_help, "help", "show available commands"},
    {cmd_context, "context", "show informations about the current state"},
    {cmd_pcode, "pcode", "show pcode of current basic block"},
    {cmd_list_states, "list", "list active states"},
    {cmd_select_state, "select", "select <id>: select a new current state"},
    {cmd_step_state, "exec", "execute one basic block in the current state"},
    {cmd_exec_until, "exec-until",
     "exec-until <what>: execute the current state until {fork,nextblock}"},
    {cmd_read, "read",
     "read <addr> <len>: read len bytes starting from addr. addr can be a "
     "register (e.g. $RAX)"},
    {cmd_pi, "pi", "show the path constraint of the current state"},
};

static void cmd_help(exec_context_t& ctx)
{
    for (auto const& cmd : handlers)
        printf("%-15s: %s\n", cmd.name, cmd.description);
}

int main(int argc, const char* argv[])
{
    if (argc < 2)
        usage(argv[0]);

    printf(" _  _    __      __    ____     ____  ____   ___ \n"
           "( \\( )  /__\\    /__\\  (_   )___(  _ \\(  _ \\ / __)\n"
           " )  (  /(__)\\  /(__)\\  / /_(___))(_) )) _ <( (_-.\n"
           "(_)\\_)(__)(__)(__)(__)(____)   (____/(____/ \\___/\n\n");

    const char*             filename = argv[1];
    naaz::loader::BFDLoader loader(filename);

    std::vector<std::string> prog_args;
    for (int i = 1; i < argc; ++i)
        prog_args.push_back(argv[i]);

    auto           entry_state = loader.entry_state();
    exec_context_t ctx(entry_state);
    entry_state->set_argv(prog_args);

    printf("\n");

    char* buf;
    while ((buf = readline("naazdbg> ")) != nullptr) {
        if (strlen(buf) > 0) {
            add_history(buf);
        }

        char* cmd_name = strtok(buf, " ");
        if (!cmd_name)
            continue;
        for (const auto& cmd : handlers) {
            if (strcmp(cmd_name, cmd.name) == 0) {
                cmd.handler(ctx);
                break;
            }
        }

        // readline malloc's a new buffer every time.
        free(buf);
    }

    return 0;
}
