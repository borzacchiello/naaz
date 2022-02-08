#pragma once

#include <map>
#include "../arch/Arch.hpp"
#include "../loader/AddressSpace.hpp"
#include "../third_party/sleigh/csleigh.h"

namespace naaz::lifter
{

class PCodeBlock
{
  private:
    csleigh_TranslationResult* m_translation;

  public:
    PCodeBlock(csleigh_TranslationResult* translation)
        : m_translation(translation)
    {
    }

    ~PCodeBlock() { csleigh_freeResult(m_translation); }

    void pp() const;

    const csleigh_TranslationResult& transl() const { return *m_translation; }
};

class PCodeLifter
{
  private:
    csleigh_Context                m_ctx;
    const Arch&                    m_arch;
    loader::AddressSpace&          m_as;
    std::map<uint64_t, PCodeBlock> m_blocks;

  public:
    PCodeLifter(const Arch& arch, loader::AddressSpace& as);
    ~PCodeLifter();

    const PCodeBlock& lift(uint64_t addr);
};

} // namespace naaz::lifter
