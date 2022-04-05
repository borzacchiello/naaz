#include "../util/ioutil.hpp"

#include "ExprBuilder.hpp"

#define min(x, y) ((x) < (y) ? (x) : (y))

namespace naaz::expr
{

ConstExprPtr ExprBuilder::const_cache(uint64_t val, size_t size)
{
    auto key = std::make_pair(val, size);
    if (m_consts.contains(key))
        return m_consts[key];

    if (m_consts.size() > 100000)
        // FIXME: 100k is a random value... Make some measurements
        m_consts.clear();

    auto c = std::static_pointer_cast<const ConstExpr>(
        get_or_create(ConstExpr(val, size)));
    m_consts[key] = c;
    return c;
}

ExprPtr ExprBuilder::get_or_create(const Expr& e)
{
    // Get a cached expression or create a new one
    uint64_t hash = e.hash();
    if (!m_exprs.contains(hash)) {
        ExprPtr r = e.clone();

        std::vector<WeakExprPtr> bucket;
        bucket.push_back(r);
        m_exprs.emplace(hash, bucket);
        return r;
    }

    std::vector<WeakExprPtr>& bucket = m_exprs.at(hash);

    // remove invalid weakrefs
    bucket.erase(std::remove_if(bucket.begin(), bucket.end(),
                                [](WeakExprPtr we) { return we.expired(); }),
                 bucket.end());

    for (auto& r : m_exprs.at(hash)) {
        if (e.eq(r.lock()))
            return r.lock();
    }
    ExprPtr r = e.clone();
    bucket.push_back(r);
    return r;
}

void ExprBuilder::collect_garbage()
{
    for (auto& [hash, bucket] : m_exprs) {
        bucket.erase(
            std::remove_if(bucket.begin(), bucket.end(),
                           [](WeakExprPtr we) { return we.expired(); }),
            bucket.end());
    }
}

static inline void check_size_or_fail(const std::string& op_name, BVExprPtr lhs,
                                      BVExprPtr rhs)
{
    if (lhs->size() != rhs->size()) {
        err("ExprBuilder") << "different sizes in " << op_name << std::endl;
        exit_fail();
    }
}

static inline void check_fp_type_or_fail(const std::string& op_name,
                                         FPExprPtr lhs, FPExprPtr rhs)
{
    if (lhs->ff() != rhs->ff()) {
        err("ExprBuilder") << "incopatible floating point types in " << op_name
                           << std::endl;
        exit_fail();
    }
}

const std::string& ExprBuilder::get_sym_name(uint32_t id) const
{
    // It raises an exeption if the id does not name a symbol
    return m_sym_id_to_name.at(id);
}

uint32_t ExprBuilder::get_sym_id(const std::string& name) const
{
    // It raises an exeption if the name is not a symbol
    return m_symbols.at(name)->id();
}

BoolConstPtr ExprBuilder::mk_true() { return BoolConst::true_expr(); }

BoolConstPtr ExprBuilder::mk_false() { return BoolConst::false_expr(); }

SymExprPtr ExprBuilder::mk_sym(const std::string& name, size_t size)
{
    if (m_symbols.contains(name)) {
        auto sym = m_symbols[name];
        if (sym->size() != size) {
            err("ExprBuilder")
                << "mk_sym(): symbol \"" << name << "\" was created with size "
                << sym->size()
                << " and now you are trying to create a new one with size "
                << size << std::endl;
            exit_fail();
        }
        return sym;
    }

    uint32_t   sym_id = m_sym_ids++;
    SymExpr    e(sym_id, size);
    SymExprPtr res = std::static_pointer_cast<const SymExpr>(get_or_create(e));

    m_symbols[name]          = res;
    m_sym_id_to_name[sym_id] = name;
    return res;
}

ConstExprPtr ExprBuilder::mk_const(const BVConst& val)
{
    ConstExpr e(val);
    return std::static_pointer_cast<const ConstExpr>(get_or_create(e));
}

ConstExprPtr ExprBuilder::mk_const(uint64_t val, size_t size)
{
    return const_cache(val, size);
}

BVExprPtr ExprBuilder::mk_extract(BVExprPtr expr, uint32_t high, uint32_t low)
{
    if (high < low) {
        err("ExprBuilder") << "mk_extract(): high (" << high << ") < low ("
                           << low << ")" << std::endl;
        exit_fail();
    }

    // redundant extract
    if (low == 0 && high == expr->size() - 1) {
        return expr;
    }

    // constant propagation
    if (expr->kind() == Expr::Kind::CONST) {
        ConstExprPtr expr_ = std::static_pointer_cast<const ConstExpr>(expr);
        BVConst      tmp(expr_->val());
        tmp.extract(high, low);
        return mk_const(tmp);
    }

    // extract of extract
    if (expr->kind() == Expr::Kind::EXTRACT) {
        auto expr_ = std::static_pointer_cast<const ExtractExpr>(expr);

        uint32_t    new_low  = low + expr_->low();
        uint32_t    new_high = high + expr_->low();
        ExtractExpr e(expr_->expr(), new_high, new_low);
        return std::static_pointer_cast<const BVExpr>(get_or_create(e));
    }

    // extract of concat
    if (expr->kind() == Expr::Kind::CONCAT) {
        ConcatExprPtr expr_ = std::static_pointer_cast<const ConcatExpr>(expr);

        uint32_t off = expr_->size();
        for (auto e : expr_->els()) {
            off -= e->size();
            uint32_t concat_high = e->size() + off - 1;
            uint32_t concat_low  = off;

            if (concat_high >= high && low >= concat_low)
                return mk_extract(e, high - off, low - off);
        }
    }

    // extract of zext/sext
    if (expr->kind() == Expr::Kind::ZEXT) {
        ZextExprPtr expr_ = std::static_pointer_cast<const ZextExpr>(expr);
        if (low == 0 && high == expr_->expr()->size() - 1)
            return expr_->expr();
        if (low >= expr_->expr()->size())
            return mk_const(0UL, high - low + 1);
        return mk_zext(
            mk_extract(expr_->expr(), min(high, expr_->size() - 1), low),
            high - low + 1);
    } else if (expr->kind() == Expr::Kind::SEXT) {
        SextExprPtr expr_ = std::static_pointer_cast<const SextExpr>(expr);
        if (low == 0 && high == expr_->expr()->size() - 1)
            return expr_->expr();
        if (low >= expr_->expr()->size()) {
            auto sign = sign_bit(expr_->expr());
            if (sign->kind() == Expr::Kind::CONST) {
                auto sign_ = std::static_pointer_cast<const ConstExpr>(sign);
                if (sign_->val().is_zero())
                    return mk_const(0UL, high - low + 1);
                return mk_const(BVConst("-1", high - low + 1));
            }
        }
    }

    ExtractExpr e(expr, high, low);
    return std::static_pointer_cast<const BVExpr>(get_or_create(e));
}

BVExprPtr ExprBuilder::mk_zext(BVExprPtr e, uint32_t n)
{
    if (n < e->size()) {
        err("ExprBuilder") << "mk_zext(): invalid size" << std::endl;
        exit_fail();
    }

    // unnecessary zext
    if (n == e->size())
        return e;

    // zext of zext
    if (e->kind() == Expr::Kind::ZEXT) {
        auto     e_ = std::static_pointer_cast<const ZextExpr>(e);
        ZextExpr r(e_->expr(), n);
        return std::static_pointer_cast<const BVExpr>(get_or_create(r));
    }

    // constant propagation
    if (e->kind() == Expr::Kind::CONST) {
        auto    e_ = std::static_pointer_cast<const ConstExpr>(e);
        BVConst tmp(e_->val());
        tmp.zext(n);
        return mk_const(tmp);
    }

    ZextExpr r(e, n);
    return std::static_pointer_cast<const BVExpr>(get_or_create(r));
}

BVExprPtr ExprBuilder::mk_sext(BVExprPtr e, uint32_t n)
{
    if (n < e->size()) {
        err("ExprBuilder") << "mk_sext(): invalid size" << std::endl;
        exit_fail();
    }

    // unnecessary sext
    if (n == e->size())
        return e;

    // sext of sext
    if (e->kind() == Expr::Kind::SEXT) {
        auto     e_ = std::static_pointer_cast<const SextExpr>(e);
        SextExpr r(e_->expr(), n);
        return std::static_pointer_cast<const BVExpr>(get_or_create(r));
    }

    // constant propagation
    if (e->kind() == Expr::Kind::CONST) {
        auto    e_ = std::static_pointer_cast<const ConstExpr>(e);
        BVConst tmp(e_->val());
        tmp.sext(n);
        return mk_const(tmp);
    }

    SextExpr r(e, n);
    return std::static_pointer_cast<const BVExpr>(get_or_create(r));
}

BVExprPtr ExprBuilder::mk_ite(BoolExprPtr guard, BVExprPtr iftrue,
                              BVExprPtr iffalse)
{
    check_size_or_fail("ITE", iftrue, iffalse);

    // constant propagation
    if (guard->kind() == Expr::Kind::BOOL_CONST) {
        auto guard_ = std::static_pointer_cast<const BoolConst>(guard);
        return guard_->is_true() ? iftrue : iffalse;
    }

    ITEExpr e(guard, iftrue, iffalse);
    return std::static_pointer_cast<const BVExpr>(get_or_create(e));
}

BVExprPtr ExprBuilder::mk_concat(BVExprPtr left, BVExprPtr right)
{
    // pattern sext(EXPR)[high:EXPR.size] # EXPR ==> sext(EXPR)
    if (left->kind() == Expr::Kind::EXTRACT) {
        auto left_ = std::static_pointer_cast<const ExtractExpr>(left);
        if (left_->low() == right->size() &&
            left_->expr()->kind() == Expr::Kind::SEXT) {
            auto sext_expr =
                std::static_pointer_cast<const SextExpr>(left_->expr());
            if (sext_expr->expr() == right)
                return mk_sext(right, left->size() + right->size());
        }
    }

    // pattern sext(EXPR)[high:N] # sext(EXPR, N) ==> sext(EXPR)
    if (left->kind() == Expr::Kind::EXTRACT &&
        right->kind() == Expr::Kind::SEXT) {
        auto left_  = std::static_pointer_cast<const ExtractExpr>(left);
        auto right_ = std::static_pointer_cast<const SextExpr>(right);
        if (left_->low() == right_->size() &&
            left_->expr()->kind() == Expr::Kind::SEXT) {
            auto sext_expr =
                std::static_pointer_cast<const SextExpr>(left_->expr());
            if (sext_expr->expr() == right_->expr())
                return mk_sext(right_->expr(), left->size() + right->size());
        }
    }

    std::vector<BVExprPtr> exprs;

    // flatten args
    if (left->kind() == Expr::Kind::CONCAT) {
        auto lhs_ = std::static_pointer_cast<const ConcatExpr>(left);
        for (auto child : lhs_->els())
            exprs.push_back(child);
    } else {
        exprs.push_back(left);
    }

    if (right->kind() == Expr::Kind::CONCAT) {
        auto rhs_ = std::static_pointer_cast<const ConcatExpr>(right);
        for (auto child : rhs_->els())
            exprs.push_back(child);
    } else {
        exprs.push_back(right);
    }

    // constant propagation
    std::vector<BVExprPtr> const_prop_exprs;
    for (uint64_t i = 0; i < exprs.size(); ++i) {
        auto e = exprs.at(i);
        if (e->kind() == Expr::Kind::CONST) {
            auto e_   = std::static_pointer_cast<const ConstExpr>(e);
            auto conc = e_->val();

            uint64_t j;
            for (j = i + 1; j < exprs.size(); ++j) {
                auto next = exprs.at(j);
                if (next->kind() != Expr::Kind::CONST)
                    break;

                auto next_ = std::static_pointer_cast<const ConstExpr>(next);
                conc.concat(next_->val());
            }
            i = j - 1;
            const_prop_exprs.push_back(mk_const(conc));
        } else
            const_prop_exprs.push_back(e);
    }

    // concat of extract
    std::vector<BVExprPtr> children;
    for (uint64_t i = 0; i < const_prop_exprs.size(); ++i) {
        auto e = const_prop_exprs.at(i);
        if (e->kind() == Expr::Kind::EXTRACT) {
            auto e_ = std::static_pointer_cast<const ExtractExpr>(e);

            uint32_t high = e_->high();
            uint32_t low  = e_->low();

            uint64_t j;
            for (j = i + 1; j < exprs.size(); ++j) {
                auto next = exprs.at(j);
                if (next->kind() != Expr::Kind::EXTRACT)
                    break;

                auto next_ = std::static_pointer_cast<const ExtractExpr>(next);
                if (next_->expr() != e_->expr() || low != next_->high() + 1)
                    break;

                low = next_->low();
            }
            i = j - 1;
            children.push_back(mk_extract(e_->expr(), high, low));
        } else
            children.push_back(e);
    }

    if (children.size() == 1)
        return children.back();

    ConcatExpr e(children);
    return std::static_pointer_cast<const BVExpr>(get_or_create(e));
}

BVExprPtr ExprBuilder::mk_shl(BVExprPtr expr, BVExprPtr val)
{
    check_size_or_fail("shl", expr, val);

    // constant propagation
    if (expr->kind() == Expr::Kind::CONST && val->kind() == Expr::Kind::CONST) {
        auto expr_ = std::static_pointer_cast<const ConstExpr>(expr);
        auto val_  = std::static_pointer_cast<const ConstExpr>(val);
        if (!val_->val().fit_in_u64())
            return mk_const(0, expr->size());

        BVConst tmp(expr_->val());
        tmp.shl(val_->val().as_u64());
        return mk_const(tmp);
    }

    // special cases
    if (val->kind() == Expr::Kind::CONST) {
        auto val_ = std::static_pointer_cast<const ConstExpr>(val);
        if (val_->val().is_zero())
            return expr;
        if (val_->val().as_u64() >= expr->size())
            return mk_const(0, expr->size());
    }

    ShlExpr e(expr, val);
    return std::static_pointer_cast<const BVExpr>(get_or_create(e));
}

BVExprPtr ExprBuilder::mk_lshr(BVExprPtr expr, BVExprPtr val)
{
    check_size_or_fail("lshr", expr, val);

    // constant propagation
    if (expr->kind() == Expr::Kind::CONST && val->kind() == Expr::Kind::CONST) {
        auto expr_ = std::static_pointer_cast<const ConstExpr>(expr);
        auto val_  = std::static_pointer_cast<const ConstExpr>(val);
        if (!val_->val().fit_in_u64())
            return mk_const(0, expr->size());

        BVConst tmp(expr_->val());
        tmp.lshr(val_->val().as_u64());
        return mk_const(tmp);
    }

    // special cases
    if (val->kind() == Expr::Kind::CONST) {
        auto val_ = std::static_pointer_cast<const ConstExpr>(val);
        if (val_->val().is_zero())
            return expr;
        if (val_->val().as_u64() >= expr->size())
            return mk_const(0, expr->size());
    }

    LShrExpr e(expr, val);
    return std::static_pointer_cast<const BVExpr>(get_or_create(e));
}

BVExprPtr ExprBuilder::mk_ashr(BVExprPtr expr, BVExprPtr val)
{
    check_size_or_fail("ashr", expr, val);

    // constant propagation
    if (expr->kind() == Expr::Kind::CONST && val->kind() == Expr::Kind::CONST) {
        auto expr_ = std::static_pointer_cast<const ConstExpr>(expr);
        auto val_  = std::static_pointer_cast<const ConstExpr>(val);
        if (!val_->val().fit_in_u64()) {
            if (val_->val().get_bit(val->size() - 1) == 0)
                return mk_const(0, expr->size());
            return mk_const(BVConst("-1", expr->size()));
        }

        BVConst tmp(expr_->val());
        tmp.ashr(val_->val().as_u64());
        return mk_const(tmp);
    }

    // special cases
    if (val->kind() == Expr::Kind::CONST) {
        auto val_ = std::static_pointer_cast<const ConstExpr>(val);
        if (val_->val().is_zero())
            return expr;
        if (val_->val().as_u64() >= expr->size()) {
            auto sign_expr =
                mk_extract(expr, expr->size() - 1, expr->size() - 1);
            if (sign_expr->kind() == Expr::Kind::CONST) {
                auto sign_expr_ =
                    std::static_pointer_cast<const ConstExpr>(sign_expr);
                if (sign_expr_->val().is_zero())
                    return mk_const(0, expr->size());
                return mk_const(BVConst("-1", expr->size()));
            }
        }
    }

    AShrExpr e(expr, val);
    return std::static_pointer_cast<const BVExpr>(get_or_create(e));
}

BVExprPtr ExprBuilder::mk_neg(BVExprPtr expr)
{
    // constant propagation
    if (expr->kind() == Expr::Kind::CONST) {
        ConstExprPtr expr_ = std::static_pointer_cast<const ConstExpr>(expr);
        BVConst      tmp(expr_->val());
        tmp.neg();
        return mk_const(tmp);
    }

    NegExpr e(expr);
    return std::static_pointer_cast<const BVExpr>(get_or_create(e));
}

BVExprPtr ExprBuilder::mk_not(BVExprPtr expr)
{
    // constant propagation
    if (expr->kind() == Expr::Kind::CONST) {
        ConstExprPtr expr_ = std::static_pointer_cast<const ConstExpr>(expr);
        BVConst      tmp(expr_->val());
        tmp.bit_not();
        return mk_const(tmp);
    }

    NotExpr e(expr);
    return std::static_pointer_cast<const BVExpr>(get_or_create(e));
}

BVExprPtr ExprBuilder::mk_add(BVExprPtr lhs, BVExprPtr rhs)
{
    check_size_or_fail("add", lhs, rhs);

    std::vector<BVExprPtr> addends;

    // flatten args
    if (lhs->kind() == Expr::Kind::ADD) {
        auto lhs_ = std::static_pointer_cast<const AddExpr>(lhs);
        for (auto child : lhs_->addends())
            addends.push_back(child);
    } else {
        addends.push_back(lhs);
    }

    if (rhs->kind() == Expr::Kind::ADD) {
        auto rhs_ = std::static_pointer_cast<const AddExpr>(rhs);
        for (auto child : rhs_->addends())
            addends.push_back(child);
    } else {
        addends.push_back(rhs);
    }

    std::vector<BVExprPtr> children;

    // constant propagation
    BVConst concrete_val(0UL, addends.at(0)->size());
    for (auto addend : addends) {
        if (addend->kind() == Expr::Kind::CONST) {
            auto addend_ = std::static_pointer_cast<const ConstExpr>(addend);
            concrete_val.add(addend_->val());
        } else {
            children.push_back(addend);
        }
    }

    // remove 'add with negated'
    std::vector<BVExprPtr>   pruned_children;
    std::map<BVExprPtr, int> negated_exprs;
    std::map<BVExprPtr, int> normal_exprs;
    for (auto exp : children) {
        if (exp->kind() == Expr::Kind::NEG) {
            auto exp_ = std::static_pointer_cast<const NegExpr>(exp);
            if (normal_exprs.contains(exp_->expr())) {
                if (normal_exprs[exp_->expr()] == 1)
                    normal_exprs.erase(exp_->expr());
                else
                    normal_exprs[exp_->expr()] -= 1;
            } else if (!negated_exprs.contains(exp_->expr()))
                negated_exprs[exp_->expr()] = 1;
            else
                negated_exprs[exp_->expr()] += 1;
        } else {
            if (negated_exprs.contains(exp)) {
                if (negated_exprs[exp] == 1)
                    negated_exprs.erase(exp);
                else
                    negated_exprs[exp] -= 1;
            } else if (!normal_exprs.contains(exp))
                normal_exprs[exp] = 1;
            else
                normal_exprs[exp] += 1;
        }
    }
    for (auto pair : negated_exprs) {
        if (pair.second > 1)
            pruned_children.push_back(mk_mul(
                mk_neg(pair.first), mk_const(pair.second, pair.first->size())));
        else
            pruned_children.push_back(mk_neg(pair.first));
    }
    for (auto pair : normal_exprs)
        if (pair.second > 1)
            pruned_children.push_back(
                mk_mul(pair.first, mk_const(pair.second, pair.first->size())));
        else
            pruned_children.push_back(pair.first);

    // final checks
    if (pruned_children.size() == 0)
        return mk_const(concrete_val);

    if (!concrete_val.is_zero())
        pruned_children.push_back(mk_const(concrete_val));

    if (pruned_children.size() == 1)
        return pruned_children.back();

    // sort children by address (commutative! We are trying to reduce the
    // number of equivalent expressions)
    std::sort(pruned_children.begin(), pruned_children.end());

    AddExpr e(pruned_children);
    return std::static_pointer_cast<const BVExpr>(get_or_create(e));
}

BVExprPtr ExprBuilder::mk_mul(BVExprPtr lhs, BVExprPtr rhs)
{
    check_size_or_fail("mul", lhs, rhs);

    std::vector<BVExprPtr> els;

    // flatten args
    if (lhs->kind() == Expr::Kind::MUL) {
        auto lhs_ = std::static_pointer_cast<const MulExpr>(lhs);
        for (auto child : lhs_->els())
            els.push_back(child);
    } else {
        els.push_back(lhs);
    }

    if (rhs->kind() == Expr::Kind::MUL) {
        auto rhs_ = std::static_pointer_cast<const MulExpr>(rhs);
        for (auto child : rhs_->els())
            els.push_back(child);
    } else {
        els.push_back(rhs);
    }

    std::vector<BVExprPtr> children;

    // constant propagation
    BVConst concrete_val(1UL, els.at(0)->size());
    for (auto e : els) {
        if (e->kind() == Expr::Kind::CONST) {
            auto e_ = std::static_pointer_cast<const ConstExpr>(e);
            concrete_val.mul(e_->val());
        } else {
            children.push_back(e);
        }
    }

    // final checks
    if (children.size() == 0 || concrete_val.is_zero())
        return mk_const(concrete_val);

    if (!concrete_val.is_one())
        children.push_back(mk_const(concrete_val));

    if (children.size() == 1)
        return children.back();

    // sort children by address (commutative! We are trying to reduce the
    // number of equivalent expressions)
    std::sort(children.begin(), children.end());

    MulExpr e(children);
    return std::static_pointer_cast<const BVExpr>(get_or_create(e));
}

BVExprPtr ExprBuilder::mk_sdiv(BVExprPtr lhs, BVExprPtr rhs)
{
    check_size_or_fail("sdiv", lhs, rhs);

    // div zero
    if (rhs->kind() == Expr::Kind::CONST) {
        ConstExprPtr rhs_ = std::static_pointer_cast<const ConstExpr>(rhs);
        if (rhs_->val().is_zero()) {
            // we are consistent with Z3...
            auto zero = mk_const(0UL, lhs->size());
            auto min1 = mk_const(BVConst("-1", lhs->size()));
            return mk_ite(mk_sge(lhs, zero), min1, zero);
        }
    }

    // constant propagation
    if (lhs->kind() == Expr::Kind::CONST && rhs->kind() == Expr::Kind::CONST) {
        ConstExprPtr lhs_ = std::static_pointer_cast<const ConstExpr>(lhs);
        ConstExprPtr rhs_ = std::static_pointer_cast<const ConstExpr>(rhs);
        BVConst      concrete_val = lhs_->val();
        concrete_val.sdiv(rhs_->val());
        ConstExpr e(concrete_val);
        return std::static_pointer_cast<const BVExpr>(get_or_create(e));
    }

    // div with myself
    if (lhs == rhs)
        return mk_const(1, lhs->size());

    SDivExpr e(lhs, rhs);
    return std::static_pointer_cast<const BVExpr>(get_or_create(e));
}

BVExprPtr ExprBuilder::mk_udiv(BVExprPtr lhs, BVExprPtr rhs)
{
    check_size_or_fail("udiv", lhs, rhs);

    // div zero
    if (rhs->kind() == Expr::Kind::CONST) {
        ConstExprPtr rhs_ = std::static_pointer_cast<const ConstExpr>(rhs);
        if (rhs_->val().is_zero()) {
            // we are consistent with Z3...
            auto min1 = mk_const(BVConst("-1", lhs->size()));
            return min1;
        }
    }

    // constant propagation
    if (lhs->kind() == Expr::Kind::CONST && rhs->kind() == Expr::Kind::CONST) {
        ConstExprPtr lhs_ = std::static_pointer_cast<const ConstExpr>(lhs);
        ConstExprPtr rhs_ = std::static_pointer_cast<const ConstExpr>(rhs);
        BVConst      concrete_val = lhs_->val();
        concrete_val.udiv(rhs_->val());
        ConstExpr e(concrete_val);
        return std::static_pointer_cast<const BVExpr>(get_or_create(e));
    }

    // div with myself
    if (lhs == rhs)
        return mk_const(1, lhs->size());

    UDivExpr e(lhs, rhs);
    return std::static_pointer_cast<const BVExpr>(get_or_create(e));
}

BVExprPtr ExprBuilder::mk_srem(BVExprPtr lhs, BVExprPtr rhs)
{
    check_size_or_fail("srem", lhs, rhs);

    // rem zero
    if (rhs->kind() == Expr::Kind::CONST) {
        ConstExprPtr rhs_ = std::static_pointer_cast<const ConstExpr>(rhs);
        if (rhs_->val().is_zero()) {
            // we are consistent with Z3...
            return lhs;
        }
    }

    // constant propagation
    if (lhs->kind() == Expr::Kind::CONST && rhs->kind() == Expr::Kind::CONST) {
        ConstExprPtr lhs_ = std::static_pointer_cast<const ConstExpr>(lhs);
        ConstExprPtr rhs_ = std::static_pointer_cast<const ConstExpr>(rhs);
        BVConst      concrete_val = lhs_->val();
        concrete_val.srem(rhs_->val());
        ConstExpr e(concrete_val);
        return std::static_pointer_cast<const BVExpr>(get_or_create(e));
    }

    // rem with 1
    if (rhs->kind() == Expr::Kind::CONST) {
        ConstExprPtr rhs_ = std::static_pointer_cast<const ConstExpr>(rhs);
        if (rhs_->val().is_one())
            return lhs;
    }

    SRemExpr e(lhs, rhs);
    return std::static_pointer_cast<const BVExpr>(get_or_create(e));
}

BVExprPtr ExprBuilder::mk_urem(BVExprPtr lhs, BVExprPtr rhs)
{
    check_size_or_fail("urem", lhs, rhs);

    // rem zero
    if (rhs->kind() == Expr::Kind::CONST) {
        ConstExprPtr rhs_ = std::static_pointer_cast<const ConstExpr>(rhs);
        if (rhs_->val().is_zero()) {
            // we are consistent with Z3...
            return lhs;
        }
    }

    // constant propagation
    if (lhs->kind() == Expr::Kind::CONST && rhs->kind() == Expr::Kind::CONST) {
        ConstExprPtr lhs_ = std::static_pointer_cast<const ConstExpr>(lhs);
        ConstExprPtr rhs_ = std::static_pointer_cast<const ConstExpr>(rhs);
        BVConst      concrete_val = lhs_->val();
        concrete_val.urem(rhs_->val());
        ConstExpr e(concrete_val);
        return std::static_pointer_cast<const BVExpr>(get_or_create(e));
    }

    // rem with 1
    if (rhs->kind() == Expr::Kind::CONST) {
        ConstExprPtr rhs_ = std::static_pointer_cast<const ConstExpr>(rhs);
        if (rhs_->val().is_one())
            return lhs;
    }

    URemExpr e(lhs, rhs);
    return std::static_pointer_cast<const BVExpr>(get_or_create(e));
}

BVExprPtr ExprBuilder::mk_and(BVExprPtr lhs, BVExprPtr rhs)
{
    check_size_or_fail("and", lhs, rhs);

    if (lhs->kind() == Expr::Kind::CONST) {
        auto lhs_ = std::static_pointer_cast<const ConstExpr>(lhs);
        // FIXME: is mask...
    }
    if (rhs->kind() == Expr::Kind::CONST) {
        auto rhs_ = std::static_pointer_cast<const ConstExpr>(rhs);
        // FIXME: is mask...
    }

    std::vector<BVExprPtr> exprs;

    // flatten args
    if (lhs->kind() == Expr::Kind::AND) {
        auto lhs_ = std::static_pointer_cast<const AndExpr>(lhs);
        for (auto child : lhs_->els())
            exprs.push_back(child);
    } else {
        exprs.push_back(lhs);
    }

    if (rhs->kind() == Expr::Kind::AND) {
        auto rhs_ = std::static_pointer_cast<const AndExpr>(rhs);
        for (auto child : rhs_->els())
            exprs.push_back(child);
    } else {
        exprs.push_back(rhs);
    }

    std::vector<BVExprPtr> children;

    // constant propagation
    BVConst concrete_val("-1", exprs.at(0)->size());
    for (auto exp : exprs) {
        if (exp->kind() == Expr::Kind::CONST) {
            auto exp_ = std::static_pointer_cast<const ConstExpr>(exp);
            // special case: and with zero
            if (exp_->val().is_zero())
                return exp_;
            concrete_val.band(exp_->val());
        } else {
            children.push_back(exp);
        }
    }

    // final checks
    if (children.size() == 0)
        return mk_const(concrete_val);

    if (!concrete_val.has_all_bit_set())
        children.push_back(mk_const(concrete_val));

    if (children.size() == 1)
        return children.back();

    // sort children by address (commutative! We are trying to reduce the
    // number of equivalent expressions)
    std::sort(children.begin(), children.end());

    AndExpr e(children);
    return std::static_pointer_cast<const BVExpr>(get_or_create(e));
}

BVExprPtr ExprBuilder::mk_or(BVExprPtr lhs, BVExprPtr rhs)
{
    check_size_or_fail("or", lhs, rhs);

    std::vector<BVExprPtr> exprs;

    // flatten args
    if (lhs->kind() == Expr::Kind::OR) {
        auto lhs_ = std::static_pointer_cast<const OrExpr>(lhs);
        for (auto child : lhs_->els())
            exprs.push_back(child);
    } else {
        exprs.push_back(lhs);
    }

    if (rhs->kind() == Expr::Kind::OR) {
        auto rhs_ = std::static_pointer_cast<const OrExpr>(rhs);
        for (auto child : rhs_->els())
            exprs.push_back(child);
    } else {
        exprs.push_back(rhs);
    }

    std::vector<BVExprPtr> children;

    // constant propagation
    BVConst concrete_val(0UL, exprs.at(0)->size());
    for (auto exp : exprs) {
        if (exp->kind() == Expr::Kind::CONST) {
            auto exp_ = std::static_pointer_cast<const ConstExpr>(exp);
            // special case: or with 11..1
            if (exp_->val().has_all_bit_set())
                return mk_const(BVConst("-1", exprs.at(0)->size()));
            concrete_val.bor(exp_->val());
        } else {
            children.push_back(exp);
        }
    }

    // final checks
    if (children.size() == 0)
        return mk_const(concrete_val);

    if (!concrete_val.is_zero())
        children.push_back(mk_const(concrete_val));

    if (children.size() == 1)
        return children.back();

    // sort children by address (commutative! We are trying to reduce the
    // number of equivalent expressions)
    std::sort(children.begin(), children.end());

    OrExpr e(children);
    return std::static_pointer_cast<const BVExpr>(get_or_create(e));
}

BVExprPtr ExprBuilder::mk_xor(BVExprPtr lhs, BVExprPtr rhs)
{
    check_size_or_fail("xor", lhs, rhs);

    std::vector<BVExprPtr> exprs;

    // flatten args
    if (lhs->kind() == Expr::Kind::XOR) {
        auto lhs_ = std::static_pointer_cast<const XorExpr>(lhs);
        for (auto child : lhs_->els())
            exprs.push_back(child);
    } else {
        exprs.push_back(lhs);
    }

    if (rhs->kind() == Expr::Kind::XOR) {
        auto rhs_ = std::static_pointer_cast<const XorExpr>(rhs);
        for (auto child : rhs_->els())
            exprs.push_back(child);
    } else {
        exprs.push_back(rhs);
    }

    std::vector<BVExprPtr> children;

    // constant propagation
    BVConst concrete_val(0UL, exprs.at(0)->size());
    for (auto exp : exprs) {
        if (exp->kind() == Expr::Kind::CONST) {
            auto exp_ = std::static_pointer_cast<const ConstExpr>(exp);
            concrete_val.bxor(exp_->val());
        } else {
            children.push_back(exp);
        }
    }

    // remove 'xor with myself'
    std::vector<BVExprPtr> pruned_children;
    std::set<BVExprPtr>    added_exprs;
    for (auto exp : children) {
        if (added_exprs.contains(exp))
            added_exprs.erase(exp);
        else
            added_exprs.insert(exp);
    }
    for (auto exp : added_exprs)
        pruned_children.push_back(exp);

    // final checks
    if (pruned_children.size() == 0)
        return mk_const(concrete_val);

    if (!concrete_val.is_zero())
        pruned_children.push_back(mk_const(concrete_val));

    if (pruned_children.size() == 1)
        return pruned_children.back();

    // sort children by address (commutative! We are trying to reduce the
    // number of equivalent expressions)
    std::sort(pruned_children.begin(), pruned_children.end());

    XorExpr e(pruned_children);
    return std::static_pointer_cast<const BVExpr>(get_or_create(e));
}

BVExprPtr ExprBuilder::mk_sub(BVExprPtr lhs, BVExprPtr rhs)
{
    check_size_or_fail("sub", lhs, rhs);

    // constant propagation
    if (lhs->kind() == Expr::Kind::CONST && rhs->kind() == Expr::Kind::CONST) {
        ConstExprPtr lhs_ = std::static_pointer_cast<const ConstExpr>(lhs);
        ConstExprPtr rhs_ = std::static_pointer_cast<const ConstExpr>(rhs);
        BVConst      tmp(lhs_->val());
        tmp.sub(rhs_->val());
        return mk_const(tmp);
    }

    return mk_add(lhs, mk_neg(rhs));
}

BoolExprPtr ExprBuilder::mk_not(BoolExprPtr expr)
{
    // constant propagation
    if (expr->kind() == Expr::Kind::BOOL_CONST) {
        auto expr_ = std::static_pointer_cast<const BoolConst>(expr);
        return expr_->is_true() ? mk_false() : mk_true();
    }

    // double not
    if (expr->kind() == Expr::Kind::BOOL_NOT) {
        auto expr_ = std::static_pointer_cast<const BoolNotExpr>(expr);
        return expr_->expr();
    }

    BoolNotExpr e(expr);
    return std::static_pointer_cast<const BoolExpr>(get_or_create(e));
}

BoolExprPtr ExprBuilder::mk_ult(BVExprPtr lhs, BVExprPtr rhs)
{
    check_size_or_fail("ult", lhs, rhs);

    // constant propagation
    if (lhs->kind() == Expr::Kind::CONST && rhs->kind() == Expr::Kind::CONST) {
        ConstExprPtr lhs_ = std::static_pointer_cast<const ConstExpr>(lhs);
        ConstExprPtr rhs_ = std::static_pointer_cast<const ConstExpr>(rhs);
        return lhs_->val().ult(rhs_->val()) ? mk_true() : mk_false();
    }

    UltExpr e(lhs, rhs);
    return std::static_pointer_cast<const BoolExpr>(get_or_create(e));
}

BoolExprPtr ExprBuilder::mk_ule(BVExprPtr lhs, BVExprPtr rhs)
{
    check_size_or_fail("ule", lhs, rhs);

    // constant propagation
    if (lhs->kind() == Expr::Kind::CONST && rhs->kind() == Expr::Kind::CONST) {
        ConstExprPtr lhs_ = std::static_pointer_cast<const ConstExpr>(lhs);
        ConstExprPtr rhs_ = std::static_pointer_cast<const ConstExpr>(rhs);
        return lhs_->val().ule(rhs_->val()) ? mk_true() : mk_false();
    }

    UleExpr e(lhs, rhs);
    return std::static_pointer_cast<const BoolExpr>(get_or_create(e));
}

BoolExprPtr ExprBuilder::mk_ugt(BVExprPtr lhs, BVExprPtr rhs)
{
    check_size_or_fail("ugt", lhs, rhs);

    // constant propagation
    if (lhs->kind() == Expr::Kind::CONST && rhs->kind() == Expr::Kind::CONST) {
        ConstExprPtr lhs_ = std::static_pointer_cast<const ConstExpr>(lhs);
        ConstExprPtr rhs_ = std::static_pointer_cast<const ConstExpr>(rhs);
        return lhs_->val().ugt(rhs_->val()) ? mk_true() : mk_false();
    }

    UgtExpr e(lhs, rhs);
    return std::static_pointer_cast<const BoolExpr>(get_or_create(e));
}

BoolExprPtr ExprBuilder::mk_uge(BVExprPtr lhs, BVExprPtr rhs)
{
    check_size_or_fail("uge", lhs, rhs);

    // constant propagation
    if (lhs->kind() == Expr::Kind::CONST && rhs->kind() == Expr::Kind::CONST) {
        ConstExprPtr lhs_ = std::static_pointer_cast<const ConstExpr>(lhs);
        ConstExprPtr rhs_ = std::static_pointer_cast<const ConstExpr>(rhs);
        return lhs_->val().uge(rhs_->val()) ? mk_true() : mk_false();
    }

    UgeExpr e(lhs, rhs);
    return std::static_pointer_cast<const BoolExpr>(get_or_create(e));
}

BoolExprPtr ExprBuilder::mk_slt(BVExprPtr lhs, BVExprPtr rhs)
{
    check_size_or_fail("slt", lhs, rhs);

    // constant propagation
    if (lhs->kind() == Expr::Kind::CONST && rhs->kind() == Expr::Kind::CONST) {
        ConstExprPtr lhs_ = std::static_pointer_cast<const ConstExpr>(lhs);
        ConstExprPtr rhs_ = std::static_pointer_cast<const ConstExpr>(rhs);
        return lhs_->val().slt(rhs_->val()) ? mk_true() : mk_false();
    }

    SltExpr e(lhs, rhs);
    return std::static_pointer_cast<const BoolExpr>(get_or_create(e));
}

BoolExprPtr ExprBuilder::mk_sle(BVExprPtr lhs, BVExprPtr rhs)
{
    check_size_or_fail("sle", lhs, rhs);

    // constant propagation
    if (lhs->kind() == Expr::Kind::CONST && rhs->kind() == Expr::Kind::CONST) {
        ConstExprPtr lhs_ = std::static_pointer_cast<const ConstExpr>(lhs);
        ConstExprPtr rhs_ = std::static_pointer_cast<const ConstExpr>(rhs);
        return lhs_->val().sle(rhs_->val()) ? mk_true() : mk_false();
    }

    SleExpr e(lhs, rhs);
    return std::static_pointer_cast<const BoolExpr>(get_or_create(e));
}

BoolExprPtr ExprBuilder::mk_sgt(BVExprPtr lhs, BVExprPtr rhs)
{
    check_size_or_fail("sgt", lhs, rhs);

    // constant propagation
    if (lhs->kind() == Expr::Kind::CONST && rhs->kind() == Expr::Kind::CONST) {
        ConstExprPtr lhs_ = std::static_pointer_cast<const ConstExpr>(lhs);
        ConstExprPtr rhs_ = std::static_pointer_cast<const ConstExpr>(rhs);
        return lhs_->val().sgt(rhs_->val()) ? mk_true() : mk_false();
    }

    SgtExpr e(lhs, rhs);
    return std::static_pointer_cast<const BoolExpr>(get_or_create(e));
}

BoolExprPtr ExprBuilder::mk_sge(BVExprPtr lhs, BVExprPtr rhs)
{
    check_size_or_fail("sge", lhs, rhs);

    // constant propagation
    if (lhs->kind() == Expr::Kind::CONST && rhs->kind() == Expr::Kind::CONST) {
        ConstExprPtr lhs_ = std::static_pointer_cast<const ConstExpr>(lhs);
        ConstExprPtr rhs_ = std::static_pointer_cast<const ConstExpr>(rhs);
        return lhs_->val().sge(rhs_->val()) ? mk_true() : mk_false();
    }

    SgeExpr e(lhs, rhs);
    return std::static_pointer_cast<const BoolExpr>(get_or_create(e));
}

BoolExprPtr ExprBuilder::mk_eq(BVExprPtr lhs, BVExprPtr rhs)
{
    check_size_or_fail("eq", lhs, rhs);

    // constant propagation
    if (lhs->kind() == Expr::Kind::CONST && rhs->kind() == Expr::Kind::CONST) {
        ConstExprPtr lhs_ = std::static_pointer_cast<const ConstExpr>(lhs);
        ConstExprPtr rhs_ = std::static_pointer_cast<const ConstExpr>(rhs);
        return lhs_->val().eq(rhs_->val()) ? mk_true() : mk_false();
    }

    // (a - b == 0) => (a == b)
    if (lhs->kind() == Expr::Kind::ADD && rhs->kind() == Expr::Kind::CONST) {
        auto lhs_ = std::static_pointer_cast<const AddExpr>(lhs);
        auto rhs_ = std::static_pointer_cast<const ConstExpr>(rhs);

        if (rhs_->val().is_zero()) {
            NegExprPtr             negated_expr = nullptr;
            std::vector<BVExprPtr> other_exprs;
            for (auto a : lhs_->addends()) {
                if (negated_expr == negated_expr &&
                    a->kind() == Expr::Kind::NEG)
                    negated_expr = std::static_pointer_cast<const NegExpr>(a);
                else
                    other_exprs.push_back(a);
            }

            if (negated_expr != nullptr) {
                assert(other_exprs.size() > 0 &&
                       "mk_eq(): invalid number of other exprs");
                if (other_exprs.size() == 1) {
                    EqExpr e(other_exprs.at(0), negated_expr->expr());
                    return std::static_pointer_cast<const BoolExpr>(
                        get_or_create(e));
                } else {
                    AddExpr ae(other_exprs);
                    auto    ae_expr = std::static_pointer_cast<const BVExpr>(
                        get_or_create(ae));
                    EqExpr e(ae_expr, negated_expr->expr());
                    return std::static_pointer_cast<const BoolExpr>(
                        get_or_create(e));
                }
            }
        }
    }

    EqExpr e(lhs, rhs);
    return std::static_pointer_cast<const BoolExpr>(get_or_create(e));
}

BoolExprPtr ExprBuilder::mk_neq(BVExprPtr lhs, BVExprPtr rhs)
{
    return mk_not(mk_eq(lhs, rhs));
}

BoolExprPtr ExprBuilder::mk_bool_and_no_simpl(std::set<BoolExprPtr> exprs)
{
    if (exprs.size() == 0)
        return mk_true();
    if (exprs.size() == 1)
        return *exprs.begin();

    BoolAndExpr e(exprs);
    return std::static_pointer_cast<const BoolExpr>(get_or_create(e));
}

BoolExprPtr ExprBuilder::mk_bool_and(BoolExprPtr e1, BoolExprPtr e2)
{
    std::set<BoolExprPtr> exprs;

    // flatten args
    if (e1->kind() == Expr::Kind::BOOL_AND) {
        auto e1_ = std::static_pointer_cast<const BoolAndExpr>(e1);
        for (auto e : e1_->exprs())
            exprs.insert(e);
    } else {
        exprs.insert(e1);
    }

    if (e2->kind() == Expr::Kind::BOOL_AND) {
        auto e2_ = std::static_pointer_cast<const BoolAndExpr>(e2);
        for (auto e : e2_->exprs())
            exprs.insert(e);
    } else {
        exprs.insert(e2);
    }

    std::vector<BoolExprPtr> actual_exprs;

    // constant propagation
    for (auto e : exprs) {
        if (e->kind() == Expr::Kind::BOOL_CONST) {
            auto e_ = std::static_pointer_cast<const BoolConst>(e);
            if (!e_->is_true())
                return mk_false();
        } else {
            actual_exprs.push_back(e);
        }
    }

    // final checks
    if (actual_exprs.size() == 0)
        return mk_true();

    if (actual_exprs.size() == 1)
        return actual_exprs.back();

    // sort actual_exprs by address (commutative! We are trying to reduce the
    // number of equivalent expressions)
    std::sort(actual_exprs.begin(), actual_exprs.end());

    BoolAndExpr e(actual_exprs);
    return std::static_pointer_cast<const BoolExpr>(get_or_create(e));
}

BoolExprPtr ExprBuilder::mk_bool_or(BoolExprPtr e1, BoolExprPtr e2)
{
    std::set<BoolExprPtr> exprs;

    // flatten args
    if (e1->kind() == Expr::Kind::BOOL_OR) {
        auto e1_ = std::static_pointer_cast<const BoolOrExpr>(e1);
        for (auto e : e1_->exprs())
            exprs.insert(e);
    } else {
        exprs.insert(e1);
    }

    if (e2->kind() == Expr::Kind::BOOL_OR) {
        auto e2_ = std::static_pointer_cast<const BoolOrExpr>(e2);
        for (auto e : e2_->exprs())
            exprs.insert(e);
    } else {
        exprs.insert(e2);
    }

    std::vector<BoolExprPtr> actual_exprs;

    // constant propagation
    for (auto e : exprs) {
        if (e->kind() == Expr::Kind::BOOL_CONST) {
            auto e_ = std::static_pointer_cast<const BoolConst>(e);
            if (e_->is_true())
                return mk_true();
        } else {
            actual_exprs.push_back(e);
        }
    }

    // final checks
    if (actual_exprs.size() == 0)
        return mk_false();

    if (actual_exprs.size() == 1)
        return actual_exprs.back();

    // sort actual_exprs by address (commutative! We are trying to reduce the
    // number of equivalent expressions)
    std::sort(actual_exprs.begin(), actual_exprs.end());

    BoolOrExpr e(actual_exprs);
    return std::static_pointer_cast<const BoolExpr>(get_or_create(e));
}

BVExprPtr ExprBuilder::sign_bit(BVExprPtr expr)
{
    return mk_extract(expr, expr->size() - 1, expr->size() - 1);
}

BVExprPtr ExprBuilder::bool_to_bv(BoolExprPtr expr)
{
    return mk_ite(expr, mk_const(1, 8), mk_const(0, 8));
}

BoolExprPtr ExprBuilder::bv_to_bool(BVExprPtr expr)
{
    // simplify bv_to_bool(bool_to_bv)
    if (expr->kind() == Expr::Kind::ITE) {
        auto expr_ = std::static_pointer_cast<const ITEExpr>(expr);
        if (expr_->iftrue()->kind() == Expr::Kind::CONST &&
            expr_->iffalse()->kind() == Expr::Kind::CONST) {
            auto iftrue_ =
                std::static_pointer_cast<const ConstExpr>(expr_->iftrue());
            auto iffalse_ =
                std::static_pointer_cast<const ConstExpr>(expr_->iffalse());
            if (iftrue_->val().is_one() && iffalse_->val().is_zero())
                return expr_->guard();
        }
    }
    return mk_ugt(expr, mk_const(0, expr->size()));
}

FPConstExprPtr ExprBuilder::mk_fp_const(FloatFormatPtr ff, double v)
{
    FPConstExpr c(ff, v);
    return std::static_pointer_cast<const FPConstExpr>(get_or_create(c));
}

FPConstExprPtr ExprBuilder::mk_fp_const(const FPConst& c)
{
    FPConstExpr e(c);
    return std::static_pointer_cast<const FPConstExpr>(get_or_create(e));
}

FPExprPtr ExprBuilder::mk_bv_to_fp(FloatFormatPtr ff, BVExprPtr expr)
{
    if (ff->getSize() * 8 != expr->size()) {
        err("ExprBuilder") << "mk_bv_to_fp(): inconsistent size" << std::endl;
        exit_fail();
    }

    // constant propagation
    if (expr->kind() == Expr::Kind::CONST) {
        auto        expr_ = std::static_pointer_cast<const ConstExpr>(expr);
        FPConstExpr c(FPConst(ff, expr_->val().as_u64()));
        return std::static_pointer_cast<const FPConstExpr>(get_or_create(c));
    }

    // bv_to_fp of fp_to_bv
    if (expr->kind() == Expr::Kind::FP_TO_BV) {
        auto expr_ = std::static_pointer_cast<const FPToBVExpr>(expr);
        return expr_->expr();
    }

    BVToFPExpr e(ff, expr);
    return std::static_pointer_cast<const FPExpr>(get_or_create(e));
}

BVExprPtr ExprBuilder::mk_fp_to_bv(FPExprPtr expr)
{
    // constant propagation
    if (expr->kind() == Expr::Kind::FP_CONST) {
        auto expr_ = std::static_pointer_cast<const FPConstExpr>(expr);
        return mk_const(expr_->val().val(), expr_->val().size());
    }

    // fp_to_bv of bv_to_fp
    if (expr->kind() == Expr::Kind::BV_TO_FP) {
        auto expr_ = std::static_pointer_cast<const BVToFPExpr>(expr);
        return expr_->expr();
    }

    FPToBVExpr e(expr);
    return std::static_pointer_cast<const BVExpr>(get_or_create(e));
}

FPExprPtr ExprBuilder::mk_fp_convert(FPExprPtr expr, FloatFormatPtr ff)
{
    // unuseful op
    if (ff == expr->ff())
        return expr;

    // constant propagation
    if (expr->kind() == Expr::Kind::FP_CONST) {
        auto expr_ = std::static_pointer_cast<const FPConstExpr>(expr);
        auto val   = expr_->val();
        val.convert(ff);
        FPConstExpr c(val);
        return std::static_pointer_cast<const FPConstExpr>(get_or_create(c));
    }

    FPConvert e(expr, ff);
    return std::static_pointer_cast<const FPExpr>(get_or_create(e));
}

FPExprPtr ExprBuilder::mk_int_to_fp(BVExprPtr expr, FloatFormatPtr ff)
{
    // constant propagation
    if (expr->kind() == Expr::Kind::CONST) {
        auto        expr_ = std::static_pointer_cast<const ConstExpr>(expr);
        auto        val   = expr_->val();
        double      v     = (double)val.as_s64();
        FPConstExpr c(FPConst(ff, v));
        return std::static_pointer_cast<const FPConstExpr>(get_or_create(c));
    }

    IntToFPExpr e(expr, ff);
    return std::static_pointer_cast<const FPExpr>(get_or_create(e));
}

BoolExprPtr ExprBuilder::mk_fp_is_nan(FPExprPtr expr)
{
    // constant propagation
    if (expr->kind() == Expr::Kind::FP_CONST) {
        auto expr_ = std::static_pointer_cast<const FPConstExpr>(expr);
        auto val   = expr_->val();
        return val.is_nan() ? mk_true() : mk_false();
    }

    FPIsNAN e(expr);
    return std::static_pointer_cast<const FPIsNAN>(get_or_create(e));
}

FPExprPtr ExprBuilder::mk_fp_neg(FPExprPtr expr)
{
    // constant propagation
    if (expr->kind() == Expr::Kind::FP_CONST) {
        auto expr_ = std::static_pointer_cast<const FPConstExpr>(expr);
        auto tmp   = expr_->val();
        tmp.neg();
        FPConstExpr c(tmp);
        return std::static_pointer_cast<const FPConstExpr>(get_or_create(c));
    }

    FPNegExpr e(expr);
    return std::static_pointer_cast<const FPExpr>(get_or_create(e));
}

FPExprPtr ExprBuilder::mk_fp_add(FPExprPtr lhs, FPExprPtr rhs)
{
    check_fp_type_or_fail("fp_add", lhs, rhs);

    std::vector<FPExprPtr> addends;

    // flatten args
    if (lhs->kind() == Expr::Kind::FP_ADD) {
        auto lhs_ = std::static_pointer_cast<const FPAddExpr>(lhs);
        for (auto child : lhs_->els())
            addends.push_back(child);
    } else {
        addends.push_back(lhs);
    }

    if (rhs->kind() == Expr::Kind::FP_ADD) {
        auto rhs_ = std::static_pointer_cast<const FPAddExpr>(rhs);
        for (auto child : rhs_->els())
            addends.push_back(child);
    } else {
        addends.push_back(rhs);
    }

    std::vector<FPExprPtr> children;

    // constant propagation
    FPConst concrete_val(addends.at(0)->ff(), 0.0);
    for (auto addend : addends) {
        if (addend->kind() == Expr::Kind::CONST) {
            auto addend_ = std::static_pointer_cast<const FPConstExpr>(addend);
            concrete_val.add(addend_->val());
        } else {
            children.push_back(addend);
        }
    }

    // remove 'add with negated'
    std::vector<FPExprPtr>   pruned_children;
    std::map<FPExprPtr, int> negated_exprs;
    std::map<FPExprPtr, int> normal_exprs;
    for (auto exp : children) {
        if (exp->kind() == Expr::Kind::FP_NEG) {
            auto exp_ = std::static_pointer_cast<const FPNegExpr>(exp);
            if (normal_exprs.contains(exp_->expr())) {
                if (normal_exprs[exp_->expr()] == 1)
                    normal_exprs.erase(exp_->expr());
                else
                    normal_exprs[exp_->expr()] -= 1;
            } else if (!negated_exprs.contains(exp_->expr()))
                negated_exprs[exp_->expr()] = 1;
            else
                negated_exprs[exp_->expr()] += 1;
        } else {
            if (negated_exprs.contains(exp)) {
                if (negated_exprs[exp] == 1)
                    negated_exprs.erase(exp);
                else
                    negated_exprs[exp] -= 1;
            } else if (!normal_exprs.contains(exp))
                normal_exprs[exp] = 1;
            else
                normal_exprs[exp] += 1;
        }
    }
    for (auto pair : negated_exprs) {
        if (pair.second > 1)
            pruned_children.push_back(
                mk_fp_mul(mk_fp_neg(pair.first),
                          mk_fp_const(pair.first->ff(), (double)pair.second)));
        else
            pruned_children.push_back(mk_fp_neg(pair.first));
    }
    for (auto pair : normal_exprs)
        if (pair.second > 1)
            pruned_children.push_back(
                mk_fp_mul(pair.first,
                          mk_fp_const(pair.first->ff(), (double)pair.second)));
        else
            pruned_children.push_back(pair.first);

    // final checks
    if (pruned_children.size() == 0)
        return mk_fp_const(concrete_val);

    if (!concrete_val.is_zero())
        pruned_children.push_back(mk_fp_const(concrete_val));

    if (pruned_children.size() == 1)
        return pruned_children.back();

    // sort children by address (commutative! We are trying to reduce the
    // number of equivalent expressions)
    std::sort(pruned_children.begin(), pruned_children.end());

    FPAddExpr e(pruned_children);
    return std::static_pointer_cast<const FPExpr>(get_or_create(e));
}

FPExprPtr ExprBuilder::mk_fp_sub(FPExprPtr lhs, FPExprPtr rhs)
{
    return mk_fp_add(lhs, mk_fp_neg(rhs));
}

FPExprPtr ExprBuilder::mk_fp_mul(FPExprPtr lhs, FPExprPtr rhs)
{
    check_fp_type_or_fail("fp_mul", lhs, rhs);

    std::vector<FPExprPtr> els;

    // flatten args
    if (lhs->kind() == Expr::Kind::FP_MUL) {
        auto lhs_ = std::static_pointer_cast<const FPMulExpr>(lhs);
        for (auto child : lhs_->els())
            els.push_back(child);
    } else {
        els.push_back(lhs);
    }

    if (rhs->kind() == Expr::Kind::FP_MUL) {
        auto rhs_ = std::static_pointer_cast<const FPMulExpr>(rhs);
        for (auto child : rhs_->els())
            els.push_back(child);
    } else {
        els.push_back(rhs);
    }

    std::vector<FPExprPtr> children;

    // constant propagation
    FPConst concrete_val(els.at(0)->ff(), 1.0);
    for (auto e : els) {
        if (e->kind() == Expr::Kind::CONST) {
            auto e_ = std::static_pointer_cast<const FPConstExpr>(e);
            concrete_val.mul(e_->val());
        } else {
            children.push_back(e);
        }
    }

    // final checks
    if (children.size() == 0 || concrete_val.is_zero())
        return mk_fp_const(concrete_val);

    if (!concrete_val.is_one())
        children.push_back(mk_fp_const(concrete_val));

    if (children.size() == 1)
        return children.back();

    // sort children by address (commutative! We are trying to reduce the
    // number of equivalent expressions)
    std::sort(children.begin(), children.end());

    FPMulExpr e(children);
    return std::static_pointer_cast<const FPExpr>(get_or_create(e));
}

FPExprPtr ExprBuilder::mk_fp_div(FPExprPtr lhs, FPExprPtr rhs)
{
    check_fp_type_or_fail("fp_div", lhs, rhs);

    // constant propagation
    if (lhs->kind() == Expr::Kind::FP_CONST &&
        rhs->kind() == Expr::Kind::FP_CONST) {
        auto lhs_ = std::static_pointer_cast<const FPConstExpr>(lhs);
        auto rhs_ = std::static_pointer_cast<const FPConstExpr>(rhs);
        auto v    = lhs_->val();
        v.div(rhs_->val());
        FPConstExpr c(v);
        return std::static_pointer_cast<const FPConstExpr>(get_or_create(c));
    }

    FPDivExpr e(lhs, rhs);
    return std::static_pointer_cast<const FPDivExpr>(get_or_create(e));
}

BoolExprPtr ExprBuilder::mk_fp_lt(FPExprPtr lhs, FPExprPtr rhs)
{
    check_fp_type_or_fail("fp_lt", lhs, rhs);

    // constant propagation
    if (lhs->kind() == Expr::Kind::FP_CONST &&
        rhs->kind() == Expr::Kind::FP_CONST) {
        auto lhs_ = std::static_pointer_cast<const FPConstExpr>(lhs);
        auto rhs_ = std::static_pointer_cast<const FPConstExpr>(rhs);
        return lhs_->val().lt(rhs_->val()) ? mk_true() : mk_false();
    }

    FPLtExpr e(lhs, rhs);
    return std::static_pointer_cast<const FPLtExpr>(get_or_create(e));
}

BoolExprPtr ExprBuilder::mk_fp_eq(FPExprPtr lhs, FPExprPtr rhs)
{
    check_fp_type_or_fail("fp_eq", lhs, rhs);

    // constant propagation
    if (lhs->kind() == Expr::Kind::FP_CONST &&
        rhs->kind() == Expr::Kind::FP_CONST) {
        auto lhs_ = std::static_pointer_cast<const FPConstExpr>(lhs);
        auto rhs_ = std::static_pointer_cast<const FPConstExpr>(rhs);
        return lhs_->val().eq(rhs_->val()) ? mk_true() : mk_false();
    }

    FPEqExpr e(lhs, rhs);
    return std::static_pointer_cast<const FPEqExpr>(get_or_create(e));
}

} // namespace naaz::expr
