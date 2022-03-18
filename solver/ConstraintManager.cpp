#include "ConstraintManager.hpp"

#include "../expr/ExprBuilder.hpp"

#define exprBuilder naaz::expr::ExprBuilder::The()

namespace naaz::solver
{

std::map<expr::ExprPtr, std::set<uint32_t>>
    ConstraintManager::g_involved_symbols;

const std::set<uint32_t>&
ConstraintManager::get_involved_inputs(expr::ExprPtr constraint)
{
    if (g_involved_symbols.contains(constraint)) {
        return g_involved_symbols.at(constraint);
    }

    std::set<uint32_t> res;
    if (constraint->kind() == expr::Expr::Kind::SYM) {
        auto constraint_ =
            std::static_pointer_cast<const expr::SymExpr>(constraint);
        res.insert(constraint_->id());
    } else {
        for (expr::ExprPtr e : constraint->children())
            for (auto& sym : get_involved_inputs(e))
                res.insert(sym);
    }

    g_involved_symbols[constraint] = res;
    return g_involved_symbols[constraint];
}

ConstraintManager::ConstraintManager(const ConstraintManager& other)
    : m_constraint_map(other.m_constraint_map),
      m_constraints(other.m_constraints), m_dependencies(other.m_dependencies)
{
}

void ConstraintManager::add(expr::BoolExprPtr constraint)
{
    const std::set<uint32_t>& involved_inputs = get_involved_inputs(constraint);

    for (auto& sym : involved_inputs) {
        m_constraint_map[sym].insert(constraint);
        for (auto& sym_again : involved_inputs)
            m_dependencies[sym].insert(sym_again);
    }

    m_constraints.insert(constraint);
}

std::set<uint32_t>
ConstraintManager::get_dependencies(expr::ExprPtr constraint) const
{
    const std::set<uint32_t>& involved_inputs = get_involved_inputs(constraint);

    std::vector<uint32_t> queue;
    for (auto& sym : involved_inputs)
        queue.push_back(sym);

    std::set<uint32_t> res;
    while (queue.size() > 0) {
        uint32_t curr = queue.back();
        queue.pop_back();

        if (res.contains(curr))
            continue;

        res.insert(curr);
        if (m_dependencies.contains(curr)) {
            for (uint32_t next : m_dependencies.at(curr)) {
                if (!res.contains(next))
                    queue.push_back(next);
            }
        }
    }

    return res;
}

expr::BoolExprPtr ConstraintManager::pi(expr::ExprPtr expr) const
{
    std::set<uint32_t> involved_inputs = get_dependencies(expr);

    std::set<expr::BoolExprPtr> constraints;
    for (auto& sym : involved_inputs) {
        if (!m_constraint_map.contains(sym))
            continue;
        for (expr::BoolExprPtr c : m_constraint_map.at(sym))
            constraints.insert(c);
    }

    // This is a potential hot function
    // Use a faster version of mk_bool (no simpl. though!)
    return exprBuilder.mk_bool_and_no_simpl(constraints);
}

expr::BoolExprPtr ConstraintManager::pi() const
{
    if (m_constraints.size() == 0)
        return exprBuilder.mk_true();

    auto              it = m_constraints.begin();
    expr::BoolExprPtr e  = *it;
    it++;

    for (; it != m_constraints.end(); it++)
        e = exprBuilder.mk_bool_and(e, *it);

    return e;
}

} // namespace naaz::solver
