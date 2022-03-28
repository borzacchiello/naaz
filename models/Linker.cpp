#include "Linker.hpp"

#include "../state/State.hpp"
#include "../expr/ExprBuilder.hpp"
#include "../loader/AddressSpace.hpp"
#include "../util/ioutil.hpp"
#include "libc/directory.hpp"

namespace naaz::models
{

Linker& Linker::The()
{
    static Linker l;
    REGISTER_LIBC_FUNCTIONS;

    return l;
}

void Linker::register_model(const std::string& name, const Model* m)
{
    m_models.emplace(name, m);
}

void Linker::link(state::State& state) const
{
    uint64_t external_addr = state.arch().get_ext_func_base();

    // register libc_start_main_exit_wrapper function
    state.register_linked_function(external_addr,
                                   m_models.at("libc_start_main_exit_wrapper"));
    external_addr += state.arch().ptr_size() / 8;

    for (auto const& reloc : state.address_space()->relocations()) {
        if (reloc.type() == loader::Relocation::Type::FUNC) {
            if (!m_models.contains(reloc.name()))
                info("Linker") << "link(): unmodeled linked function "
                               << reloc.name() << std::endl;
            else {
                auto fun_addr = expr::ExprBuilder::The().mk_const(
                    external_addr, state.arch().ptr_size());
                state.write(reloc.addr(), fun_addr);
                state.register_linked_function(external_addr,
                                               m_models.at(reloc.name()));
                external_addr += state.arch().ptr_size() / 8;
            }
        }
    }
}

} // namespace naaz::models
