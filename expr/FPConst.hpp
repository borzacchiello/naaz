#pragma once

#include "../third_party/sleigh/csleigh.h"

namespace naaz::expr
{

class FPConst
{
    uint64_t       m_val;
    FloatFormatPtr m_ff;

  public:
    FPConst(FloatFormatPtr ff, uint64_t v) : m_ff(ff), m_val(v) {}
    FPConst(FloatFormatPtr ff, double v) : m_ff(ff)
    {
        m_val = ff->getEncoding(v);
    }

    uint64_t hash() const { return (uint64_t)m_ff.get() ^ m_val; }
    uint32_t size() const { return m_ff->getSize() * 8; }

    FloatFormatPtr ff() const { return m_ff; }
    uint64_t       val() const { return m_val; }
    double         as_double() const
    {
        static auto fc = FloatFormat::floatclass::normalized;
        return m_ff->getHostFloat(m_val, &fc);
    }

    void convert(FloatFormatPtr ff);
    void add(const FPConst& other);
    bool is_nan() const;

    bool lt(const FPConst& other) const;
    bool eq(const FPConst& other) const;
};

} // namespace naaz::expr