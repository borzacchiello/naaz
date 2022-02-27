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

    ssize_t  size() const { return m_size; }
    uint64_t hash() const;

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
    void mul(const BVConst& other);
    void umul(const BVConst& other);
    void smul(const BVConst& other);
    void sdiv(const BVConst& other);
    void udiv(const BVConst& other);
    void srem(const BVConst& other);
    void urem(const BVConst& other);
    void neg();

    void band(const BVConst& other);
    void bor(const BVConst& other);
    void bxor(const BVConst& other);
    void bnot();

    void ashr(uint32_t v);
    void lshr(uint32_t v);
    void shl(uint32_t v);

    // comparison
    bool eq(const BVConst& other) const;
    bool ult(const BVConst& other) const;
    bool ule(const BVConst& other) const;
    bool ugt(const BVConst& other) const;
    bool uge(const BVConst& other) const;
    bool slt(const BVConst& other) const;
    bool sle(const BVConst& other) const;
    bool sgt(const BVConst& other) const;
    bool sge(const BVConst& other) const;

    bool is_zero() const;
    bool is_one() const;

    std::string to_string(bool hex = false) const;

    friend std::ostream& operator<<(std::ostream& os, const BVConst& c);
};

} // namespace naaz::expr
