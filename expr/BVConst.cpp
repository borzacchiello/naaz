#include <cassert>
#include <sstream>

#include "BVConst.hpp"
#include "../util/ioutil.hpp"

#include "gmpxx.h"

namespace naaz::expr
{

static std::string mpz_to_string(const mpz_class& v, bool hex);

BVConst::BVConst(uint64_t value, ssize_t size) : m_size(size)
{
    if (size == 0) {
        err("BVConst") << "BVConst(): size cannot be zero" << std::endl;
        exit_fail();
    }

    m_size = size;
    if (size <= 64) {
        m_small_val = value;
        m_big_val   = mpz_class(0);
    } else {
        m_big_val = mpz_class(value);
    }
}

BVConst::BVConst(const std::string& value, ssize_t size) : m_size(size)
{
    if (size == 0) {
        err("BVConst") << "BVConst(): size cannot be zero" << std::endl;
        exit_fail();
    }

    std::string num(value);

    int base = 10;
    if (value[0] == '0' && value[1] == 'x') {
        base = 16;
        num  = num.substr(2);
    } else if (value[0] == '0' && value[1] == 'b') {
        base = 2;
        num  = num.substr(2);
    }

    if (size <= 64) {
        m_small_val = std::stoul(num, 0, base);
        m_big_val   = mpz_class(0);
    } else {
        m_big_val = mpz_class(num, base);
    }
}

BVConst::BVConst(const BVConst& other) : m_size(other.m_size)
{
    if (m_size <= 64) {
        m_small_val = other.m_small_val;
        m_big_val   = mpz_class(0);
    } else {
        m_big_val = mpz_class(other.m_big_val);
    }
}

BVConst::BVConst(const uint8_t* data, ssize_t size, Endianess end)
{
    m_size      = size * 8;
    m_big_val   = mpz_class(0);
    m_small_val = 0;
    if (size * 8 <= 64) {
        for (ssize_t i = 0; i < size; ++i) {
            uint32_t shift =
                end == Endianess::LITTLE ? (i * 8) : ((size - i - 1) * 8);
            m_small_val |= (uint64_t)data[i] << shift;
        }
    } else {
        for (ssize_t i = 0; i < size; ++i) {
            for (ssize_t j = 0; j < 8; ++j) {
                uint8_t bit = end == Endianess::LITTLE
                                  ? ((data[i] & (1 << j)) > 0)
                                  : ((data[size - i - 1] & (1 << j)) > 0);
                if (bit)
                    mpz_setbit(m_big_val.get_mpz_t(), i * 8 + j);
                else
                    mpz_clrbit(m_big_val.get_mpz_t(), i * 8 + j);
            }
        }
    }
}

BVConst::~BVConst() {}

void BVConst::check_size_or_die(const BVConst& lhs, const BVConst& rhs) const
{
    if (lhs.size() != rhs.size()) {
        err("BVConst") << "check_size_or_die(): different size (" << lhs.size()
                       << " : " << rhs.size() << ")" << std::endl;
        exit_fail();
    }
}

static inline int64_t _sext(uint64_t v, ssize_t size)
{
    if (size > 64) {
        err() << "sext(): size > 64 is not supported" << std::endl;
        exit_fail();
    }

    int64_t res = (int64_t)v;
    if (res & (1UL << (size - 1)))
        res |= ((2UL << (64UL - size)) - 1UL) << size;
    return res;
}

static inline uint64_t _bitmask(ssize_t size)
{
    if (size > 64) {
        err() << "_bitmask(): size > 64 is not supported" << std::endl;
        exit_fail();
    }

    uint64_t res = (2UL << (size - 1)) - 1UL;
    return res;
}

void BVConst::adjust_bits()
{
    if (m_size == 64)
        return;

    if (!is_big()) {
        m_small_val &= _bitmask(m_size);
    } else if (mpz_sizeinbase(m_big_val.get_mpz_t(), 2) > m_size) {
        mpz_t tmp;
        mpz_init_set_ui(tmp, 0);
        for (uint32_t i = 0; i < m_size; ++i) {
            if (mpz_tstbit(m_big_val.get_mpz_t(), i))
                mpz_setbit(tmp, i);
            else
                mpz_clrbit(tmp, i);
        }
        mpz_set(m_big_val.get_mpz_t(), tmp);
        mpz_clear(tmp);

        assert(mpz_sizeinbase(m_big_val.get_mpz_t(), 2) <= m_size &&
               "adjust_bits(): bits not truncated correctly");
    }
}

mpz_class BVConst::as_mpz() const
{
    if (is_big())
        return m_big_val;
    return mpz_class(m_small_val);
}

uint64_t BVConst::as_u64() const
{
    if (m_size > 64) {
        err("BVConst") << "as_u64(): the const cannot be converted to u64"
                       << std::endl;
        exit_fail();
    }

    return m_small_val;
}

int64_t BVConst::as_s64() const
{
    if (m_size > 64) {
        err("BVConst") << "as_s64(): the const cannot be converted to u64"
                       << std::endl;
        exit_fail();
    }

    return _sext(m_small_val, m_size);
}

uint8_t BVConst::get_bit(uint64_t idx) const
{
    if (idx >= m_size) {
        err("BVConst") << "get_bit(): the index is invalid (" << idx << ")"
                       << std::endl;
        exit_fail();
    }

    if (!is_big())
        return (uint8_t)((m_small_val & (1 << idx)) > 0UL);

    return mpz_tstbit(m_big_val.get_mpz_t(), idx);
}

uint8_t BVConst::get_byte(uint64_t idx) const
{
    uint64_t low  = idx * 8;
    uint64_t high = low + 7;

    if (high >= m_size) {
        err("BVConst") << "get_byte(): the index is invalid (" << idx << ")"
                       << std::endl;
        exit_fail();
    }

    if (!is_big())
        return (uint8_t)((m_small_val >> low) & _bitmask(high - low + 1));

    uint8_t res;
    for (uint64_t i = 0; i < 8; ++i) {
        if (i + low >= m_size)
            // not a multiple of 8
            break;

        res |= mpz_tstbit(m_big_val.get_mpz_t(), i + low) << i;
    }
    return res;
}

std::vector<uint8_t> BVConst::as_data() const
{
    std::vector<uint8_t> res;

    ssize_t num_bytes = m_size / 8UL;
    num_bytes += (m_size % 8 == 0) ? 0 : 1;

    for (ssize_t i = num_bytes - 1; i >= 0; --i)
        res.push_back(get_byte(i));
    return res;
}

void BVConst::add(const BVConst& other)
{
    check_size_or_die(*this, other);

    if (!is_big()) {
        m_small_val += other.m_small_val;
    } else {
        m_big_val += other.m_big_val;
    }

    adjust_bits();
}

void BVConst::sub(const BVConst& other)
{
    check_size_or_die(*this, other);

    if (!is_big()) {
        m_small_val -= other.m_small_val;
    } else {
        m_big_val -= other.m_big_val;
    }

    adjust_bits();
}

void BVConst::ashr(uint32_t v)
{
    if (v == 0)
        return;

    if (!is_big()) {
        m_small_val = (uint64_t)(_sext(m_small_val, m_size) >> v);
        adjust_bits();
    } else {
        mpz_class tmp(0);
        uint32_t  i;
        for (i = 0; i < m_size - v; ++i) {
            if (mpz_tstbit(m_big_val.get_mpz_t(), i + v))
                mpz_setbit(tmp.get_mpz_t(), i);
            else
                mpz_clrbit(tmp.get_mpz_t(), i);
        }

        if (mpz_tstbit(m_big_val.get_mpz_t(), m_size - 1)) {
            // sign extend
            for (; i < m_size; ++i)
                mpz_setbit(tmp.get_mpz_t(), i);
        }

        m_big_val = tmp;
    }
}

void BVConst::lshr(uint32_t v)
{
    if (v == 0)
        return;

    if (v >= m_size) {
        if (!is_big())
            m_small_val = 0;
        else
            m_big_val = mpz_class(0);
        return;
    }

    if (!is_big()) {
        m_small_val = m_small_val >> v;
        adjust_bits();
    } else {
        mpz_class tmp(0);
        for (uint32_t i = 0; i < m_size - v; ++i) {
            if (mpz_tstbit(m_big_val.get_mpz_t(), i + v))
                mpz_setbit(tmp.get_mpz_t(), i);
            else
                mpz_clrbit(tmp.get_mpz_t(), i);
        }

        m_big_val = tmp;
    }
}

void BVConst::shl(uint32_t v)
{
    if (v == 0)
        return;

    if (v >= m_size) {
        if (!is_big())
            m_small_val = 0;
        else
            m_big_val = mpz_class(0);
        return;
    }

    if (!is_big()) {
        m_small_val = m_small_val << v;
        adjust_bits();
    } else {
        mpz_class tmp(m_big_val);
        for (uint32_t i = v; i < m_size; ++i) {
            if (mpz_tstbit(m_big_val.get_mpz_t(), i - v) == 1)
                mpz_setbit(tmp.get_mpz_t(), i);
            else
                mpz_clrbit(tmp.get_mpz_t(), i);
        }

        m_big_val = tmp;
    }
}

void BVConst::concat(const BVConst& other)
{
    ssize_t new_size = m_size + other.m_size;
    if (!is_big()) {
        m_small_val = (m_small_val << other.m_size) | other.m_small_val;
    } else {
        mpz_class this_mpz  = as_mpz();
        mpz_class other_mpz = other.as_mpz();

        mpz_class tmp;
        mpz_mul_2exp(tmp.get_mpz_t(), this_mpz.get_mpz_t(), other.m_size);
        mpz_ior(this_mpz.get_mpz_t(), tmp.get_mpz_t(), other_mpz.get_mpz_t());

        m_big_val = this_mpz;
    }

    m_size = new_size;
    adjust_bits();
}

void BVConst::zext(uint32_t size)
{
    if (m_size > size) {
        err("BVConst") << "zext(): invalid size (" << m_size << " > " << size
                       << ")" << std::endl;
        exit_fail();
    }

    if (size <= 64) {
        m_size = size;
        return;
    }

    if (is_big()) {
        m_size = size;
        return;
    }

    m_big_val = mpz_class(m_small_val);
    m_size    = size;
}

void BVConst::sext(uint32_t size)
{
    if (m_size > size) {
        err("BVConst") << "sext(): invalid size (" << m_size << " > " << size
                       << ")" << std::endl;
        exit_fail();
    }

    if (size == m_size)
        return;

    if (size <= 64) {
        m_small_val = _sext(m_small_val, m_size);
        m_size      = size;
        adjust_bits();
        return;
    }

    if (!is_big())
        m_big_val = mpz_class(m_small_val);

    if (mpz_tstbit(m_big_val.get_mpz_t(), m_size - 1) == 1) {
        for (uint32_t i = m_size; i < size; ++i)
            mpz_setbit(m_big_val.get_mpz_t(), i);
    }

    m_size = size;
    return;
}

void BVConst::extract(uint32_t high, uint32_t low)
{
    if (high < low) {
        err("BVConst") << "extract(): invalid high and low (" << high << ", "
                       << low << ")" << std::endl;
        exit_fail();
    }

    if (!is_big()) {
        m_size      = high - low + 1;
        m_small_val = (m_small_val >> low) & _bitmask(m_size);
    } else {
        m_size = high - low + 1;
        mpz_class tmp;

        for (uint32_t i = 0; i < m_size; ++i) {
            if (mpz_tstbit(m_big_val.get_mpz_t(), i + low) == 1)
                mpz_setbit(tmp.get_mpz_t(), i);
            else
                mpz_clrbit(tmp.get_mpz_t(), i);
        }

        if (m_size <= 64) {
            m_small_val = tmp.get_ui();
        } else {
            m_big_val = tmp;
        }
    }
}

std::ostream& operator<<(std::ostream& os, const BVConst& c)
{
    return os << c.to_string();
}

static std::string mpz_to_string(const mpz_class& v, bool hex)
{
    static char        buf[4096];
    static const char* dec_fmt = "%Zd";
    static const char* hex_fmt = "0x%Zx";
    const char*        fmt     = hex ? hex_fmt : dec_fmt;

    size_t n_written =
        (size_t)gmp_snprintf(buf, sizeof(buf), fmt, v.get_mpz_t());

    if (n_written > sizeof(buf)) {
        err("BVConst") << "unable to print BVConst: the number is too long"
                       << std::endl;
        exit_fail();
    }
    return std::string(buf);
}

std::string BVConst::to_string(bool hex) const
{
    if (!is_big()) {
        std::stringstream ss;
        if (hex)
            ss << "0x" << std::hex << m_small_val;
        else
            ss << std::dec << m_small_val;
        return ss.str();
    }

    return mpz_to_string(m_big_val, hex);
}

} // namespace naaz::expr
