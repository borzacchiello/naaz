#pragma once

#include "../Linker.hpp"

namespace naaz::models::libc
{

#define GEN_MODEL_CLASS(model_name, call_conv)                                 \
    class model_name final : public Model                                      \
    {                                                                          \
        model_name() : Model(#model_name, call_conv) {}                        \
                                                                               \
      public:                                                                  \
        static const model_name& The()                                         \
        {                                                                      \
            static const model_name v;                                         \
            return v;                                                          \
        }                                                                      \
        virtual void exec(state::StatePtr           s,                         \
                          executor::ExecutorResult& o_successors) const;       \
    };

GEN_MODEL_CLASS(libc_start_main_exit_wrapper, CallConv::CDECL)
GEN_MODEL_CLASS(__libc_start_main, CallConv::CDECL)
GEN_MODEL_CLASS(exit, CallConv::CDECL)
GEN_MODEL_CLASS(read, CallConv::CDECL)
GEN_MODEL_CLASS(open, CallConv::CDECL)
GEN_MODEL_CLASS(close, CallConv::CDECL)

} // namespace naaz::models::libc

#define REGISTER_LIBC_FUNCTIONS                                                \
    l.register_model(libc::libc_start_main_exit_wrapper::The().name(),         \
                     &libc::libc_start_main_exit_wrapper::The());              \
    l.register_model(libc::__libc_start_main::The().name(),                    \
                     &libc::__libc_start_main::The());                         \
    l.register_model(libc::exit::The().name(), &libc::exit::The());            \
    l.register_model(libc::read::The().name(), &libc::read::The());            \
    l.register_model(libc::open::The().name(), &libc::open::The());            \
    l.register_model(libc::close::The().name(), &libc::close::The());
