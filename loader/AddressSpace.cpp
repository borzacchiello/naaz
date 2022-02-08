#include "AddressSpace.hpp"

#include <cstring>

namespace naaz::loader
{

Segment::Segment(uint64_t addr, const uint8_t* data, size_t size, uint8_t perm)
    : m_addr(addr), m_size(size), m_perm(perm)
{
    m_data = (uint8_t*)malloc(size);
    memcpy(m_data, data, size);
}

Segment::~Segment() { free(m_data); }

bool Segment::contains(uint64_t addr) const
{
    return addr >= m_addr && addr < m_addr + m_size;
}

std::optional<uint8_t> Segment::read(uint64_t addr) const
{
    if (!contains(addr))
        return {};
    return m_data[addr - m_addr];
}

bool Segment::get_ref(uint64_t addr, const uint8_t** o_data,
                      size_t* o_size) const
{
    if (!contains(addr))
        return false;

    *o_data = m_data + (addr - m_addr);
    *o_size = (m_size - (addr - m_addr));
    return true;
}

void AddressSpace::register_segment(uint64_t addr, std::vector<uint8_t> data,
                                    uint8_t perm)
{
    register_segment(addr, data.data(), data.size(), perm);
}

void AddressSpace::register_segment(uint64_t addr, const uint8_t* data,
                                    size_t size, uint8_t perm)
{
    m_segments.emplace_back(addr, data, size, perm);
}

std::optional<uint8_t> AddressSpace::read_byte(uint64_t addr) const
{
    for (auto& s : m_segments) {
        if (s.contains(addr))
            return s.read(addr);
    }
    return {};
}

std::optional<uint16_t>
AddressSpace::read_word(uint64_t addr, Endianess end) const
{
    for (auto& s : m_segments) {
        if (s.contains(addr) && s.contains(addr + 1)) {
            uint8_t b1 = s.read(addr).value();
            uint8_t b2 = s.read(addr + 1).value();
            return end == Endianess::BIG ? ((uint16_t)b1 << 8U) | b2
                                         : ((uint16_t)b2 << 8U) | b1;
        }
    }
    return {};
}

std::optional<uint32_t>
AddressSpace::read_dword(uint64_t addr, Endianess end) const
{
    std::optional<uint16_t> w1 = read_word(addr);
    std::optional<uint16_t> w2 = read_word(addr + 2);

    if (!w1.has_value() || !w2.has_value())
        return {};

    return end == Endianess::BIG ? ((uint32_t)w1.value() << 16U) | w2.value()
                                 : ((uint32_t)w2.value() << 16U) | w1.value();
}

std::optional<uint64_t>
AddressSpace::read_qword(uint64_t addr, Endianess end) const
{
    std::optional<uint32_t> d1 = read_dword(addr);
    std::optional<uint32_t> d2 = read_dword(addr + 4);

    if (!d1.has_value() || !d2.has_value())
        return {};

    return end == Endianess::BIG ? ((uint64_t)d1.value() << 32U) | d2.value()
                                 : ((uint64_t)d2.value() << 32U) | d1.value();
}

bool AddressSpace::get_ref(uint64_t addr, const uint8_t** o_buf, size_t* o_size)
{
    for (auto& s : m_segments) {
        if (s.contains(addr))
            return s.get_ref(addr, o_buf, o_size);
    }
    return false;
}

} // namespace naaz::loader