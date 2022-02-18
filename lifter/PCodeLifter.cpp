#include <string.h>
#include <pugixml.hpp>
#include <iostream>
#include <iomanip>

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

void PCodeBlock::pp() const
{
    pp_stream() << "PCodeBlock:" << std::endl;
    pp_stream() << "===========" << std::endl;
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
            if (op.output)
                pp_stream() << varnode_to_string(*op.output) << " <- ";
            if (op.inputs_count > 0) {
                pp_stream() << varnode_to_string(op.inputs[0]);
                for (uint32_t k = 1; k < op.inputs_count; ++k)
                    pp_stream() << ", " << varnode_to_string(op.inputs[k]);
            }
            pp_stream() << std::endl;
        }
        pp_stream() << "-------------" << std::endl;
    }
}

PCodeLifter::PCodeLifter(const Arch& arch) : m_arch(arch)
{
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

csleigh_Varnode PCodeLifter::reg(const std::string& name) const
{
    return csleigh_Sleigh_getRegister(m_ctx, name.c_str());
}

} // namespace naaz::lifter
