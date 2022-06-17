#pragma once

#include "../SyscallModel.hpp"

namespace naaz::models::linux_syscalls
{

#define GEN_MODEL_CLASS(model_name)                                            \
    class model_name final : public SyscallModel                               \
    {                                                                          \
        model_name() : SyscallModel(#model_name) {}                            \
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

GEN_MODEL_CLASS(open)
GEN_MODEL_CLASS(read)

} // namespace naaz::models::linux_syscalls
