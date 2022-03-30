#include "FPConst.hpp"
#include "../util/ioutil.hpp"

namespace naaz::expr
{

static void assert_type_or_fail(const FPConst& a, const FPConst& b)
{
    if (a.ff() != b.ff()) {
        err("FPConst") << "assert_type_or_fail(): inconsistent types"
                       << std::endl;
        exit_fail();
    }
}

void FPConst::convert(FloatFormatPtr ff)
{
    if (ff == m_ff)
        // nothing to do
        return;

    m_val = ff->convertEncoding(m_val, m_ff.get());
    m_ff  = ff;
}

void FPConst::neg() { m_val = m_ff->opNeg(m_val); }

void FPConst::add(const FPConst& other)
{
    assert_type_or_fail(*this, other);

    m_val = m_ff->opAdd(m_val, other.m_val);
}

void FPConst::mul(const FPConst& other)
{
    assert_type_or_fail(*this, other);

    m_val = m_ff->opMult(m_val, other.m_val);
}

void FPConst::div(const FPConst& other)
{
    assert_type_or_fail(*this, other);

    m_val = m_ff->opDiv(m_val, other.m_val);
}

bool FPConst::is_nan() const { return (bool)m_ff->opNan(m_val); }

bool FPConst::lt(const FPConst& other) const
{
    assert_type_or_fail(*this, other);

    return (bool)m_ff->opLess(m_val, other.m_val);
}

bool FPConst::eq(const FPConst& other) const
{
    assert_type_or_fail(*this, other);

    return (bool)m_ff->opEqual(m_val, other.m_val);
}

} // namespace naaz::expr
