#include <sstream>

#include "Expr.hpp"
#include "ExprBuilder.hpp"

#include "../util/ioutil.hpp"
#include "../third_party/xxHash/xxh3.h"

namespace naaz::expr
{

// **********
// * SymExpr
// **********

uint64_t SymExpr::hash() const
{
    XXH64_state_t state;
    XXH64_reset(&state, 0);
    XXH64_update(&state, &m_id, sizeof(m_id));
    XXH64_update(&state, &m_size, sizeof(m_size));
    return XXH64_digest(&state);
}

bool SymExpr::eq(ExprPtr other) const
{
    if (other->kind() != ekind)
        return false;

    std::shared_ptr<const SymExpr> other_ =
        std::static_pointer_cast<const SymExpr>(other);
    return m_id == other_->m_id;
}

const std::string& SymExpr::name() const
{
    return ExprBuilder::The().get_sym_name(m_id);
}

// ***************
// * ConstExpr
// ***************

ConstExpr::ConstExpr(uint64_t val, size_t size) : m_val(val, size)
{
    m_hash = m_val.hash();
}

ConstExpr::ConstExpr(const BVConst& val) : m_val(val) { m_hash = m_val.hash(); }

uint64_t ConstExpr::hash() const { return m_hash; }

bool ConstExpr::eq(ExprPtr other) const
{
    if (other->kind() != ekind)
        return false;

    std::shared_ptr<const ConstExpr> other_ =
        std::static_pointer_cast<const ConstExpr>(other);
    if (size() != other_->size())
        return false;
    return m_val.eq(other_->m_val);
}

// ***************
// * ITEExpr
// ***************

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

// ***************
// * ExtractExpr
// ***************

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
           m_expr == other_->m_expr;
}

// ***************
// * ConcatExpr
// ***************

uint64_t ConcatExpr::hash() const
{
    XXH64_state_t state;
    XXH64_reset(&state, 0);
    XXH64_update(&state, &m_size, sizeof(m_size));
    for (const auto& child : m_children) {
        void* raw_child = (void*)child.get();
        XXH64_update(&state, raw_child, sizeof(void*));
    }
    return XXH64_digest(&state);
}

bool ConcatExpr::eq(ExprPtr other) const
{
    if (other->kind() != ekind)
        return false;

    auto other_ = std::static_pointer_cast<const ConcatExpr>(other);
    if (m_size != other_->m_size)
        return false;
    if (m_children.size() != other_->m_children.size())
        return false;

    for (uint64_t i = 0; i < m_children.size(); ++i)
        if (m_children.at(i) != other_->m_children.at(i))
            return false;
    return true;
}

// ***************
// * ZextExpr
// ***************

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
    return m_size == other_->m_size && m_expr == other_->m_expr;
}

// ***************
// * SextExpr
// ***************

SextExpr::SextExpr(BVExprPtr e, size_t s) : m_expr(e), m_size(s)
{
    if (s < e->size()) {
        err("SextExpr") << "invalid size" << std::endl;
        exit_fail();
    }
}

uint64_t SextExpr::hash() const
{
    XXH64_state_t state;
    XXH64_reset(&state, 0);
    XXH64_update(&state, &m_size, sizeof(m_size));
    void* raw_expr = (void*)m_expr.get();
    XXH64_update(&state, raw_expr, sizeof(void*));
    return XXH64_digest(&state);
}

bool SextExpr::eq(ExprPtr other) const
{
    if (other->kind() != ekind)
        return false;

    auto other_ = std::static_pointer_cast<const SextExpr>(other);
    return m_size == other_->m_size && m_expr == other_->m_expr;
}

// ***************
// * NegExpr
// ***************

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
    return m_size == other_->m_size && m_expr == other_->m_expr;
}

// ***************
// * NotExpr
// ***************

uint64_t NotExpr::hash() const
{
    XXH64_state_t state;
    XXH64_reset(&state, 0);
    XXH64_update(&state, &m_size, sizeof(m_size));
    void* raw_expr = (void*)m_expr.get();
    XXH64_update(&state, raw_expr, sizeof(void*));
    return XXH64_digest(&state);
}

bool NotExpr::eq(ExprPtr other) const
{
    if (other->kind() != ekind)
        return false;

    auto other_ = std::static_pointer_cast<const NotExpr>(other);
    return m_size == other_->m_size && m_expr == other_->m_expr;
}

// ***************
// * ShlExpr
// ***************

