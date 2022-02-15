#include "AddressSpace.hpp"
#include "../util/ioutil.hpp"

#include <cstring>

namespace naaz::loader
{

const std::string& Symbol::type_to_string(Type t)
{
    switch (t) {
        case Type::FUNCTION: {
            static const std::string name = "function    ";
            return name;
        }
        case Type::EXT_FUNCTION: {
            static const std::string name = "ext_function";
            return name;
        }
        case Type::LOCAL: {
            static const std::string name = "local       ";
            return name;
        }
        case Type::GLOBAL: {
            static const std::string name = "global      ";
            return name;
        }
        default:
            break;
    }
    static const std::string name = "unknown     ";
    return name;
}

Segment::Segment(Segment&& other)
    : m_name(other.m_name), m_addr(other.m_addr), m_size(other.m_size),
      m_data(std::move(other.m_data)), m_perm(other.m_perm)
{
}

Segment::Segment(const std::string& name, uint64_t addr, const uint8_t* data,
                 size_t size, uint8_t perm)
    : m_name(name), m_addr(addr), m_size(size), m_perm(perm)
{
    m_data = std::unique_ptr<uint8_t[]>{
        new uint8_t[size]() // parenthesis are crucial to zero-initialize the
                            // buffer
    };
    memcpy(m_data.get(), data, size);
}

Segment::Segment(const std::string& name, uint64_t addr, size_t size,
                 uint8_t perm)
    : m_name(name), m_addr(addr), m_size(size), m_perm(perm)
{
    m_data = std::unique_ptr<uint8_t[]>{
        new uint8_t[size]() // parenthesis are crucial to zero-initialize the
                            // buffer
    };
}

Segment::~Segment() {}

bool Segment::contains(uint64_t addr) const
{
    return addr >= m_addr && addr < m_addr + m_size;
}

Symbol::Symbol(uint64_t addr, const std::string& name, Type type)
    : m_addr(addr), m_name(name), m_type(type)
{
}

std::optional<uint8_t> Segment::read(uint64_t addr) const
{
    if (!contains(addr))
        return {};
    return m_data[addr - m_addr];
}

bool Segment::get_ref(uint64_t addr, uint8_t** o_data, size_t* o_size) const
{
    if (!contains(addr))
        return false;

    *o_data = m_data.get() + (addr - m_addr);
    *o_size = (m_size - (addr - m_addr));
    return true;
}

Segment& AddressSpace::register_segment(const std::string& name, uint64_t addr,
                                        std::vector<uint8_t> data, uint8_t perm)
{
    return register_segment(name, addr, data.data(), data.size(), perm);
}

Segment& AddressSpace::register_segment(const std::string& name, uint64_t addr,
                                        const uint8_t* data, size_t size,
                                        uint8_t perm)
{
    m_segments.emplace_back(name, addr, data, size, perm);
    return m_segments.back();
}

Segment& AddressSpace::register_segment(const std::string& name, uint64_t addr,
                                        size_t size, uint8_t perm)
{
    m_segments.emplace_back(name, addr, size, perm);
    return m_segments.back();
}

std::optional<uint8_t> AddressSpace::read_byte(uint64_t addr) const
{
    for (auto& s : m_segments) {
        if (s.contains(addr))
            return s.read(addr);
    }
    return {};
}

std::optional<uint16_t> AddressSpace::read_word(uint64_t  addr,
                                                Endianess end) const
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

std::optional<uint32_t> AddressSpace::read_dword(uint64_t  addr,
                                                 Endianess end) const
{
    std::optional<uint16_t> w1 = read_word(addr);
    std::optional<uint16_t> w2 = read_word(addr + 2);

    if (!w1.has_value() || !w2.has_value())
        return {};

    return end == Endianess::BIG ? ((uint32_t)w1.value() << 16U) | w2.value()
                                 : ((uint32_t)w2.value() << 16U) | w1.value();
}

std::optional<uint64_t> AddressSpace::read_qword(uint64_t  addr,
                                                 Endianess end) const
{
    std::optional<uint32_t> d1 = read_dword(addr);
    std::optional<uint32_t> d2 = read_dword(addr + 4);

    if (!d1.has_value() || !d2.has_value())
        return {};

    return end == Endianess::BIG ? ((uint64_t)d1.value() << 32U) | d2.value()
                                 : ((uint64_t)d2.value() << 32U) | d1.value();
}

bool AddressSpace::get_ref(uint64_t addr, uint8_t** o_buf, size_t* o_size)
{
    for (auto& s : m_segments) {
        if (s.contains(addr))
            return s.get_ref(addr, o_buf, o_size);
    }
    return false;
}

void AddressSpace::register_symbol(uint64_t addr, const std::string& name,
                                   Symbol::Type type)
{
    if (m_symbols.contains(addr)) {
        m_symbols.at(addr).emplace_back(addr, name, type);
    }

    std::vector<Symbol> syms;
    syms.emplace_back(addr, name, type);
    m_symbols.emplace(addr, syms);
}

std::optional<const Symbol*>
AddressSpace::ext_function_symbol_at(uint64_t addr) const
{
    if (!m_symbols.contains(addr))
        return {};

    for (auto& sym : m_symbols.at(addr)) {
        if (sym.type() == Symbol::Type::EXT_FUNCTION)
            return &sym;
    }
    return {};
}

} // namespace naaz::loader
