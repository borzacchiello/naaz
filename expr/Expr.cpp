#include "Expr.hpp"

#include "../util/ioutil.hpp"
#include "../third_party/xxHash/xxh3.h"

namespace naaz::expr
{

SymExpr::SymExpr(const std::string& name, size_t bits)
    : m_name(name), m_size(bits)
{
    if (m_size == 0) {
        err("SymExpr") << "invalid size (zero)" << std::endl;
        exit_fail();
    }
}

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

std::string SymExpr::to_string() const { return m_name; }

ConstExpr::ConstExpr(__uint128_t val, size_t size) : m_val(val), m_size(size)
{
    if (m_size == 0) {
        err("ConstExpr") << "invalid size (zero)" << std::endl;
        exit_fail();
    }
}

__int128_t ConstExpr::sval() const
{
    __int128_t res = (__int128_t)m_val;
    if (res & ((__uint128_t)1 << (m_size - 1)))
        res |= (((__uint128_t)2 << (128 - m_size)) - (__uint128_t)1) << m_size;
    return res;
}

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

std::string ConstExpr::to_string() const
{
    // FIXME: print the number correctly
    return std::to_string((uint64_t)m_val);
}

uint64_t ITEExpr::hash() const
{
    XXH64_state_t state;
    XXH64_reset(&state, 0);
    XXH64_update(&state, &m_size, sizeof(m_size));
    void* raw_guard   = (void*)m_guard.get();
    void* raw_iftrue  = (void*)m_iftrue.get();
    void* raw_iffalse = (void*)m_iffalse.get();
    XXH64_update(&state, raw_guard, sizeof(void*));
    XXH64_update(&state, raw_iftrue, sizeof(void*));
    XXH64_update(&state, raw_iffalse, sizeof(void*));
    return XXH64_digest(&state);
}

bool ITEExpr::eq(ExprPtr other) const
{
    if (other->kind() != ekind)
        return false;

    auto other_ = std::static_pointer_cast<const ITEExpr>(other);
    return m_size == other_->m_size && m_guard == other_->m_guard &&
           m_iftrue == other_->m_iftrue && m_iffalse == other_->m_iffalse;
}

std::string ITEExpr::to_string() const
{
    return "ITE(" + m_guard->to_string() + ", " + m_iftrue->to_string() + ", " +
           m_iffalse->to_string() + ")";
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

std::string ExtractExpr::to_string() const
{
    return m_expr->to_string() + "[" + std::to_string(m_high) + ":" +
           std::to_string(m_low) + "]";
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

std::string ConcatExpr::to_string() const
{
    return m_lhs->to_string() + " # " + m_rhs->to_string();
}

ZextExpr::ZextExpr(BVExprPtr e, size_t s) : m_expr(e), m_size(s)
{
    if (s < e->size()) {
        err("ZextExpr") << "invalid size" << std::endl;
        exit_fail();
    }
}

uint64_t ZextExpr::hash() const
{
    XXH64_state_t state;
    XXH64_reset(&state, 0);
    XXH64_update(&state, &m_size, sizeof(m_size));
    void* raw_expr = (void*)m_expr.get();
    XXH64_update(&state, raw_expr, sizeof(void*));
    return XXH64_digest(&state);
}

bool ZextExpr::eq(ExprPtr other) const
{
    if (other->kind() != ekind)
        return false;

    auto other_ = std::static_pointer_cast<const ZextExpr>(other);
    return m_size == other_->m_size && m_expr.get() == other_->m_expr.get();
}

std::string ZextExpr::to_string() const { return m_expr->to_string(); }

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

std::string NegExpr::to_string() const
{
    return std::string("( - (") + m_expr->to_string() + "))";
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

std::string AddExpr::to_string() const
{
    std::string res = m_children.at(0)->to_string();
    for (uint64_t i = 1; i < m_children.size(); ++i) {
        res += " + ";
        res += m_children.at(i)->to_string();
    }
    return res;
}

uint64_t BoolConst::hash() const
{
    if (m_is_true)
        return 1;
    return 0;
}

bool BoolConst::eq(ExprPtr other) const
{
    if (other->kind() != ekind)
        return false;
    auto other_ = std::static_pointer_cast<const BoolConst>(other);
    return m_is_true == other_->m_is_true;
}

std::string BoolConst::to_string() const
{
    if (m_is_true)
        return "true";
    return "false";
}

uint64_t NotExpr::hash() const
{
    XXH64_state_t state;
    XXH64_reset(&state, 0);
    void* raw_expr = (void*)m_expr.get();
    XXH64_update(&state, raw_expr, sizeof(void*));
    return XXH64_digest(&state);
}

bool NotExpr::eq(ExprPtr other) const
{
    if (other->kind() != ekind)
        return false;

    auto other_ = std::static_pointer_cast<const NotExpr>(other);
    return m_expr == other_->m_expr;
}

std::string NotExpr::to_string() const
{
    return std::string("not (") + m_expr->to_string() + ")";
}

#define GEN_BINARY_LOGICAL_EXPR_IMPL(NAME, OP_STR)                             \
    uint64_t NAME::hash() const                                                \
    {                                                                          \
        XXH64_state_t state;                                                   \
        XXH64_reset(&state, 0);                                                \
        void* raw_lhs = (void*)m_lhs.get();                                    \
        XXH64_update(&state, raw_lhs, sizeof(void*));                          \
        void* raw_rhs = (void*)m_rhs.get();                                    \
        XXH64_update(&state, raw_rhs, sizeof(void*));                          \
        return XXH64_digest(&state);                                           \
    }                                                                          \
    bool NAME::eq(ExprPtr other) const                                         \
    {                                                                          \
        if (other->kind() != ekind)                                            \
            return false;                                                      \
        auto other_ = std::static_pointer_cast<const NAME>(other);             \
        return m_lhs == other_->m_lhs && m_rhs == other_->m_rhs;               \
    }                                                                          \
    std::string NAME::to_string() const                                        \
    {                                                                          \
        return m_lhs->to_string() + OP_STR + m_rhs->to_string();               \
    }

GEN_BINARY_LOGICAL_EXPR_IMPL(UltExpr, " u< ")
GEN_BINARY_LOGICAL_EXPR_IMPL(UleExpr, " u<= ")
GEN_BINARY_LOGICAL_EXPR_IMPL(UgtExpr, " u> ")
GEN_BINARY_LOGICAL_EXPR_IMPL(UgeExpr, " u>= ")
GEN_BINARY_LOGICAL_EXPR_IMPL(SltExpr, " s< ")
GEN_BINARY_LOGICAL_EXPR_IMPL(SleExpr, " s<= ")
GEN_BINARY_LOGICAL_EXPR_IMPL(SgtExpr, " s> ")
GEN_BINARY_LOGICAL_EXPR_IMPL(SgeExpr, " s>= ")
GEN_BINARY_LOGICAL_EXPR_IMPL(EqExpr, " == ")

} // namespace naaz::expr