uint64_t ShlExpr::hash() const
{
    XXH64_state_t state;
    XXH64_reset(&state, 0);
    XXH64_update(&state, &m_size, sizeof(m_size));
    void* raw_expr = (void*)m_expr.get();
    void* raw_val  = (void*)m_val.get();
    XXH64_update(&state, raw_expr, sizeof(void*));
    XXH64_update(&state, raw_val, sizeof(void*));
    return XXH64_digest(&state);
}

bool ShlExpr::eq(ExprPtr other) const
{
    if (other->kind() != ekind)
        return false;

    auto other_ = std::static_pointer_cast<const ShlExpr>(other);
    return m_size == other_->m_size && m_expr == other_->m_expr &&
           m_val == other_->m_val;
}

// ***************
// * LShrExpr
// ***************

uint64_t LShrExpr::hash() const
{
    XXH64_state_t state;
    XXH64_reset(&state, 0);
    XXH64_update(&state, &m_size, sizeof(m_size));
    void* raw_expr = (void*)m_expr.get();
    void* raw_val  = (void*)m_val.get();
    XXH64_update(&state, raw_expr, sizeof(void*));
    XXH64_update(&state, raw_val, sizeof(void*));
    return XXH64_digest(&state);
}

bool LShrExpr::eq(ExprPtr other) const
{
    if (other->kind() != ekind)
        return false;

    auto other_ = std::static_pointer_cast<const LShrExpr>(other);
    return m_size == other_->m_size && m_expr == other_->m_expr &&
           m_val == other_->m_val;
}

// ***************
// * AShrExpr
// ***************

uint64_t AShrExpr::hash() const
{
    XXH64_state_t state;
    XXH64_reset(&state, 0);
    XXH64_update(&state, &m_size, sizeof(m_size));
    void* raw_expr = (void*)m_expr.get();
    void* raw_val  = (void*)m_val.get();
    XXH64_update(&state, raw_expr, sizeof(void*));
    XXH64_update(&state, raw_val, sizeof(void*));
    return XXH64_digest(&state);
}

bool AShrExpr::eq(ExprPtr other) const
{
    if (other->kind() != ekind)
        return false;

    auto other_ = std::static_pointer_cast<const AShrExpr>(other);
    return m_size == other_->m_size && m_expr == other_->m_expr &&
           m_val == other_->m_val;
}

// ***************
// * AddExpr
// ***************

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

// ***************
// * MulExpr
// ***************

uint64_t MulExpr::hash() const
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

bool MulExpr::eq(ExprPtr other) const
{
    if (other->kind() != ekind)
        return false;

    auto other_ = std::static_pointer_cast<const MulExpr>(other);
    if (m_size != other_->m_size)
        return false;
    if (m_children.size() != other_->m_children.size())
        return false;

    for (uint64_t i = 0; i < m_children.size(); ++i)
        if (m_children.at(i) != other_->m_children.at(i))
            return false;
    return true;
}

// ***************
// * SDivExpr
// ***************

uint64_t SDivExpr::hash() const
{
    XXH64_state_t state;
    XXH64_reset(&state, 0);
    XXH64_update(&state, (void*)&m_size, sizeof(m_size));
    void* lhs_raw = (void*)m_lhs.get();
    XXH64_update(&state, lhs_raw, sizeof(void*));
    void* rhs_raw = (void*)m_rhs.get();
    XXH64_update(&state, rhs_raw, sizeof(void*));
    return XXH64_digest(&state);
}

bool SDivExpr::eq(ExprPtr other) const
{
    if (other->kind() != ekind)
        return false;

    auto other_ = std::static_pointer_cast<const SDivExpr>(other);
    if (m_size != other_->m_size)
        return false;
    if (m_lhs != other_->m_lhs || m_rhs != other_->m_rhs)
        return false;
    return true;
}

// ***************
// * UDivExpr
// ***************

uint64_t UDivExpr::hash() const
{
    XXH64_state_t state;
    XXH64_reset(&state, 0);
    XXH64_update(&state, (void*)&m_size, sizeof(m_size));
    void* lhs_raw = (void*)m_lhs.get();
    XXH64_update(&state, lhs_raw, sizeof(void*));
    void* rhs_raw = (void*)m_rhs.get();
    XXH64_update(&state, rhs_raw, sizeof(void*));
    return XXH64_digest(&state);
}

bool UDivExpr::eq(ExprPtr other) const
{
    if (other->kind() != ekind)
        return false;

    auto other_ = std::static_pointer_cast<const UDivExpr>(other);
    if (m_size != other_->m_size)
        return false;
    if (m_lhs != other_->m_lhs || m_rhs != other_->m_rhs)
        return false;
    return true;
}

