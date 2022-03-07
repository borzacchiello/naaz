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
    // FIXME: should be obtained dynamically
    uint64_t external_addr = 0x5000000;

    // register exit function
    state.register_linked_function(external_addr, m_models.at("exit"));
    external_addr += state.arch().ptr_size() / 8;

    for (auto const& [addr, syms] : state.address_space()->symbols()) {
        for (auto const& sym : syms) {
            if (sym.type() == loader::Symbol::Type::EXT_FUNCTION) {
                if (!m_models.contains(sym.name()))
                    info("Linker") << "link(): unmodeled linked function "
                                   << sym.name() << std::endl;
                else {
                    auto fun_addr = expr::ExprBuilder::The().mk_const(
                        external_addr, state.arch().ptr_size());
                    state.write(addr, fun_addr);
                    state.register_linked_function(external_addr,
                                                   m_models.at(sym.name()));
                    external_addr += state.arch().ptr_size() / 8;
                }
                break;
            }
        }
    }
}

} // namespace naaz::models
