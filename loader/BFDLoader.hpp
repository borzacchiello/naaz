#pragma once

#define PACKAGE         ""
#define PACKAGE_VERSION ""

#include <bfd.h>
#include <filesystem>

#include "Loader.hpp"

namespace naaz::loader
{

class BFDLoader : public Loader
{
  private:
    std::filesystem::path m_filename;
    bfd*                  m_obj;
    AddressSpace          m_address_space;
    uint64_t              m_entrypoint;
    const Arch*           m_arch;
    BinaryType            m_bin_type;

    void load_sections();

    void process_symtable(asymbol* symtab[]);
    void load_symtab();
    void load_dyn_symtab();
    void load_dyn_relocs();

  public:
    BFDLoader(const std::filesystem::path& filename);
    ~BFDLoader();

    virtual AddressSpace& address_space();
    virtual const Arch&   arch() const;
    virtual BinaryType    bin_type() const;
    virtual uint64_t      entrypoint() const;
};

} // namespace naaz::loader