// ***************
// * SRemExpr
// ***************

uint64_t SRemExpr::hash() const
{
    XXH64_state_t state;
    XXH64_reset(&state, 0);
    XXH64_update(&state, (void*)&m_size, sizeof(m_size));
    void* lhs_raw = (void*)m_lhs.get();
    XXH64_update(&state, lhs_raw, sizeof(void*));
    void* rhs_raw = (void*)m_rhs.get();
    XXH64_update(&state, rhs_raw, sizeof(void*));
    return XXH64_digest(&state);
}

bool SRemExpr::eq(ExprPtr other) const
{
    if (other->kind() != ekind)
        return false;

    auto other_ = std::static_pointer_cast<const SRemExpr>(other);
    if (m_size != other_->m_size)
        return false;
    if (m_lhs != other_->m_lhs || m_rhs != other_->m_rhs)
        return false;
    return true;
}

// ***************
// * URemExpr
// ***************

uint64_t URemExpr::hash() const
{
    XXH64_state_t state;
    XXH64_reset(&state, 0);
    XXH64_update(&state, (void*)&m_size, sizeof(m_size));
    void* lhs_raw = (void*)m_lhs.get();
    XXH64_update(&state, lhs_raw, sizeof(void*));
    void* rhs_raw = (void*)m_rhs.get();
    XXH64_update(&state, rhs_raw, sizeof(void*));
    return XXH64_digest(&state);
}

bool URemExpr::eq(ExprPtr other) const
{
    if (other->kind() != ekind)
        return false;

    auto other_ = std::static_pointer_cast<const URemExpr>(other);
    if (m_size != other_->m_size)
        return false;
    if (m_lhs != other_->m_lhs || m_rhs != other_->m_rhs)
        return false;
    return true;
}

// ***************
// * AndExpr
// ***************

uint64_t AndExpr::hash() const
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

bool AndExpr::eq(ExprPtr other) const
{
    if (other->kind() != ekind)
        return false;

    auto other_ = std::static_pointer_cast<const AndExpr>(other);
    if (m_size != other_->m_size)
        return false;
    if (m_children.size() != other_->m_children.size())
        return false;

    for (uint64_t i = 0; i < m_children.size(); ++i)
        if (m_children.at(i) != other_->m_children.at(i))
            return false;
    return true;
}

// ***************
// * OrExpr
// ***************

uint64_t OrExpr::hash() const
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

bool OrExpr::eq(ExprPtr other) const
{
    if (other->kind() != ekind)
        return false;

    auto other_ = std::static_pointer_cast<const OrExpr>(other);
    if (m_size != other_->m_size)
        return false;
    if (m_children.size() != other_->m_children.size())
        return false;

    for (uint64_t i = 0; i < m_children.size(); ++i)
        if (m_children.at(i) != other_->m_children.at(i))
            return false;
    return true;
}

// ***************
// * XorExpr
// ***************

uint64_t XorExpr::hash() const
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

bool XorExpr::eq(ExprPtr other) const
{
    if (other->kind() != ekind)
        return false;

    auto other_ = std::static_pointer_cast<const XorExpr>(other);
    if (m_size != other_->m_size)
        return false;
    if (m_children.size() != other_->m_children.size())
        return false;

    for (uint64_t i = 0; i < m_children.size(); ++i)
        if (m_children.at(i) != other_->m_children.at(i))
            return false;
    return true;
}

// ***************
// * BoolConst
// ***************

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

// ***************
// * BoolNotExpr
// ***************

uint64_t BoolNotExpr::hash() const
{
    XXH64_state_t state;
    XXH64_reset(&state, 0);
    void* raw_expr = (void*)m_expr.get();
    XXH64_update(&state, raw_expr, sizeof(void*));
    return XXH64_digest(&state);
}

bool BoolNotExpr::eq(ExprPtr other) const
{
    if (other->kind() != ekind)
        return false;

    auto other_ = std::static_pointer_cast<const BoolNotExpr>(other);
    return m_expr == other_->m_expr;
}

// ***************
// * BoolAndExpr
// ***************

uint64_t BoolAndExpr::hash() const
{
    XXH64_state_t state;
    XXH64_reset(&state, 0);
    for (const auto& child : m_exprs) {
        void* raw_child = (void*)child.get();
        XXH64_update(&state, raw_child, sizeof(void*));
    }
    return XXH64_digest(&state);
}

