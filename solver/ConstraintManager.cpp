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
    : m_constraints(other.m_constraints), m_dependencies(other.m_dependencies)
{
}

void ConstraintManager::add(expr::BoolExprPtr constraint)
{
    const std::set<uint32_t>& involved_inputs = get_involved_inputs(constraint);

    for (auto& sym : involved_inputs) {
        m_constraints[sym].insert(constraint);
        for (auto& sym_again : involved_inputs)
            m_dependencies[sym].insert(sym_again);
    }
}

std::set<uint32_t>
ConstraintManager::get_dependencies(expr::BoolExprPtr constraint) const
{
    const std::set<uint32_t>& involved_inputs = get_involved_inputs(constraint);

    std::vector<uint32_t> queue;
    for (auto& sym : involved_inputs)
        queue.push_back(sym);

    std::set<uint32_t> res;
    while (queue.size() > 0) {
        uint32_t curr = queue.back();
        queue.pop_back();

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

expr::BoolExprPtr
ConstraintManager::build_query(expr::BoolExprPtr constraint) const
{
    std::set<uint32_t> involved_inputs = get_dependencies(constraint);

    std::set<expr::BoolExprPtr> constraints;
    for (auto& sym : involved_inputs) {
        if (!m_constraints.contains(sym))
            continue;
        for (expr::BoolExprPtr c : m_constraints.at(sym))
            constraints.insert(c);
    }

    expr::BoolExprPtr q = constraint;
    for (auto c : constraints)
        q = exprBuilder.mk_bool_and(q, c);
    return q;
}

} // namespace naaz::solver
