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

SymExprPtr ExprBuilder::mk_sym(const std::string& name, size_t size)
{
    SymExpr e(name, size);
    return std::static_pointer_cast<const SymExpr>(get_or_create(e));
}

ConstExprPtr ExprBuilder::mk_const(__uint128_t val, size_t size)
{
    ConstExpr e(val, size);
    return std::static_pointer_cast<const ConstExpr>(get_or_create(e));
}

AddExprPtr ExprBuilder::mk_add(ExprPtr lhs, ExprPtr rhs)
{
    AddExpr e(lhs, rhs);
    return std::static_pointer_cast<const AddExpr>(get_or_create(e));
}

SubExprPtr ExprBuilder::mk_sub(ExprPtr lhs, ExprPtr rhs)
{
    SubExpr e(lhs, rhs);
    return std::static_pointer_cast<const SubExpr>(get_or_create(e));
}

} // namespace naaz::expr
