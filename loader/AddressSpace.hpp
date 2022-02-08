#pragma once

#include <optional>
#include <cstdint>
#include <vector>

#include "../arch/Arch.hpp"

#define PERM_READ  1
#define PERM_WRITE 2
#define PERM_EXEC  3

namespace naaz::loader
{

class Segment
{
  private:
    uint64_t m_addr;
    uint8_t* m_data;
    size_t   m_size;
    uint8_t  m_perm;

  public:
    Segment(uint64_t addr, const uint8_t* data, size_t size, uint8_t perm);
    ~Segment();

    bool                   contains(uint64_t addr) const;
    std::optional<uint8_t> read(uint64_t addr) const;
    bool get_ref(uint64_t addr, const uint8_t** o_data, size_t* o_size) const;
};

class AddressSpace
{
  private:
    std::vector<Segment> m_segments;

  public:
    AddressSpace() {}

    void register_segment(uint64_t addr, std::vector<uint8_t> data,
                          uint8_t perm);
    void register_segment(uint64_t addr, const uint8_t* data, size_t size,
                          uint8_t perm);
    void register_segment(const Segment& seg);

    std::optional<uint8_t>  read_byte(uint64_t addr) const;
    std::optional<uint16_t> read_word(uint64_t  addr,
                                      Endianess end = Endianess::LITTLE) const;
    std::optional<uint32_t> read_dword(uint64_t  addr,
                                       Endianess end = Endianess::LITTLE) const;
    std::optional<uint64_t> read_qword(uint64_t  addr,
                                       Endianess end = Endianess::LITTLE) const;

    bool get_ref(uint64_t addr, const uint8_t** o_buf, size_t* o_size);
};

} // namespace naaz::loader
