#include <stdio.h>

#include "PCodeLifter.hpp"

namespace naaz::lifter
{

void PCodeBlock::pp() const
{
    printf("PCodeBlock:\n");
    printf("===========\n");
    for (uint32_t i = 0; i < m_translation->instructions_count; ++i) {
        csleigh_Translation* inst = &m_translation->instructions[i];
        printf("0x%08lxh : %s %s\n", inst->address.offset, inst->asm_mnem,
               inst->asm_body);
    }
}

PCodeLifter::PCodeLifter(const Arch& arch, loader::AddressSpace& as)
    : m_arch(arch), m_as(as)
{
    m_ctx = csleigh_createContext(arch.getSleighSLA().c_str());
}

PCodeLifter::~PCodeLifter() { csleigh_destroyContext(m_ctx); }

const PCodeBlock& PCodeLifter::lift(uint64_t addr)
{
    if (m_blocks.contains(addr))
        return m_blocks.at(addr);

    const uint8_t* buf;
    size_t         size;
    if (!m_as.get_ref(addr, &buf, &size)) {
        abort();
    }

    csleigh_TranslationResult* r =
        csleigh_translate(m_ctx, buf, size, addr, 0, true);
    m_blocks.emplace(addr, r);

    return m_blocks.at(addr);
}

} // namespace naaz::lifter
