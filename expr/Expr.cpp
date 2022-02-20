#include "Expr.hpp"

#include "../util/ioutil.hpp"
#include "../third_party/xxHash/xxh3.h"

namespace naaz::expr
{

uint64_t SymExpr::hash() const
{
    XXH64_state_t state;
    XXH64_reset(&state, 0);
    XXH64_update(&state, m_name.data(), m_name.size());
    XXH64_update(&state, &m_size, sizeof(m_size));
    return XXH64_digest(&state);
}

bool SymExpr::eq(ExprPtr other) const
{
    if (other->kind() != ekind)
        return false;

    std::shared_ptr<const SymExpr> other_ =
        std::static_pointer_cast<const SymExpr>(other);
    return m_name.compare(other_->m_name) == 0;
}

void SymExpr::pp() const { pp_stream() << m_name; }

uint64_t ConstExpr::hash() const
{
    XXH64_state_t state;
    XXH64_reset(&state, 0);
    XXH64_update(&state, &m_val, sizeof(m_val));
    XXH64_update(&state, &m_size, sizeof(m_size));
    return XXH64_digest(&state);
}

bool ConstExpr::eq(ExprPtr other) const
{
    if (other->kind() != ekind)
        return false;

    std::shared_ptr<const ConstExpr> other_ =
        std::static_pointer_cast<const ConstExpr>(other);
    return m_size == other_->m_size && m_val == other_->m_val;
}

void ConstExpr::pp() const
{
    // FIXME: print the number correctly
    pp_stream() << (uint64_t)m_val;
}

uint64_t ExtractExpr::hash() const
{
    XXH64_state_t state;
    XXH64_reset(&state, 0);
    XXH64_update(&state, &m_high, sizeof(m_high));
    XXH64_update(&state, &m_low, sizeof(m_low));
    void* raw_expr = (void*)m_expr.get();
    XXH64_update(&state, raw_expr, sizeof(void*));
    return XXH64_digest(&state);
}

bool ExtractExpr::eq(ExprPtr other) const
{
    if (other->kind() != ekind)
        return false;

    auto other_ = std::static_pointer_cast<const ExtractExpr>(other);
    return m_high == other_->m_high && m_low == other_->m_low &&
           m_expr.get() == other_->m_expr.get();
}

void ExtractExpr::pp() const
{
    m_expr->pp();
    pp_stream() << "[" << m_high << ":" << m_low << "]";
}

uint64_t ConcatExpr::hash() const
{
    XXH64_state_t state;
    XXH64_reset(&state, 0);
    XXH64_update(&state, &m_size, sizeof(m_size));
    void* raw_lhs = (void*)m_lhs.get();
    void* raw_rhs = (void*)m_rhs.get();
    XXH64_update(&state, raw_lhs, sizeof(void*));
    XXH64_update(&state, raw_rhs, sizeof(void*));
    return XXH64_digest(&state);
}

bool ConcatExpr::eq(ExprPtr other) const
{
    if (other->kind() != ekind)
        return false;

    auto other_ = std::static_pointer_cast<const ConcatExpr>(other);
    return m_size == other_->m_size && m_lhs.get() == other_->m_lhs.get() &&
           m_rhs.get() == other_->m_rhs.get();
}

void ConcatExpr::pp() const
{
    m_lhs->pp();
    pp_stream() << " + ";
    m_rhs->pp();
}

uint64_t NegExpr::hash() const
{
    XXH64_state_t state;
    XXH64_reset(&state, 0);
    XXH64_update(&state, &m_size, sizeof(m_size));
    void* raw_expr = (void*)m_expr.get();
    XXH64_update(&state, raw_expr, sizeof(void*));
    return XXH64_digest(&state);
}

bool NegExpr::eq(ExprPtr other) const
{
    if (other->kind() != ekind)
        return false;

    auto other_ = std::static_pointer_cast<const NegExpr>(other);
    return m_size == other_->m_size && m_expr.get() == other_->m_expr.get();
}

void NegExpr::pp() const
{
    pp_stream() << "-";
    m_expr->pp();
}

uint64_t AddExpr::hash() const
{
    XXH64_state_t state;
    XXH64_reset(&state, 0);
    XXH64_update(&state, (void*)&m_size, sizeof(m_size));
    for (const auto& child : m_children) {
        void* raw_child = (void*)child.get();
        XXH64_update(&state, raw_child, sizeof(void*));
    }
    return XXH64_digest(&state);
}

bool AddExpr::eq(ExprPtr other) const
{
    if (other->kind() != ekind)
        return false;

    auto other_ = std::static_pointer_cast<const AddExpr>(other);
    if (m_size != other_->m_size)
        return false;
    if (m_children.size() != other_->m_children.size())
        return false;

    for (uint64_t i = 0; i < m_children.size(); ++i)
        if (m_children.at(i) != other_->m_children.at(i))
            return false;
    return true;
}

void AddExpr::pp() const
{
    m_children.at(0)->pp();
    for (uint64_t i = 1; i < m_children.size(); ++i) {
        pp_stream() << " + ";
        m_children.at(i)->pp();
    }
}

} // namespace naaz::expr
