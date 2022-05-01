#include <string.h>
#include <pugixml.hpp>
#include <cassert>
#include <iostream>
#include <iomanip>
#include <filesystem>

#include "PCodeLifter.hpp"
#include "../util/ioutil.hpp"
#include "../util/strutil.hpp"

namespace naaz::lifter
{

std::string PCodeBlock::varnode_to_string(csleigh_Varnode varnode) const
{
    const char* space_name = csleigh_AddrSpace_getName(varnode.space);
    if (strcmp(space_name, "const") == 0) {
        return string_format("0x%lx", varnode.offset);
    } else if (strcmp(space_name, "unique") == 0) {
        return string_format("TMP_%u:%u", varnode.offset, varnode.size);
    } else if (strcmp(space_name, "register") == 0) {
        return m_lifter.reg_name(varnode);
    }
    return string_format("%s[0x%lx:%ld]", space_name, varnode.offset,
                         varnode.size);
}

std::string PCodeBlock::load_to_string(csleigh_PcodeOp op) const
{
    assert(op.opcode == csleigh_CPUI_LOAD &&
           "PCodeBlock::load_to_string(): invalid PcodeOp");
    csleigh_Address   addr = {.space  = op.inputs[0].space,
                              .offset = op.inputs[0].offset};
    csleigh_AddrSpace as   = csleigh_Addr_getSpaceFromConst(&addr);

    return varnode_to_string(*op.output) + " <- " +
           string_format("%s[", csleigh_AddrSpace_getName(as)) +
           varnode_to_string(op.inputs[1]) + "]";
}

std::string PCodeBlock::store_to_string(csleigh_PcodeOp op) const
{
    assert(op.opcode == csleigh_CPUI_STORE &&
           "PCodeBlock::store_to_string(): invalid PcodeOp");
    csleigh_Address   addr = {.space  = op.inputs[0].space,
                              .offset = op.inputs[0].offset};
    csleigh_AddrSpace as   = csleigh_Addr_getSpaceFromConst(&addr);

    return string_format("%s[", csleigh_AddrSpace_getName(as)) +
           varnode_to_string(op.inputs[1]) + "] <- " +
           varnode_to_string(op.inputs[2]);
}

void PCodeBlock::pp() const
{
    for (uint32_t i = 0; i < m_translation->instructions_count; ++i) {
        csleigh_Translation* inst = &m_translation->instructions[i];
        pp_stream() << string_format("0x%08lxh : %s %s", inst->address.offset,
                                     inst->asm_mnem, inst->asm_body)
                    << std::endl;
        pp_stream() << "--- PCODE ---" << std::endl;
        for (uint32_t j = 0; j < inst->ops_count; ++j) {
            csleigh_PcodeOp op = inst->ops[j];
            pp_stream() << "            | ";
            pp_stream() << std::left << std::setw(15) << std::setfill(' ')
                        << csleigh_OpCodeName(op.opcode);
            switch (op.opcode) {
                case csleigh_CPUI_LOAD: {
                    pp_stream() << load_to_string(op) << std::endl;
                    break;
                }
                case csleigh_CPUI_STORE: {
                    pp_stream() << store_to_string(op) << std::endl;
                    break;
                }
                default: {
                    if (op.output)
                        pp_stream() << varnode_to_string(*op.output) << " <- ";
                    if (op.inputs_count > 0) {
                        pp_stream() << varnode_to_string(op.inputs[0]);
                        for (uint32_t k = 1; k < op.inputs_count; ++k)
                            pp_stream()
                                << ", " << varnode_to_string(op.inputs[k]);
                    }
                    pp_stream() << std::endl;
                    break;
                }
            }
        }
        pp_stream() << "-------------" << std::endl;
    }
}

static inline bool file_exists(const std::filesystem::path& filename)
{
    ifstream f(filename.c_str());
    return f.good();
}

PCodeLifter::PCodeLifter(const Arch& arch) : m_arch(arch)
{
    if (!file_exists(arch.getSleighSLA())) {
        err("PCodeLifter")
            << "unable to find Sleigh's SLA file. Set the environment variable "
               "SLEIGH_PROCESSORS with the path to Sleigh's processors "
               "(/path/to/naaz/third_party/sleigh/processors)"
            << std::endl;
        exit_fail();
    }

    m_ctx = csleigh_createContext(arch.getSleighSLA().c_str());

    pugi::xml_document     doc;
    pugi::xml_parse_result result =
        doc.load_file(arch.getSleighPSPEC().c_str());
    if (!result) {
        abort();
    }

    // FIXME: maybe this thing should be done in csleigh lib
    for (pugi::xml_node tool : doc.child("processor_spec")
                                   .child("context_data")
                                   .child("context_set")
                                   .children("set")) {

        std::string name = tool.attribute("name").as_string();
        int         val  = tool.attribute("val").as_int();

        csleigh_setVariableDefault(m_ctx, name.c_str(), val);
    }

    auto ffs = csleigh_Sleigh_getFloatFormats(m_ctx);
    for (auto ff : ffs)
        m_float_formats.push_back(FloatFormatPtr(new FloatFormat(*ff)));
}

PCodeLifter::~PCodeLifter() { csleigh_destroyContext(m_ctx); }

const PCodeBlock* PCodeLifter::lift(uint64_t addr, const uint8_t* data,
                                    size_t data_size)
{
    if (m_blocks.contains(addr))
        return m_blocks.at(addr).get();

    csleigh_TranslationResult* r =
        csleigh_translate(m_ctx, data, data_size, addr, 0, true);
    if (!r) {
        err("PCodeLifter") << "Unable to lift block" << std::endl;
        exit_fail();
    }
    m_blocks[addr] = std::make_unique<PCodeBlock>(*this, r);

    return m_blocks.at(addr).get();
}

std::string PCodeLifter::reg_name(csleigh_Varnode v) const
{
    const char* space_name = csleigh_AddrSpace_getName(v.space);
    if (strcmp(space_name, "register") != 0) {
        err("PCodeLifter")
            << "register_name(): wrong varnode (not in `register` AddrSpace)"
            << std::endl;
        exit_fail();
    }

    return std::string(
        csleigh_Sleigh_getRegisterName(m_ctx, v.space, v.offset, v.size));
}

FloatFormatPtr PCodeLifter::get_float_format(int32_t size) const
{
    for (auto ff : m_float_formats) {
        if (ff->getSize() == size)
            return ff;
    }
    return nullptr;
}

csleigh_Varnode PCodeLifter::reg(const std::string& name) const
{
    return csleigh_Sleigh_getRegister(m_ctx, name.c_str());
}

void PCodeLifter::clear_block_cache() { m_blocks.clear(); }

uint32_t PCodeLifter::ram_space_id() const
{
    auto space = csleigh_Sleigh_getSpaceByName(m_ctx, "ram");
    return csleigh_AddrSpace_getId(space);
}

uint32_t PCodeLifter::regs_space_id() const
{
    auto space = csleigh_Sleigh_getSpaceByName(m_ctx, "register");
    return csleigh_AddrSpace_getId(space);
}

uint32_t PCodeLifter::const_space_id() const
{
    auto space = csleigh_Sleigh_getSpaceByName(m_ctx, "const");
    return csleigh_AddrSpace_getId(space);
}

uint32_t PCodeLifter::tmp_space_id() const
{
    auto space = csleigh_Sleigh_getSpaceByName(m_ctx, "unique");
    return csleigh_AddrSpace_getId(space);
}

} // namespace naaz::lifter
