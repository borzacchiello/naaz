#pragma once

#include <map>
#include "../arch/Arch.hpp"
#include "../third_party/sleigh/csleigh.h"

namespace naaz::lifter
{

class PCodeLifter;
class PCodeBlock
{
  private:
    const PCodeLifter&         m_lifter;
    csleigh_TranslationResult* m_translation;

    std::string varnode_to_string(csleigh_Varnode varnode);
    std::string opcode_to_string(csleigh_OpCode op);

  protected:
    PCodeBlock(const PCodeLifter&         lifter,
               csleigh_TranslationResult* translation)
        : m_lifter(lifter), m_translation(translation)
    {
    }

  public:
    ~PCodeBlock() { csleigh_freeResult(m_translation); }

    void                             pp() const;
    const csleigh_TranslationResult& transl() const { return *m_translation; }

    friend class PCodeLifter;
};

class PCodeLifter
{
  private:
    csleigh_Context                m_ctx;
    const Arch&                    m_arch;
    std::map<uint64_t, PCodeBlock> m_blocks;

  public:
    PCodeLifter(const Arch& arch);
    ~PCodeLifter();

    const PCodeBlock& lift(uint64_t addr, const uint8_t* data,
                           size_t data_size);
    std::string       register_name(csleigh_Varnode v) const;
};

} // namespace naaz::lifter
