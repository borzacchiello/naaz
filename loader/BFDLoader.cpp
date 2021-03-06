#include "BFDLoader.hpp"

#include "../state/State.hpp"
#include "../util/ioutil.hpp"
#include "../arch/x86_64.hpp"
#include "../arch/arm32LE.hpp"

namespace naaz::loader
{

BFDLoader::BFDLoader(const std::filesystem::path& filename)
    : m_filename(filename), m_arch(NULL), m_obj(NULL)
{
    m_obj = bfd_openr(filename.c_str(), nullptr);
    if (m_obj == nullptr) {
        err("BFDLoader") << "bfd_openr failed." << std::endl;
        exit_fail();
    }
    if (!bfd_check_format(m_obj, bfd_object)) {
        err("BFDLoader") << "bfd_check_format failed." << std::endl;
        exit_fail();
    }
    bfd_set_error(bfd_error_no_error);

    m_entrypoint = bfd_get_start_address(m_obj);

    const bfd_arch_info_type* bfd_arch_info = bfd_get_arch_info(m_obj);
    switch (bfd_arch_info->mach) {
        case bfd_mach_x86_64:
            m_arch = &arch::x86_64::The();
            break;
        case bfd_mach_arm_6:
        case bfd_mach_arm_7:
            m_arch = &arch::arm32LE::The();
            break;
        default:
            err("BFDLoader")
                << "unsupported architecture " << bfd_arch_info->printable_name
                << " (" << bfd_arch_info->mach << ")" << std::endl;
            exit_fail();
    }

    m_lifter        = std::make_shared<lifter::PCodeLifter>(*m_arch);
    m_address_space = std::make_shared<AddressSpace>();

    bfd_flavour flavor = bfd_get_flavour(m_obj);
    if (bfd_get_flavour(m_obj) == bfd_target_unknown_flavour) {
        err("BFDLoader") << "unrecognized binary format ("
                         << bfd_errmsg(bfd_get_error()) << ")" << std::endl;
        exit_fail();
    }
    switch (flavor) {
        case bfd_target_elf_flavour:
            m_bin_type = BinaryType::ELF;
            break;
        case bfd_target_coff_flavour:
            m_bin_type = BinaryType::PE;
            break;
        default:
            err("BFDLoader") << "unsupported binary type " << m_obj->xvec->name
                             << " (" << flavor << ")" << std::endl;
            exit_fail();
    }

    load_sections();
    load_symtab();
    load_dyn_symtab();
    load_dyn_relocs();

    deduce_syscall_abi();
}

void BFDLoader::load_sections()
{
    for (asection* bfd_sec = m_obj->sections; bfd_sec != (asection*)NULL;
         bfd_sec           = bfd_sec->next) {

        if (bfd_section_size(bfd_sec) == 0)
            continue;

        uint8_t perm = PERM_READ | PERM_WRITE;
        if (bfd_section_flags(bfd_sec) & SEC_READONLY)
            perm &= ~PERM_WRITE;
        if (bfd_section_flags(bfd_sec) & EXEC_P)
            perm |= PERM_EXEC;

        std::string sec_name(bfd_section_name(bfd_sec));
        Segment&    segment = m_address_space->register_segment(
               sec_name, bfd_section_vma(bfd_sec), bfd_section_size(bfd_sec),
               perm);

        uint8_t* seg_data;
        size_t   seg_size;
        if (!segment.get_ref(bfd_section_vma(bfd_sec), &seg_data, &seg_size)) {
            err("BFDLoader") << "failing in getting segment data" << std::endl;
            exit_fail();
        }

        bfd_simple_get_relocated_section_contents(m_obj, bfd_sec, seg_data,
                                                  NULL);
    }
}

void BFDLoader::deduce_syscall_abi()
{
    if (m_bin_type == BinaryType::ELF && m_arch == &arch::x86_64::The())
        m_syscall_abi = SyscallABI::LINUX_X86_64;
    else if (m_bin_type == BinaryType::ELF && m_arch == &arch::arm32LE::The())
        m_syscall_abi = SyscallABI::LINUX_ARMv7;
    else
        m_syscall_abi = SyscallABI::UNKNOWN;
}

void BFDLoader::process_symtable(asymbol* symbol_table[],
                                 size_t   number_of_symbols)
{
    symbol_info symbolinfo;
    for (int i = 0; i < number_of_symbols; i++) {
        asymbol* symbol = symbol_table[i];

        bfd_symbol_info(symbol_table[i], &symbolinfo);
        if (symbolinfo.value == 0)
            continue;

        Symbol::Type type = Symbol::Type::UNKNOWN;
        if (symbol->flags & BSF_FUNCTION)
            type = Symbol::Type::FUNCTION;
        else if (symbol->flags & BSF_LOCAL)
            type = Symbol::Type::LOCAL;
        else if (symbol->flags & BSF_GLOBAL)
            type = Symbol::Type::GLOBAL;

        std::string symb_name(symbolinfo.name);
        m_address_space->register_symbol(symbolinfo.value, symb_name, type);
    }
}

void BFDLoader::load_symtab()
{
    long storage_needed = bfd_get_symtab_upper_bound(m_obj);
    if (storage_needed < 0) {
        err("BFDLoader") << "bfd_get_symtab_upper_bound failed." << std::endl;
        exit_fail();
    }
    if (storage_needed == 0) {
        return;
    }

    asymbol* symbol_table[storage_needed];
    long     number_of_symbols = bfd_canonicalize_symtab(m_obj, symbol_table);
    if (number_of_symbols < 0) {
        err("BFDLoader") << "bfd_canonicalize_symtab failed." << std::endl;
        exit_fail();
    }
    process_symtable(symbol_table, number_of_symbols);
}

void BFDLoader::load_dyn_symtab()
{
    long storage_needed = bfd_get_dynamic_symtab_upper_bound(m_obj);
    if (storage_needed < 0) {
        err("BFDLoader") << "bfd_get_dynamic_symtab_upper_bound failed."
                         << std::endl;
        exit_fail();
    }
    if (storage_needed == 0) {
        return;
    }

    asymbol* symbol_table[storage_needed];
    long     number_of_symbols =
        bfd_canonicalize_dynamic_symtab(m_obj, symbol_table);
    if (number_of_symbols < 0) {
        err("BFDLoader") << "bfd_canonicalize_dynamic_symtab failed."
                         << std::endl;
        exit_fail();
    }
    process_symtable(symbol_table, number_of_symbols);
}

void BFDLoader::load_dyn_relocs()
{
    long reloc_size = bfd_get_dynamic_reloc_upper_bound(m_obj);
    if (reloc_size < 0) {
        err("BFDLoader") << "bfd_get_dynamic_reloc_upper_bound failed"
                         << std::endl;
        exit_fail();
    }

    if (reloc_size == 0)
        return;

    long dyn_sym_size = bfd_get_dynamic_symtab_upper_bound(m_obj);
    if (dyn_sym_size < 0) {
        err("BFDLoader") << "bfd_get_dynamic_symtab_upper_bound failed."
                         << std::endl;
        exit_fail();
    }
    if (dyn_sym_size == 0) {
        return;
    }

    asymbol* dyn_symbols[dyn_sym_size];

    long num_dyn_symbols = bfd_canonicalize_dynamic_symtab(m_obj, dyn_symbols);
    if (num_dyn_symbols < 0) {
        err("BFDLoader") << "bfd_canonicalize_dynamic_symtab failed."
                         << std::endl;
        exit_fail();
    }

    arelent* relpp[reloc_size];
    long relcount = bfd_canonicalize_dynamic_reloc(m_obj, relpp, dyn_symbols);
    for (long i = 0; i < relcount; ++i) {
        arelent* reloc = relpp[i];
        if ((*reloc->sym_ptr_ptr)->flags & BSF_FUNCTION) {
            std::string fname((*reloc->sym_ptr_ptr)->name);
            uint64_t    addr = reloc->address;
            m_address_space->register_relocation(addr, fname,
                                                 Relocation::Type::FUNC);
        } else if ((*reloc->sym_ptr_ptr)->flags & BSF_GLOBAL) {
            std::string fname((*reloc->sym_ptr_ptr)->name);
            uint64_t    addr = reloc->address;
            m_address_space->register_relocation(addr, fname,
                                                 Relocation::Type::DATA);
        }
    }
}

BFDLoader::~BFDLoader()
{
    if (m_obj)
        bfd_close(m_obj);
}

std::shared_ptr<AddressSpace> BFDLoader::address_space()
{
    return m_address_space;
}
const Arch& BFDLoader::arch() const { return *m_arch; }
BinaryType  BFDLoader::bin_type() const { return m_bin_type; }
SyscallABI  BFDLoader::syscall_abi() const { return m_syscall_abi; }
uint64_t    BFDLoader::entrypoint() const { return m_entrypoint; }

state::StatePtr BFDLoader::entry_state() const
{
    auto state = std::make_shared<state::State>(m_address_space, m_lifter,
                                                m_entrypoint, m_syscall_abi);
    return state;
}

} // namespace naaz::loader
