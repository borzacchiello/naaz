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

static inline void check_size_or_fail(const std::string& op_name, ExprPtr lhs,
                                      ExprPtr rhs)
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

    ConstExpr e(val, size);
    return std::static_pointer_cast<const ConstExpr>(get_or_create(e));
}

ExprPtr ExprBuilder::mk_extract(ExprPtr expr, uint32_t high, uint32_t low)
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
    return get_or_create(e);
}

ExprPtr ExprBuilder::mk_concat(ExprPtr left, ExprPtr right)
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
    return get_or_create(e);
}

ExprPtr ExprBuilder::mk_add(ExprPtr lhs, ExprPtr rhs)
{
    check_size_or_fail("add", lhs, rhs);

    // FIXME: this should be smarter
    // constant propagation
    if (lhs->kind() == Expr::Kind::CONST && rhs->kind() == Expr::Kind::CONST) {
        ConstExprPtr lhs_ = std::static_pointer_cast<const ConstExpr>(lhs);
        ConstExprPtr rhs_ = std::static_pointer_cast<const ConstExpr>(rhs);
        return mk_const(lhs_->val() + rhs_->val(), lhs->size());
    }

    // keep constants to the right
    if (lhs->kind() == Expr::Kind::CONST) {
        AddExpr e(rhs, lhs);
        return get_or_create(e);
    }

    AddExpr e(lhs, rhs);
    return get_or_create(e);
}

ExprPtr ExprBuilder::mk_sub(ExprPtr lhs, ExprPtr rhs)
{
    check_size_or_fail("sub", lhs, rhs);

    // constant propagation
    if (lhs->kind() == Expr::Kind::CONST && rhs->kind() == Expr::Kind::CONST) {
        ConstExprPtr lhs_ = std::static_pointer_cast<const ConstExpr>(lhs);
        ConstExprPtr rhs_ = std::static_pointer_cast<const ConstExpr>(rhs);
        return mk_const(lhs_->val() - rhs_->val(), lhs->size());
    }

    SubExpr e(lhs, rhs);
    return get_or_create(e);
}

} // namespace naaz::expr
