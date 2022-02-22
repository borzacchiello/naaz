#include "../util/ioutil.hpp"

#include "ExprBuilder.hpp"

namespace naaz::expr
{

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

static inline __uint128_t bitmask(uint32_t n)
{
    if (n > 128) {
        err("ExprBuilder") << "bitmask does not support value of n > 128"
                           << std::endl;
        exit_fail();
    }
    return ((__uint128_t)2 << (__uint128_t)(n - 1)) - (__uint128_t)1;
}

BoolConstPtr ExprBuilder::mk_true()
{
    static const BoolConstPtr e = std::make_shared<const BoolConst>(1);
    return e;
}

BoolConstPtr ExprBuilder::mk_false()
{
    static const BoolConstPtr e = std::make_shared<const BoolConst>(0);
    return e;
}

SymExprPtr ExprBuilder::mk_sym(const std::string& name, size_t size)
{
    if (size > 128) {
        err("ExprBuilder") << "mk_sym(): size > 128 is not supported"
                           << std::endl;
        exit_fail();
    }

    SymExpr e(name, size);
    return std::static_pointer_cast<const SymExpr>(get_or_create(e));
}

ConstExprPtr ExprBuilder::mk_const(__uint128_t val, size_t size)
{
    if (size > 128) {
        err("ExprBuilder") << "mk_const(): size > 128 is not supported"
                           << std::endl;
        exit_fail();
    }

    ConstExpr e(val & bitmask(size), size);
    return std::static_pointer_cast<const ConstExpr>(get_or_create(e));
}

BVExprPtr ExprBuilder::mk_extract(BVExprPtr expr, uint32_t high, uint32_t low)
{
    if (high < low) {
        err("ExprBuilder") << "mk_extract(): high < low" << std::endl;
        exit_fail();
    }

    // redundant extract
    if (low == 0 && high == expr->size() - 1) {
        return expr;
    }

    // constant propagation
    if (expr->kind() == Expr::Kind::CONST) {
        ConstExprPtr expr_ = std::static_pointer_cast<const ConstExpr>(expr);
        return mk_const((expr_->val() >> (__uint128_t)low) &
                            bitmask(high - low + 1),
                        high - low + 1);
    }

    // extract of concat
    if (expr->kind() == Expr::Kind::CONCAT) {
        ConcatExprPtr expr_ = std::static_pointer_cast<const ConcatExpr>(expr);
        if (high < expr_->rhs()->size()) {
            return mk_extract(expr_->rhs(), high, low);
        } else if (low >= expr_->rhs()->size()) {
            uint32_t off = expr_->rhs()->size();
            return mk_extract(expr_->lhs(), high - off, low - off);
        }
    }

    // extract of zext/sext
    // TODO

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

    // constant propagation
    if (e->kind() == Expr::Kind::CONST) {
        auto e_ = std::static_pointer_cast<const ConstExpr>(e);
        return mk_const(e_->val(), n);
    }

    ZextExpr r(e, n);
    return std::static_pointer_cast<const BVExpr>(get_or_create(r));
}

BVExprPtr ExprBuilder::mk_ite(BoolExprPtr guard, BVExprPtr iftrue,
                              BVExprPtr iffalse)
{
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
    // constant propagation
    if (left->kind() == Expr::Kind::CONST &&
        right->kind() == Expr::Kind::CONST) {
        auto left_  = std::static_pointer_cast<const ConstExpr>(left);
        auto right_ = std::static_pointer_cast<const ConstExpr>(right);
        return mk_const(right_->val() | (left_->val() << right_->size()),
                        left->size() + right->size());
    }

    // concat of extract
    if (left->kind() == Expr::Kind::EXTRACT &&
        right->kind() == Expr::Kind::EXTRACT) {
        auto left_  = std::static_pointer_cast<const ExtractExpr>(left);
        auto right_ = std::static_pointer_cast<const ExtractExpr>(right);
        if (left_->expr() == right_->expr() &&
            left_->low() - 1 == right_->high())
            return mk_extract(left_->expr(), left_->high(), right_->low());
    }

    ConcatExpr e(left, right);
    return std::static_pointer_cast<const BVExpr>(get_or_create(e));
}

BVExprPtr ExprBuilder::mk_neg(BVExprPtr expr)
{
    // constant propagation
    if (expr->kind() == Expr::Kind::CONST) {
        ConstExprPtr expr_ = std::static_pointer_cast<const ConstExpr>(expr);
        return mk_const((__uint128_t)-expr_->sval(), expr_->size());
    }

    NegExpr e(expr);
    return std::static_pointer_cast<const BVExpr>(get_or_create(e));
}

BVExprPtr ExprBuilder::mk_add(BVExprPtr lhs, BVExprPtr rhs)
{
    check_size_or_fail("add", lhs, rhs);

    std::vector<BVExprPtr> addends;

    // flatten args
    if (lhs->kind() == Expr::Kind::ADD) {
        auto lhs_ = std::static_pointer_cast<const AddExpr>(lhs);
        for (const auto& child : lhs_->children())
            addends.push_back(child);
    } else {
        addends.push_back(lhs);
    }

    if (rhs->kind() == Expr::Kind::ADD) {
        auto rhs_ = std::static_pointer_cast<const AddExpr>(rhs);
        for (const auto& child : rhs_->children())
            addends.push_back(child);
    } else {
        addends.push_back(rhs);
    }

    std::vector<BVExprPtr> children;

    // constant propagation
    __uint128_t concrete_val = 0;
    for (const auto& addend : addends) {
        if (addend->kind() == Expr::Kind::CONST) {
            auto addend_ = std::static_pointer_cast<const ConstExpr>(addend);
            concrete_val += addend_->val();
        } else {
            children.push_back(addend);
        }
    }

    if (children.size() == 0)
        return mk_const(concrete_val, lhs->size());

    if (children.size() == 1 && concrete_val == 0)
        return children.back();

    if (concrete_val > 0)
        children.push_back(mk_const(concrete_val, lhs->size()));

    // sort children by address
    std::sort(children.begin(), children.end());

    AddExpr e(children);
    return std::static_pointer_cast<const BVExpr>(get_or_create(e));
}

BVExprPtr ExprBuilder::mk_sub(BVExprPtr lhs, BVExprPtr rhs)
{
    check_size_or_fail("sub", lhs, rhs);

    // constant propagation
    if (lhs->kind() == Expr::Kind::CONST && rhs->kind() == Expr::Kind::CONST) {
        ConstExprPtr lhs_ = std::static_pointer_cast<const ConstExpr>(lhs);
        ConstExprPtr rhs_ = std::static_pointer_cast<const ConstExpr>(rhs);
        return mk_const(lhs_->val() - rhs_->val(), lhs->size());
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

    NotExpr e(expr);
    return std::static_pointer_cast<const BoolExpr>(get_or_create(e));
}

BoolExprPtr ExprBuilder::mk_ult(BVExprPtr lhs, BVExprPtr rhs)
{
    check_size_or_fail("ult", lhs, rhs);

    // constant propagation
    if (lhs->kind() == Expr::Kind::CONST && rhs->kind() == Expr::Kind::CONST) {
        ConstExprPtr lhs_ = std::static_pointer_cast<const ConstExpr>(lhs);
        ConstExprPtr rhs_ = std::static_pointer_cast<const ConstExpr>(rhs);
        return lhs_->val() < rhs_->val() ? mk_true() : mk_false();
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
        return lhs_->val() <= rhs_->val() ? mk_true() : mk_false();
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
        return lhs_->val() > rhs_->val() ? mk_true() : mk_false();
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
        return lhs_->val() >= rhs_->val() ? mk_true() : mk_false();
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
        return lhs_->sval() < rhs_->sval() ? mk_true() : mk_false();
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
        return lhs_->sval() <= rhs_->sval() ? mk_true() : mk_false();
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
        return lhs_->sval() > rhs_->sval() ? mk_true() : mk_false();
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
        return lhs_->sval() >= rhs_->sval() ? mk_true() : mk_false();
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
        return lhs_->val() == rhs_->val() ? mk_true() : mk_false();
    }

    EqExpr e(lhs, rhs);
    return std::static_pointer_cast<const BoolExpr>(get_or_create(e));
}

BVExprPtr ExprBuilder::sign_bit(BVExprPtr expr)
{
    return mk_extract(expr, expr->size() - 1, expr->size() - 1);
}

BVExprPtr ExprBuilder::bool_to_bv(BoolExprPtr expr)
{
    return mk_ite(expr, mk_const(1, 1), mk_const(0, 1));
}

} // namespace naaz::expr
