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

std::string SymExpr::to_string() const
{
    return ExprBuilder::The().get_sym_name(m_id);
}

const std::string& SymExpr::name() const
{
    return ExprBuilder::The().get_sym_name(m_id);
}

z3::expr SymExpr::to_z3(z3::context& ctx) const
{
    return ctx.bv_const(ExprBuilder::The().get_sym_name(m_id).c_str(), m_size);
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

std::string ConstExpr::to_string() const { return m_val.to_string(true); }

z3::expr ConstExpr::to_z3(z3::context& ctx) const
{
    return ctx.bv_val(m_val.to_string().c_str(), size());
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

std::string ITEExpr::to_string() const
{
    return "ITE(" + m_guard->to_string() + ", " + m_iftrue->to_string() + ", " +
           m_iffalse->to_string() + ")";
}

z3::expr ITEExpr::to_z3(z3::context& ctx) const
{
    return z3::ite(m_guard->to_z3(ctx), m_iftrue->to_z3(ctx),
                   m_iffalse->to_z3(ctx));
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

std::string ExtractExpr::to_string() const
{
    return std::string("(") + m_expr->to_string() + ")[" +
           std::to_string(m_high) + ":" + std::to_string(m_low) + "]";
}

z3::expr ExtractExpr::to_z3(z3::context& ctx) const
{
    return m_expr->to_z3(ctx).extract(m_high, m_low);
}

// ***************
// * ConcatExpr
// ***************

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
    return m_size == other_->m_size && m_lhs == other_->m_lhs &&
           m_rhs == other_->m_rhs;
}

std::string ConcatExpr::to_string() const
{
    return m_lhs->to_string() + " # " + m_rhs->to_string();
}

z3::expr ConcatExpr::to_z3(z3::context& ctx) const
{
    return z3::concat(m_lhs->to_z3(ctx), m_rhs->to_z3(ctx));
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

std::string ZextExpr::to_string() const
{
    std::stringstream ss;
    ss << "zext(" << m_expr->to_string() << ", " << m_size << ")";
    return ss.str();
}

z3::expr ZextExpr::to_z3(z3::context& ctx) const
{
    return z3::zext(m_expr->to_z3(ctx), m_size - m_expr->size());
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

std::string SextExpr::to_string() const
{
    std::stringstream ss;
    ss << "sext(" << m_expr->to_string() << ", " << m_size << ")";
    return ss.str();
}

z3::expr SextExpr::to_z3(z3::context& ctx) const
{
    return z3::sext(m_expr->to_z3(ctx), m_size - m_expr->size());
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

std::string NegExpr::to_string() const
{
    return std::string("( - (") + m_expr->to_string() + "))";
}

z3::expr NegExpr::to_z3(z3::context& ctx) const { return -m_expr->to_z3(ctx); }

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

std::string ShlExpr::to_string() const
{
    return m_expr->to_string() + " << " + m_val->to_string();
}

z3::expr ShlExpr::to_z3(z3::context& ctx) const
{
    return z3::shl(m_expr->to_z3(ctx), m_val->to_z3(ctx));
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

std::string LShrExpr::to_string() const
{
    return m_expr->to_string() + " l>> " + m_val->to_string();
}

z3::expr LShrExpr::to_z3(z3::context& ctx) const
{
    return z3::lshr(m_expr->to_z3(ctx), m_val->to_z3(ctx));
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

std::string AShrExpr::to_string() const
{
    return m_expr->to_string() + " a>> " + m_val->to_string();
}

z3::expr AShrExpr::to_z3(z3::context& ctx) const
{
    return z3::ashr(m_expr->to_z3(ctx), m_val->to_z3(ctx));
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

std::string AddExpr::to_string() const
{
    std::string res = m_children.at(0)->to_string();
    for (uint64_t i = 1; i < m_children.size(); ++i) {
        res += " + ";
        res += m_children.at(i)->to_string();
    }
    return res;
}

z3::expr AddExpr::to_z3(z3::context& ctx) const
{
    z3::expr z3expr = m_children.at(0)->to_z3(ctx);
    for (uint64_t i = 1; i < m_children.size(); ++i)
        z3expr = z3expr + m_children.at(i)->to_z3(ctx);
    return z3expr;
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

std::string MulExpr::to_string() const
{
    std::string res = "(";
    res += m_children.at(0)->to_string();
    for (uint64_t i = 1; i < m_children.size(); ++i) {
        res += " * ";
        res += m_children.at(i)->to_string();
    }
    res += ")";
    return res;
}

z3::expr MulExpr::to_z3(z3::context& ctx) const
{
    z3::expr z3expr = m_children.at(0)->to_z3(ctx);
    for (uint64_t i = 1; i < m_children.size(); ++i)
        z3expr = z3expr * m_children.at(i)->to_z3(ctx);
    return z3expr;
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

std::string SDivExpr::to_string() const
{
    std::string res = "(";
    res += m_lhs->to_string();
    res += " s/ ";
    res += m_rhs->to_string();
    res += ")";
    return res;
}

z3::expr SDivExpr::to_z3(z3::context& ctx) const
{
    z3::expr z3expr = m_lhs->to_z3(ctx) / m_rhs->to_z3(ctx);
    return z3expr;
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

std::string UDivExpr::to_string() const
{
    std::string res = "(";
    res += m_lhs->to_string();
    res += " u/ ";
    res += m_rhs->to_string();
    res += ")";
    return res;
}

z3::expr UDivExpr::to_z3(z3::context& ctx) const
{
    z3::expr z3expr = z3::udiv(m_lhs->to_z3(ctx), m_rhs->to_z3(ctx));
    return z3expr;
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

std::string SRemExpr::to_string() const
{
    std::string res = "(";
    res += m_lhs->to_string();
    res += " s% ";
    res += m_rhs->to_string();
    res += ")";
    return res;
}

z3::expr SRemExpr::to_z3(z3::context& ctx) const
{
    z3::expr z3expr = m_lhs->to_z3(ctx) % m_rhs->to_z3(ctx);
    return z3expr;
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

std::string URemExpr::to_string() const
{
    std::string res = "(";
    res += m_lhs->to_string();
    res += " u% ";
    res += m_rhs->to_string();
    res += ")";
    return res;
}

z3::expr URemExpr::to_z3(z3::context& ctx) const
{
    z3::expr z3expr = z3::urem(m_lhs->to_z3(ctx), m_rhs->to_z3(ctx));
    return z3expr;
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

std::string AndExpr::to_string() const
{
    std::string res = m_children.at(0)->to_string();
    for (uint64_t i = 1; i < m_children.size(); ++i) {
        res += " & ";
        res += m_children.at(i)->to_string();
    }
    return res;
}

z3::expr AndExpr::to_z3(z3::context& ctx) const
{
    z3::expr z3expr = m_children.at(0)->to_z3(ctx);
    for (uint64_t i = 1; i < m_children.size(); ++i)
        z3expr = z3expr & m_children.at(i)->to_z3(ctx);
    return z3expr;
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

std::string OrExpr::to_string() const
{
    std::string res = m_children.at(0)->to_string();
    for (uint64_t i = 1; i < m_children.size(); ++i) {
        res += " | ";
        res += m_children.at(i)->to_string();
    }
    return res;
}

z3::expr OrExpr::to_z3(z3::context& ctx) const
{
    z3::expr z3expr = m_children.at(0)->to_z3(ctx);
    for (uint64_t i = 1; i < m_children.size(); ++i)
        z3expr = z3expr | m_children.at(i)->to_z3(ctx);
    return z3expr;
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

std::string XorExpr::to_string() const
{
    std::string res = "( ";
    res +=  m_children.at(0)->to_string();
    for (uint64_t i = 1; i < m_children.size(); ++i) {
        res += " ^ ";
        res += m_children.at(i)->to_string();
    }
    res += ")";
    return res;
}

z3::expr XorExpr::to_z3(z3::context& ctx) const
{
    z3::expr z3expr = m_children.at(0)->to_z3(ctx);
    for (uint64_t i = 1; i < m_children.size(); ++i)
        z3expr = z3expr ^ m_children.at(i)->to_z3(ctx);
    return z3expr;
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

std::string BoolConst::to_string() const
{
    if (m_is_true)
        return "true";
    return "false";
}

z3::expr BoolConst::to_z3(z3::context& ctx) const
{
    if (m_is_true)
        return ctx.bool_val(true);
    return ctx.bool_val(false);
}

// ***************
// * NotExpr
// ***************

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

z3::expr NotExpr::to_z3(z3::context& ctx) const { return !m_expr->to_z3(ctx); }

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

std::string BoolAndExpr::to_string() const
{
    std::string res = "( ";
    res += m_exprs.at(0)->to_string();
    for (uint64_t i = 1; i < m_exprs.size(); ++i) {
        res += " && ";
        res += m_exprs.at(i)->to_string();
    }
    res += " )";
    return res;
}

z3::expr BoolAndExpr::to_z3(z3::context& ctx) const
{
    // I'm assuming that there is always at least one element in BoolAndExpr.
    // There should be at least 2 elements in m_exprs, this must be enforced by
    // ExprBuilder
    z3::expr e(m_exprs.at(0)->to_z3(ctx));

    for (uint64_t i = 1; i < m_exprs.size(); ++i) {
        e = e && m_exprs.at(i)->to_z3(ctx);
    }
    return e;
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

std::string BoolOrExpr::to_string() const
{
    std::string res = "(";
    res += m_exprs.at(0)->to_string();
    for (uint64_t i = 1; i < m_exprs.size(); ++i) {
        res += " || ";
        res += m_exprs.at(i)->to_string();
    }
    res += ")";
    return res;
}

z3::expr BoolOrExpr::to_z3(z3::context& ctx) const
{
    // I'm assuming that there is always at least one element in BoolOrExpr.
    // There should be at least 2 elements in m_exprs, this must be enforced by
    // ExprBuilder
    z3::expr e(m_exprs.at(0)->to_z3(ctx));

    for (uint64_t i = 1; i < m_exprs.size(); ++i) {
        e = e || m_exprs.at(i)->to_z3(ctx);
    }
    return e;
}

// ***************
// * LogicalExprs
// ***************

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
z3::expr UltExpr::to_z3(z3::context& ctx) const
{
    return z3::ult(m_lhs->to_z3(ctx), m_rhs->to_z3(ctx));
}

GEN_BINARY_LOGICAL_EXPR_IMPL(UleExpr, " u<= ")
z3::expr UleExpr::to_z3(z3::context& ctx) const
{
    return z3::ule(m_lhs->to_z3(ctx), m_rhs->to_z3(ctx));
}

GEN_BINARY_LOGICAL_EXPR_IMPL(UgtExpr, " u> ")
z3::expr UgtExpr::to_z3(z3::context& ctx) const
{
    return z3::ugt(m_lhs->to_z3(ctx), m_rhs->to_z3(ctx));
}

GEN_BINARY_LOGICAL_EXPR_IMPL(UgeExpr, " u>= ")
z3::expr UgeExpr::to_z3(z3::context& ctx) const
{
    return z3::uge(m_lhs->to_z3(ctx), m_rhs->to_z3(ctx));
}

GEN_BINARY_LOGICAL_EXPR_IMPL(SltExpr, " s< ")
z3::expr SltExpr::to_z3(z3::context& ctx) const
{
    return z3::slt(m_lhs->to_z3(ctx), m_rhs->to_z3(ctx));
}

GEN_BINARY_LOGICAL_EXPR_IMPL(SleExpr, " s<= ")
z3::expr SleExpr::to_z3(z3::context& ctx) const
{
    return z3::sle(m_lhs->to_z3(ctx), m_rhs->to_z3(ctx));
}

GEN_BINARY_LOGICAL_EXPR_IMPL(SgtExpr, " s> ")
z3::expr SgtExpr::to_z3(z3::context& ctx) const
{
    return z3::sgt(m_lhs->to_z3(ctx), m_rhs->to_z3(ctx));
}

GEN_BINARY_LOGICAL_EXPR_IMPL(SgeExpr, " s>= ")
z3::expr SgeExpr::to_z3(z3::context& ctx) const
{
    return z3::sge(m_lhs->to_z3(ctx), m_rhs->to_z3(ctx));
}

GEN_BINARY_LOGICAL_EXPR_IMPL(EqExpr, " == ")
z3::expr EqExpr::to_z3(z3::context& ctx) const
{
    return m_lhs->to_z3(ctx) == m_rhs->to_z3(ctx);
}

} // namespace naaz::expr
