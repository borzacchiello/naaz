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
GEN_MODEL_CLASS(abort, CallConv::CDECL)
GEN_MODEL_CLASS(read, CallConv::CDECL)
GEN_MODEL_CLASS(open, CallConv::CDECL)
GEN_MODEL_CLASS(close, CallConv::CDECL)
GEN_MODEL_CLASS(malloc, CallConv::CDECL)
GEN_MODEL_CLASS(calloc, CallConv::CDECL)
GEN_MODEL_CLASS(realloc, CallConv::CDECL)
GEN_MODEL_CLASS(free, CallConv::CDECL)
GEN_MODEL_CLASS(memcpy, CallConv::CDECL)
GEN_MODEL_CLASS(memcmp, CallConv::CDECL)
GEN_MODEL_CLASS(strcmp, CallConv::CDECL)
GEN_MODEL_CLASS(strlen, CallConv::CDECL)
GEN_MODEL_CLASS(strncpy, CallConv::CDECL)
GEN_MODEL_CLASS(strcpy, CallConv::CDECL)
GEN_MODEL_CLASS(strcat, CallConv::CDECL)
GEN_MODEL_CLASS(sprintf, CallConv::CDECL)
GEN_MODEL_CLASS(printf, CallConv::CDECL)
GEN_MODEL_CLASS(scanf, CallConv::CDECL)
GEN_MODEL_CLASS(puts, CallConv::CDECL)
GEN_MODEL_CLASS(fflush, CallConv::CDECL)
GEN_MODEL_CLASS(rand, CallConv::CDECL)
GEN_MODEL_CLASS(srand, CallConv::CDECL)
GEN_MODEL_CLASS(ptrace, CallConv::CDECL)

} // namespace naaz::models::libc

#define REGISTER_LIBC_FUNCTIONS                                                \
    l.register_model(libc::libc_start_main_exit_wrapper::The().name(),         \
                     &libc::libc_start_main_exit_wrapper::The());              \
    l.register_model(libc::__libc_start_main::The().name(),                    \
                     &libc::__libc_start_main::The());                         \
    l.register_model(libc::exit::The().name(), &libc::exit::The());            \
    l.register_model(libc::abort::The().name(), &libc::exit::The());           \
    l.register_model(libc::read::The().name(), &libc::read::The());            \
    l.register_model(libc::open::The().name(), &libc::open::The());            \
    l.register_model(libc::close::The().name(), &libc::close::The());          \
    l.register_model(libc::malloc::The().name(), &libc::malloc::The());        \
    l.register_model(libc::calloc::The().name(), &libc::calloc::The());        \
    l.register_model(libc::realloc::The().name(), &libc::realloc::The());      \
    l.register_model(libc::free::The().name(), &libc::free::The());            \
    l.register_model(libc::memcpy::The().name(), &libc::memcpy::The());        \
    l.register_model(libc::memcmp::The().name(), &libc::memcmp::The());        \
    l.register_model(libc::strlen::The().name(), &libc::strlen::The());        \
    l.register_model(libc::strcmp::The().name(), &libc::strcmp::The());        \
    l.register_model(libc::strncpy::The().name(), &libc::strncpy::The());      \
    l.register_model(libc::strcpy::The().name(), &libc::strcpy::The());        \
    l.register_model(libc::strcat::The().name(), &libc::strcat::The());        \
    l.register_model(libc::sprintf::The().name(), &libc::sprintf::The());      \
    l.register_model(libc::printf::The().name(), &libc::printf::The());        \
    l.register_model(libc::scanf::The().name(), &libc::scanf::The());          \
    l.register_model("__isoc99_scanf", &libc::scanf::The());                   \
    l.register_model(libc::fflush::The().name(), &libc::fflush::The());        \
    l.register_model(libc::puts::The().name(), &libc::puts::The());            \
    l.register_model(libc::rand::The().name(), &libc::rand::The());            \
    l.register_model(libc::srand::The().name(), &libc::srand::The());          \
    l.register_model(libc::ptrace::The().name(), &libc::ptrace::The());