bool BoolAndExpr::eq(ExprPtr other) const
{
    if (other->kind() != ekind)
        return false;

    auto other_ = std::static_pointer_cast<const BoolAndExpr>(other);
    if (m_exprs.size() != other_->m_exprs.size())
        return false;

    for (uint64_t i = 0; i < m_exprs.size(); ++i)
        if (m_exprs.at(i) != other_->m_exprs.at(i))
            return false;
    return true;
}

// ***************
// * BoolOrExpr
// ***************

uint64_t BoolOrExpr::hash() const
{
    XXH64_state_t state;
    XXH64_reset(&state, 0);
    for (const auto& child : m_exprs) {
        void* raw_child = (void*)child.get();
        XXH64_update(&state, raw_child, sizeof(void*));
    }
    return XXH64_digest(&state);
}

bool BoolOrExpr::eq(ExprPtr other) const
{
    if (other->kind() != ekind)
        return false;

    auto other_ = std::static_pointer_cast<const BoolOrExpr>(other);
    if (m_exprs.size() != other_->m_exprs.size())
        return false;

    for (uint64_t i = 0; i < m_exprs.size(); ++i)
        if (m_exprs.at(i) != other_->m_exprs.at(i))
            return false;
    return true;
}

// ***************
// * LogicalExprs
// ***************

#define GEN_BINARY_LOGICAL_EXPR_IMPL(NAME)                                     \
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
    }

GEN_BINARY_LOGICAL_EXPR_IMPL(UltExpr)
GEN_BINARY_LOGICAL_EXPR_IMPL(UleExpr)
GEN_BINARY_LOGICAL_EXPR_IMPL(UgtExpr)
GEN_BINARY_LOGICAL_EXPR_IMPL(UgeExpr)
GEN_BINARY_LOGICAL_EXPR_IMPL(SltExpr)
GEN_BINARY_LOGICAL_EXPR_IMPL(SleExpr)
GEN_BINARY_LOGICAL_EXPR_IMPL(SgtExpr)
GEN_BINARY_LOGICAL_EXPR_IMPL(SgeExpr)
GEN_BINARY_LOGICAL_EXPR_IMPL(EqExpr)

// ***************
// * FPConstExpr
// ***************

uint64_t FPConstExpr::hash() const { return m_val.hash(); }

bool FPConstExpr::eq(ExprPtr other) const
{
    if (other->kind() != ekind)
        return false;

    auto other_ = std::static_pointer_cast<const FPConstExpr>(other);
    if (m_ff != other_->m_ff)
        return false;

    return m_val.eq(other_->m_val);
}

// ***************
// * BVToFPExpr
// ***************

uint64_t BVToFPExpr::hash() const
{
    XXH64_state_t state;
    XXH64_reset(&state, 0);
    XXH64_update(&state, (void*)m_ff.get(), sizeof(void*));
    void* raw_expr = (void*)m_expr.get();
    XXH64_update(&state, raw_expr, sizeof(void*));
    return XXH64_digest(&state);
}

bool BVToFPExpr::eq(ExprPtr other) const
{
    if (other->kind() != ekind)
        return false;

    auto other_ = std::static_pointer_cast<const BVToFPExpr>(other);
    return m_ff == other_->m_ff && m_expr == other_->m_expr;
}

// ***************
// * FPToBVExpr
// ***************

uint64_t FPToBVExpr::hash() const
{
    XXH64_state_t state;
    XXH64_reset(&state, 0);
    void* raw_expr = (void*)m_expr.get();
    XXH64_update(&state, raw_expr, sizeof(void*));
    return XXH64_digest(&state);
}

bool FPToBVExpr::eq(ExprPtr other) const
{
    if (other->kind() != ekind)
        return false;

    auto other_ = std::static_pointer_cast<const FPToBVExpr>(other);
    return size() == other_->size() && m_expr == other_->m_expr;
}

// ***************
// * FPConvert
// ***************

uint64_t FPConvert::hash() const
{
    XXH64_state_t state;
    XXH64_reset(&state, 0);
    void* raw_expr = (void*)m_expr.get();
    XXH64_update(&state, raw_expr, sizeof(void*));
    XXH64_update(&state, (void*)m_ff.get(), sizeof(void*));
    return XXH64_digest(&state);
}

bool FPConvert::eq(ExprPtr other) const
{
    if (other->kind() != ekind)
        return false;

    auto other_ = std::static_pointer_cast<const FPConvert>(other);
    return m_ff == other_->m_ff && m_expr == other_->m_expr;
}

