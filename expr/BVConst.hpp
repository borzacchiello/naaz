#pragma once

#include <vector>
#include <cstdint>
#include <iostream>

#include "gmpxx.h"
#include "../arch/Arch.hpp"

namespace naaz::expr
{

class BVConst
{
    ssize_t   m_size;
    uint64_t  m_small_val;
    mpz_class m_big_val;

    void check_size_or_die(const BVConst& lhs, const BVConst& rhs) const;
    void adjust_bits();

    bool      is_big() const { return m_size > 64; }
    mpz_class as_mpz() const;

  public:
    BVConst(uint64_t value, ssize_t size);
    BVConst(const std::string& value, ssize_t size);
    BVConst(const uint8_t* data, ssize_t size, Endianess end = Endianess::BIG);
    BVConst(const std::vector<uint8_t>& data);
    BVConst(const BVConst& other);

    BVConst(const std::vector<uint8_t>& data, Endianess end = Endianess::BIG)
        : BVConst(data.data(), data.size(), end)
    {
    }

    ~BVConst();

    ssize_t size() const { return m_size; }

    uint64_t             as_u64() const;
    int64_t              as_s64() const;
    std::vector<uint8_t> as_data() const;

    // least significant first!
    uint8_t get_bit(uint64_t i) const;
    uint8_t get_byte(uint64_t i) const;

    void concat(const BVConst& other);
    void extract(uint32_t high, uint32_t low);
    void zext(uint32_t size);
    void sext(uint32_t size);

    // arithmetic
    void add(const BVConst& other);
    void sub(const BVConst& other);
    void ashr(uint32_t v);
    void lshr(uint32_t v);
    void shl(uint32_t v);

    // comparison

    std::string to_string(bool hex = false) const;

    friend std::ostream& operator<<(std::ostream& os, const BVConst& c);
};

} // namespace naaz::expr
