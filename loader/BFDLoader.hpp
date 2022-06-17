#pragma once

#define PACKAGE         ""
#define PACKAGE_VERSION ""

#include <bfd.h>
#include <filesystem>

#include "Loader.hpp"
#include "../lifter/PCodeLifter.hpp"

namespace naaz::loader
{

class BFDLoader : public Loader
{
  private:
    std::filesystem::path                m_filename;
    bfd*                                 m_obj;
    uint64_t                             m_entrypoint;
    const Arch*                          m_arch;
    BinaryType                           m_bin_type;
    SyscallABI                           m_syscall_abi;
    std::shared_ptr<AddressSpace>        m_address_space;
    std::shared_ptr<lifter::PCodeLifter> m_lifter;

    void load_sections();

    void process_symtable(asymbol* symtab[], size_t number_of_symbols);
    void load_symtab();
    void load_dyn_symtab();
    void load_relocs();
    void load_dyn_relocs();
    void deduce_syscall_abi();

  public:
    BFDLoader(const std::filesystem::path& filename);
    ~BFDLoader();

    virtual std::shared_ptr<AddressSpace> address_space();
    virtual const Arch&                   arch() const;
    virtual BinaryType                    bin_type() const;
    virtual SyscallABI                    syscall_abi() const;
    virtual uint64_t                      entrypoint() const;
    virtual state::StatePtr               entry_state() const;
};

} // namespace naaz::loader
