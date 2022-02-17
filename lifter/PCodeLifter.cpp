#include <stdio.h>
#include <string.h>
#include <pugixml.hpp>

#include "PCodeLifter.hpp"
#include "../util/ioutil.hpp"
#include "../util/str_utils.hpp"

namespace naaz::lifter
{

std::string PCodeBlock::varnode_to_string(csleigh_Varnode varnode)
{
    const char* space_name = csleigh_AddrSpace_getName(varnode.space);
    if (strcmp(space_name, "const") == 0) {
        return string_format("0x%08lx", varnode.offset);
    } else if (strcmp(space_name, "register") == 0) {
        return m_lifter.register_name(varnode);
    }
    return string_format("%s[%08lx:%ld]", space_name, varnode.offset,
                         varnode.size);
}

std::string PCodeBlock::opcode_to_string(csleigh_OpCode op) { return ""; }

void PCodeBlock::pp() const
{
    printf("PCodeBlock:\n");
    printf("===========\n");
    for (uint32_t i = 0; i < m_translation->instructions_count; ++i) {
        csleigh_Translation* inst = &m_translation->instructions[i];
        printf("0x%08lxh : %s %s\n", inst->address.offset, inst->asm_mnem,
               inst->asm_body);
        printf("----\n");
        for (uint32_t j = 0; j < inst->ops_count; ++j) {
            csleigh_PcodeOp op = inst->ops[j];
        }
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

const PCodeBlock& PCodeLifter::lift(uint64_t addr, const uint8_t* data,
                                    size_t data_size)
{
    if (m_blocks.contains(addr))
        return m_blocks.at(addr);

    csleigh_TranslationResult* r =
        csleigh_translate(m_ctx, data, data_size, addr, 0, true);
    if (!r) {
        err("PCodeLifter") << "Unable to lift block" << std::endl;
        exit_fail();
    }
    PCodeBlock block(*this, r);
    m_blocks.emplace(addr, block);

    return m_blocks.at(addr);
}

std::string PCodeLifter::register_name(csleigh_Varnode v) const
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

} // namespace naaz::lifter
