#pragma once

#include <map>
#include <memory>

#include "../arch/Arch.hpp"
#include "../expr/FPConst.hpp"
#include "../third_party/sleigh/csleigh.h"

namespace naaz::lifter
{

class PCodeLifter;
class PCodeBlock
{
  private:
    const PCodeLifter&         m_lifter;
    csleigh_TranslationResult* m_translation;

    std::string varnode_to_string(csleigh_Varnode varnode) const;
    std::string load_to_string(csleigh_PcodeOp op) const;
    std::string store_to_string(csleigh_PcodeOp op) const;

  public:
    PCodeBlock(const PCodeLifter&         lifter,
               csleigh_TranslationResult* translation)
        : m_lifter(lifter), m_translation(translation)
    {
    }
    ~PCodeBlock() { csleigh_freeResult(m_translation); }

    void                             pp(bool show_pcode = true) const;
    const csleigh_TranslationResult* transl() const { return m_translation; }
};

class PCodeLifter
{
  private:
    csleigh_Context                                 m_ctx;
    const Arch&                                     m_arch;
    std::vector<csleigh_Register>                   m_registers;
    std::vector<FloatFormatPtr>                     m_float_formats;
    std::map<uint64_t, std::unique_ptr<PCodeBlock>> m_blocks;

  public:
    PCodeLifter(const Arch& arch);
    ~PCodeLifter();

    const PCodeBlock*             lift(uint64_t addr, const uint8_t* data,
                                       size_t data_size);
    std::string                   reg_name(csleigh_Varnode v) const;
    bool                          has_reg(const std::string& name) const;
    csleigh_Register              reg(const std::string& name) const;
    std::vector<csleigh_Register> regs() const;
    const Arch&                   arch() const { return m_arch; }

    FloatFormatPtr get_float_format(int32_t size) const;

    void clear_block_cache();

    uint32_t ram_space_id() const;
    uint32_t regs_space_id() const;
    uint32_t const_space_id() const;
    uint32_t tmp_space_id() const;
};

} // namespace naaz::lifter
