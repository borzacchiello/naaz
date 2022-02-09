#pragma once

#include <optional>
#include <cstdint>
#include <vector>
#include <map>

#include "../arch/Arch.hpp"

#define PERM_READ  1
#define PERM_WRITE 2
#define PERM_EXEC  3

namespace naaz::loader
{

class Symbol
{
  public:
    enum Type { FUNCTION, EXT_FUNCTION, LOCAL, GLOBAL, UNKNOWN };

  private:
    uint64_t    m_addr;
    std::string m_name;
    Type        m_type;

  public:
    Symbol(uint64_t addr, const std::string& name, Type type);
};

class Segment
{
  private:
    std::string m_name;
    uint64_t    m_addr;
    uint8_t*    m_data;
    size_t      m_size;
    uint8_t     m_perm;

  public:
    Segment(const std::string& name, uint64_t addr, const uint8_t* data,
            size_t size, uint8_t perm);
    Segment(const std::string& name, uint64_t addr, size_t size, uint8_t perm);
    ~Segment();

    bool                   contains(uint64_t addr) const;
    std::optional<uint8_t> read(uint64_t addr) const;
    bool get_ref(uint64_t addr, uint8_t** o_data, size_t* o_size) const;
};

class AddressSpace
{
  private:
    std::vector<Segment>       m_segments;
    std::map<uint64_t, Symbol> m_symbols;

  public:
    AddressSpace() {}

    Segment& register_segment(const std::string& name, uint64_t addr,
                              std::vector<uint8_t> data, uint8_t perm);
    Segment& register_segment(const std::string& name, uint64_t addr,
                              const uint8_t* data, size_t size, uint8_t perm);
    Segment& register_segment(const std::string& name, uint64_t addr,
                              size_t size, uint8_t perm);

    std::optional<uint8_t>  read_byte(uint64_t addr) const;
    std::optional<uint16_t> read_word(uint64_t  addr,
                                      Endianess end = Endianess::LITTLE) const;
    std::optional<uint32_t> read_dword(uint64_t  addr,
                                       Endianess end = Endianess::LITTLE) const;
    std::optional<uint64_t> read_qword(uint64_t  addr,
                                       Endianess end = Endianess::LITTLE) const;

    bool get_ref(uint64_t addr, uint8_t** o_buf, size_t* o_size);

    void register_symbol(uint64_t addr, const std::string& name,
                         Symbol::Type type);
    std::optional<const Symbol*>      symbol_at(uint64_t addr) const;
    const std::map<uint64_t, Symbol>& symbols() const { return m_symbols; }
};

} // namespace naaz::loader
