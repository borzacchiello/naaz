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

const std::string& ExprBuilder::get_sym_name(uint32_t id) const
{
    // It raises an exeption if the id does not name a symbol
    return m_sym_id_to_name.at(id);
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
    ConstExpr e(val, size);
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
        BVConst      tmp(expr_->val());
        tmp.extract(high, low);
        return mk_const(tmp);
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
        auto    e_ = std::static_pointer_cast<const ConstExpr>(e);
        BVConst tmp(e_->val());
        tmp.zext(n);
        return mk_const(e_->val());
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
        auto    left_  = std::static_pointer_cast<const ConstExpr>(left);
        auto    right_ = std::static_pointer_cast<const ConstExpr>(right);
        BVConst tmp(left_->val());
        tmp.concat(right_->val());
        return mk_const(tmp);
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

BVExprPtr ExprBuilder::mk_add(BVExprPtr lhs, BVExprPtr rhs)
{
    check_size_or_fail("add", lhs, rhs);

    std::vector<BVExprPtr> addends;

    // flatten args
    if (lhs->kind() == Expr::Kind::ADD) {
        auto lhs_ = std::static_pointer_cast<const AddExpr>(lhs);
        for (const auto& child : lhs_->addends())
            addends.push_back(child);
    } else {
        addends.push_back(lhs);
    }

    if (rhs->kind() == Expr::Kind::ADD) {
        auto rhs_ = std::static_pointer_cast<const AddExpr>(rhs);
        for (const auto& child : rhs_->addends())
            addends.push_back(child);
    } else {
        addends.push_back(rhs);
    }

    std::vector<BVExprPtr> children;

    // constant propagation
    BVConst concrete_val(0UL, addends.at(0)->size());
    for (const auto& addend : addends) {
        if (addend->kind() == Expr::Kind::CONST) {
            auto addend_ = std::static_pointer_cast<const ConstExpr>(addend);
            concrete_val.add(addend_->val());
        } else {
            children.push_back(addend);
        }
    }

    if (children.size() == 0)
        return mk_const(concrete_val);

    if (children.size() == 1 && concrete_val.is_zero())
        return children.back();

    if (!concrete_val.is_zero())
        children.push_back(mk_const(concrete_val));

    // sort children by address (commutative! We are trying to reduce the
    // number of equivalent expressions)
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

    EqExpr e(lhs, rhs);
    return std::static_pointer_cast<const BoolExpr>(get_or_create(e));
}

BoolExprPtr ExprBuilder::mk_bool_and(BoolExprPtr e1, BoolExprPtr e2)
{
    std::set<BoolExprPtr> exprs;

    // flatten args
    if (e1->kind() == Expr::Kind::BOOL_AND) {
        auto e1_ = std::static_pointer_cast<const BoolAndExpr>(e1);
        for (const auto& e : e1_->exprs())
            exprs.insert(e);
    } else {
        exprs.insert(e1);
    }

    if (e2->kind() == Expr::Kind::BOOL_AND) {
        auto e2_ = std::static_pointer_cast<const BoolAndExpr>(e2);
        for (const auto& e : e2_->exprs())
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

BVExprPtr ExprBuilder::sign_bit(BVExprPtr expr)
{
    return mk_extract(expr, expr->size() - 1, expr->size() - 1);
}

BVExprPtr ExprBuilder::bool_to_bv(BoolExprPtr expr)
{
    return mk_ite(expr, mk_const(1, 1), mk_const(0, 1));
}

BoolExprPtr ExprBuilder::bv_to_bool(BVExprPtr expr)
{
    return mk_not(mk_eq(expr, mk_const(0, expr->size())));
}

} // namespace naaz::expr
