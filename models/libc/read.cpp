#include "directory.hpp"

#include "../../expr/Expr.hpp"
#include "../../expr/ExprBuilder.hpp"
#include "../../state/State.hpp"
#include "../../util/ioutil.hpp"
#include "../../util/strutil.hpp"

namespace naaz::models::libc
{

static uint64_t read_idx = 0;

void read::exec(state::StatePtr s, executor::ExecutorResult& o_successors) const
{
    auto fd   = s->get_int_param(m_call_conv, 0);
    auto buf  = s->get_int_param(m_call_conv, 1);
    auto size = s->get_int_param(m_call_conv, 2);

    if (fd->kind() != expr::Expr::Kind::CONST) {
        err("read") << "file descriptor is symbolic" << std::endl;
        exit_fail();
    }
    if (buf->kind() != expr::Expr::Kind::CONST) {
        err("read") << "buffer address is symbolic (FIXME)" << std::endl;
        exit_fail();
    }
    if (size->kind() != expr::Expr::Kind::CONST) {
        err("read") << "size is symbolic (FIXME)" << std::endl;
        exit_fail();
    }

    int fd_ =
        std::static_pointer_cast<const expr::ConstExpr>(fd)->val().as_s64();
    uint64_t buf_ =
        std::static_pointer_cast<const expr::ConstExpr>(buf)->val().as_u64();
    size_t size_ =
        std::static_pointer_cast<const expr::ConstExpr>(size)->val().as_u64();

    s->write(buf_, s->fs().read(fd_, size_));
    s->arch().handle_return(s, o_successors);
}

} // namespace naaz::models::libc