// ***************
// * IntToFPExpr
// ***************

uint64_t IntToFPExpr::hash() const
{
    XXH64_state_t state;
    XXH64_reset(&state, 0);
    void* raw_expr = (void*)m_expr.get();
    XXH64_update(&state, raw_expr, sizeof(void*));
    XXH64_update(&state, (void*)m_ff.get(), sizeof(void*));
    return XXH64_digest(&state);
}

bool IntToFPExpr::eq(ExprPtr other) const
{
    if (other->kind() != ekind)
        return false;

    auto other_ = std::static_pointer_cast<const IntToFPExpr>(other);
    return m_ff == other_->m_ff && m_expr == other_->m_expr;
}

// ***************
// * FPIsNAN
// ***************

uint64_t FPIsNAN::hash() const
{
    XXH64_state_t state;
    XXH64_reset(&state, 0);
    void* raw_expr = (void*)m_expr.get();
    XXH64_update(&state, raw_expr, sizeof(void*));
    return XXH64_digest(&state);
}

bool FPIsNAN::eq(ExprPtr other) const
{
    if (other->kind() != ekind)
        return false;

    auto other_ = std::static_pointer_cast<const FPIsNAN>(other);
    return m_expr == other_->m_expr;
}

// ***************
// * FPNegExpr
// ***************

uint64_t FPNegExpr::hash() const
{
    XXH64_state_t state;
    XXH64_reset(&state, 0);
    void* raw_expr = (void*)m_expr.get();
    XXH64_update(&state, (void*)m_expr.get(), sizeof(void*));
    return XXH64_digest(&state);
}

bool FPNegExpr::eq(ExprPtr other) const
{
    if (other->kind() != ekind)
        return false;

    auto other_ = std::static_pointer_cast<const FPNegExpr>(other);
    return m_expr == other_->m_expr;
}

// ***************
// * FPAddExpr
// ***************

uint64_t FPAddExpr::hash() const
{
    XXH64_state_t state;
    XXH64_reset(&state, 0);
    for (const auto& child : m_children) {
        void* raw_child = (void*)child.get();
        XXH64_update(&state, raw_child, sizeof(void*));
    }
    return XXH64_digest(&state);
}

bool FPAddExpr::eq(ExprPtr other) const
{
    if (other->kind() != ekind)
        return false;

    auto other_ = std::static_pointer_cast<const FPAddExpr>(other);
    if (m_children.size() != other_->m_children.size())
        return false;

    for (uint64_t i = 0; i < m_children.size(); ++i)
        if (m_children.at(i) != other_->m_children.at(i))
            return false;
    return true;
}

// ***************
// * FPMulExpr
// ***************

uint64_t FPMulExpr::hash() const
{
    XXH64_state_t state;
    XXH64_reset(&state, 0);
    for (const auto& child : m_children) {
        void* raw_child = (void*)child.get();
        XXH64_update(&state, raw_child, sizeof(void*));
    }
    return XXH64_digest(&state);
}

bool FPMulExpr::eq(ExprPtr other) const
{
    if (other->kind() != ekind)
        return false;

    auto other_ = std::static_pointer_cast<const FPMulExpr>(other);
    if (m_children.size() != other_->m_children.size())
        return false;

    for (uint64_t i = 0; i < m_children.size(); ++i)
        if (m_children.at(i) != other_->m_children.at(i))
            return false;
    return true;
}

// ***************
// * FPDivExpr
// ***************

uint64_t FPDivExpr::hash() const
{
    XXH64_state_t state;
    XXH64_reset(&state, 0);
    XXH64_update(&state, (void*)m_lhs.get(), sizeof(void*));
    XXH64_update(&state, (void*)m_rhs.get(), sizeof(void*));
    XXH64_update(&state, (void*)m_ff.get(), sizeof(void*));
    return XXH64_digest(&state);
}

bool FPDivExpr::eq(ExprPtr other) const
{
    if (other->kind() != ekind)
        return false;

    auto other_ = std::static_pointer_cast<const FPDivExpr>(other);
    return m_lhs == other_->m_lhs && m_rhs == other_->m_rhs &&
           m_ff == other_->m_ff;
}

// ***************
// * FPLogicalExprs
// ***************

GEN_BINARY_LOGICAL_EXPR_IMPL(FPEqExpr)
GEN_BINARY_LOGICAL_EXPR_IMPL(FPLtExpr)

} // namespace naaz::expr
