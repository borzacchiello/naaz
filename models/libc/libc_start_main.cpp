#include "directory.hpp"

#include "../../expr/Expr.hpp"
#include "../../state/State.hpp"
#include "../../util/ioutil.hpp"

namespace naaz::models::libc
{

const libc_start_main& libc_start_main::The()
{
    static const libc_start_main v("__libc_start_main", CallConv::CDECL);
    return v;
}

bool libc_start_main::returns() const { return false; }

void libc_start_main::exec(state::State& s) const
{
    auto main_addr = s.get_int_param(m_call_conv, 0);
    if (main_addr->kind() != expr::Expr::Kind::CONST) {
        err("libc_start_main") << "main address is symbolic" << std::endl;
        exit_fail();
    }

    auto main_addr_conc =
        std::static_pointer_cast<const expr::ConstExpr>(main_addr)
            ->val()
            .as_u64();
    s.set_pc(main_addr_conc);
}

} // namespace naaz::models::libc
