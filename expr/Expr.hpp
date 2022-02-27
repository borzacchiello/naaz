#pragma once

#include <cstdint>
#include <memory>
#include <vector>
#include <set>
#include <z3++.h>

#include "BVConst.hpp"

namespace naaz::expr
{

class Expr;
typedef std::shared_ptr<const Expr> ExprPtr;
typedef std::weak_ptr<const Expr>   WeakExprPtr;

class ExprBuilder;

class Expr
{
  public:
    enum Kind {
        SYM,
        CONST,
        BOOL_CONST,
        EXTRACT,
        CONCAT,
        ZEXT,
        SEXT,
        ITE,

        // arithmetic
        NEG,
        ADD,

        // logical
        NOT,
        ULT,
        ULE,
        UGT,
        UGE,
        SLT,
        SLE,
        SGT,
        SGE,
        EQ,

        BOOL_AND
    };

    virtual const Kind kind() const = 0;

    virtual uint64_t             hash() const                  = 0;
    virtual bool                 eq(ExprPtr other) const       = 0;
    virtual ExprPtr              clone() const                 = 0;
    virtual std::string          to_string() const             = 0;
    virtual std::vector<ExprPtr> children() const              = 0;
    virtual z3::expr             to_z3(z3::context& ctx) const = 0;

    friend class ExprBuilder;
};

class BVExpr : public Expr
{
  public:
    virtual size_t size() const = 0;
};
typedef std::shared_ptr<const BVExpr> BVExprPtr;

class BoolExpr : public Expr
{
};
typedef std::shared_ptr<const BoolExpr> BoolExprPtr;

class SymExpr final : public BVExpr
{
  private:
    static const Kind ekind = Kind::SYM;

    uint32_t m_id;
    size_t   m_size;

  protected:
    SymExpr(uint32_t id, size_t bits) : m_id(id), m_size(bits) {}

  public:
    virtual const Kind kind() const { return ekind; }
    virtual size_t     size() const { return m_size; }
    virtual ExprPtr clone() const { return ExprPtr(new SymExpr(m_id, m_size)); }

    virtual uint64_t             hash() const;
    virtual bool                 eq(ExprPtr other) const;
    virtual std::string          to_string() const;
    virtual z3::expr             to_z3(z3::context& ctx) const;
    virtual std::vector<ExprPtr> children() const
    {
        return std::vector<ExprPtr>();
    }

    uint32_t id() const { return m_id; }
    const std::string& name() const;

    friend class ExprBuilder;
};
typedef std::shared_ptr<const SymExpr> SymExprPtr;

class ConstExpr final : public BVExpr
{
  private:
    static const Kind ekind = Kind::CONST;

    const BVConst m_val;
    uint64_t      m_hash;

  protected:
    ConstExpr(uint64_t val, size_t size);
    ConstExpr(const BVConst& val);

  public:
    virtual const Kind kind() const { return ekind; };
    virtual size_t     size() const { return m_val.size(); };
    virtual ExprPtr    clone() const { return ExprPtr(new ConstExpr(m_val)); }

    virtual uint64_t             hash() const;
    virtual bool                 eq(ExprPtr other) const;
    virtual std::string          to_string() const;
    virtual z3::expr             to_z3(z3::context& ctx) const;
    virtual std::vector<ExprPtr> children() const
    {
        return std::vector<ExprPtr>();
    }

    const BVConst& val() const { return m_val; }

    friend class ExprBuilder;
};
typedef std::shared_ptr<const ConstExpr> ConstExprPtr;

class ITEExpr final : public BVExpr
{
  private:
    static const Kind ekind = Kind::ITE;

    BoolExprPtr m_guard;
    BVExprPtr   m_iftrue;
    BVExprPtr   m_iffalse;
    size_t      m_size;

  protected:
    ITEExpr(BoolExprPtr guard, BVExprPtr iftrue, BVExprPtr iffalse)
        : m_guard(guard), m_iftrue(iftrue), m_iffalse(iffalse)
    {
        m_size = iftrue->size();
    }

  public:
    virtual const Kind kind() const { return ekind; };
    virtual size_t     size() const { return m_size; };
    virtual ExprPtr    clone() const
    {
        return ExprPtr(new ITEExpr(m_guard, m_iftrue, m_iffalse));
    }

    virtual uint64_t             hash() const;
    virtual bool                 eq(ExprPtr other) const;
    virtual std::string          to_string() const;
    virtual z3::expr             to_z3(z3::context& ctx) const;
    virtual std::vector<ExprPtr> children() const
    {
        return std::vector<ExprPtr>{m_guard, m_iftrue, m_iffalse};
    }

    BoolExprPtr guard() const { return m_guard; }
    BVExprPtr   iftrue() const { return m_iftrue; }
    BVExprPtr   iffalse() const { return m_iffalse; }

    friend class ExprBuilder;
};
typedef std::shared_ptr<const ITEExpr> ITEExprPtr;

class ExtractExpr final : public BVExpr
{
  private:
    static const Kind ekind = Kind::EXTRACT;

    BVExprPtr m_expr;
    uint32_t  m_high;
    uint32_t  m_low;

  protected:
    ExtractExpr(BVExprPtr expr, uint32_t high, uint32_t low)
        : m_expr(expr), m_high(high), m_low(low)
    {
    }

  public:
    virtual const Kind kind() const { return ekind; };
    virtual size_t     size() const { return m_high - m_low + 1; };
    virtual ExprPtr    clone() const
    {
        return ExprPtr(new ExtractExpr(m_expr, m_high, m_low));
    }

    virtual uint64_t             hash() const;
    virtual bool                 eq(ExprPtr other) const;
    virtual std::string          to_string() const;
    virtual z3::expr             to_z3(z3::context& ctx) const;
    virtual std::vector<ExprPtr> children() const
    {
        return std::vector<ExprPtr>{m_expr};
    }

    BVExprPtr expr() const { return m_expr; }
    uint32_t  high() const { return m_high; }
    uint32_t  low() const { return m_low; }

    friend class ExprBuilder;
};
typedef std::shared_ptr<const ExtractExpr> ExtractExprPtr;

class ConcatExpr final : public BVExpr
{
  private:
    static const Kind ekind = Kind::CONCAT;

    BVExprPtr m_lhs;
    BVExprPtr m_rhs;
    size_t    m_size;

  protected:
    ConcatExpr(BVExprPtr lhs, BVExprPtr rhs)
        : m_lhs(lhs), m_rhs(rhs), m_size(lhs->size() + rhs->size())
    {
    }

  public:
    virtual const Kind kind() const { return ekind; };
    virtual size_t     size() const { return m_size; };
    virtual ExprPtr    clone() const
    {
        return ExprPtr(new ConcatExpr(m_lhs, m_rhs));
    }

    virtual uint64_t             hash() const;
    virtual bool                 eq(ExprPtr other) const;
    virtual std::string          to_string() const;
    virtual z3::expr             to_z3(z3::context& ctx) const;
    virtual std::vector<ExprPtr> children() const
    {
        return std::vector<ExprPtr>{m_lhs, m_rhs};
    }

    BVExprPtr lhs() const { return m_lhs; }
    BVExprPtr rhs() const { return m_rhs; }

    friend class ExprBuilder;
};
typedef std::shared_ptr<const ConcatExpr> ConcatExprPtr;

class ZextExpr final : public BVExpr
{
  private:
    static const Kind ekind = Kind::ZEXT;

    BVExprPtr m_expr;
    size_t    m_size;

  protected:
    ZextExpr(BVExprPtr expr, size_t size);

  public:
    virtual const Kind kind() const { return ekind; };
    virtual size_t     size() const { return m_size; };
    virtual ExprPtr    clone() const
    {
        return ExprPtr(new ZextExpr(m_expr, m_size));
    }

    virtual uint64_t             hash() const;
    virtual bool                 eq(ExprPtr other) const;
    virtual std::string          to_string() const;
    virtual z3::expr             to_z3(z3::context& ctx) const;
    virtual std::vector<ExprPtr> children() const
    {
        return std::vector<ExprPtr>{m_expr};
    }

    BVExprPtr expr() const { return m_expr; }

    friend class ExprBuilder;
};
typedef std::shared_ptr<const ZextExpr> ZextExprPtr;

class SextExpr final : public BVExpr
{
  private:
    static const Kind ekind = Kind::SEXT;

    BVExprPtr m_expr;
    size_t    m_size;

  protected:
    SextExpr(BVExprPtr expr, size_t size);

  public:
    virtual const Kind kind() const { return ekind; };
    virtual size_t     size() const { return m_size; };
    virtual ExprPtr    clone() const
    {
        return ExprPtr(new SextExpr(m_expr, m_size));
    }

    virtual uint64_t             hash() const;
    virtual bool                 eq(ExprPtr other) const;
    virtual std::string          to_string() const;
    virtual z3::expr             to_z3(z3::context& ctx) const;
    virtual std::vector<ExprPtr> children() const
    {
        return std::vector<ExprPtr>{m_expr};
    }

    BVExprPtr expr() const { return m_expr; }

    friend class ExprBuilder;
};
typedef std::shared_ptr<const SextExpr> SextExprPtr;

class NegExpr final : public BVExpr
{
  private:
    static const Kind ekind = Kind::NEG;

    BVExprPtr m_expr;
    size_t    m_size;

  protected:
    NegExpr(BVExprPtr expr) : m_expr(expr), m_size(m_expr->size()) {}

  public:
    virtual const Kind kind() const { return ekind; };
    virtual size_t     size() const { return m_size; };
    virtual ExprPtr    clone() const { return ExprPtr(new NegExpr(m_expr)); }

    virtual uint64_t             hash() const;
    virtual bool                 eq(ExprPtr other) const;
    virtual std::string          to_string() const;
    virtual z3::expr             to_z3(z3::context& ctx) const;
    virtual std::vector<ExprPtr> children() const
    {
        return std::vector<ExprPtr>{m_expr};
    }

    BVExprPtr expr() const { return m_expr; }

    friend class ExprBuilder;
};
typedef std::shared_ptr<const NegExpr> NegExprPtr;

class AddExpr final : public BVExpr
{
  private:
    static const Kind ekind = Kind::ADD;

    std::vector<BVExprPtr> m_children;
    size_t                 m_size;

  protected:
    AddExpr(std::vector<BVExprPtr> children)
        : m_children(children), m_size(children.at(0)->size())
    {
    }

  public:
    virtual const Kind kind() const { return ekind; };
    virtual size_t     size() const { return m_size; };
    virtual ExprPtr clone() const { return ExprPtr(new AddExpr(m_children)); }

    virtual uint64_t             hash() const;
    virtual bool                 eq(ExprPtr other) const;
    virtual std::string          to_string() const;
    virtual z3::expr             to_z3(z3::context& ctx) const;
    virtual std::vector<ExprPtr> children() const
    {
        std::vector<ExprPtr> res;
        for (auto c : m_children)
            res.push_back(std::static_pointer_cast<const Expr>(c));
        return res;
    }

    const std::vector<BVExprPtr>& addends() const { return m_children; }

    friend class ExprBuilder;
};
typedef std::shared_ptr<const AddExpr> AddExprPtr;

class BoolConst final : public BoolExpr
{
  private:
    static const Kind ekind = Kind::BOOL_CONST;

    bool m_is_true;

    BoolConst(bool is_true) : m_is_true(is_true) {}

  protected:
    static std::shared_ptr<const BoolConst> true_expr()
    {
        static auto v = std::shared_ptr<const BoolConst>(new BoolConst(1));
        return v;
    }

    static std::shared_ptr<const BoolConst> false_expr()
    {
        static auto v = std::shared_ptr<const BoolConst>(new BoolConst(0));
        return v;
    }

  public:
    virtual const Kind kind() const { return ekind; };
    virtual ExprPtr clone() const { return ExprPtr(new BoolConst(m_is_true)); }

    virtual uint64_t             hash() const;
    virtual bool                 eq(ExprPtr other) const;
    virtual std::string          to_string() const;
    virtual z3::expr             to_z3(z3::context& ctx) const;
    virtual std::vector<ExprPtr> children() const
    {
        return std::vector<ExprPtr>();
    }

    bool is_true() const { return m_is_true; }

    friend class ExprBuilder;
};
typedef std::shared_ptr<const BoolConst> BoolConstPtr;

class NotExpr final : public BoolExpr
{
  private:
    static const Kind ekind = Kind::NOT;

    BoolExprPtr m_expr;

  protected:
    NotExpr(BoolExprPtr expr) : m_expr(expr) {}

  public:
    virtual const Kind kind() const { return ekind; };
    virtual ExprPtr    clone() const { return ExprPtr(new NotExpr(m_expr)); }

    virtual uint64_t             hash() const;
    virtual bool                 eq(ExprPtr other) const;
    virtual std::string          to_string() const;
    virtual z3::expr             to_z3(z3::context& ctx) const;
    virtual std::vector<ExprPtr> children() const
    {
        return std::vector<ExprPtr>{m_expr};
    }

    BoolExprPtr expr() const { return m_expr; }

    friend class ExprBuilder;
};
typedef std::shared_ptr<const NotExpr> NotExprPtr;

class BoolAndExpr final : public BoolExpr
{
  private:
    static const Kind ekind = Kind::BOOL_AND;

    std::vector<BoolExprPtr> m_exprs;

  protected:
    BoolAndExpr(const std::set<BoolExprPtr>& exprs)
    {
        for (auto e : exprs)
            m_exprs.push_back(e);
    }

    BoolAndExpr(const std::vector<BoolExprPtr>& exprs) : m_exprs(exprs) {}

  public:
    virtual const Kind kind() const { return ekind; };
    virtual ExprPtr clone() const { return ExprPtr(new BoolAndExpr(m_exprs)); }

    virtual uint64_t             hash() const;
    virtual bool                 eq(ExprPtr other) const;
    virtual std::string          to_string() const;
    virtual z3::expr             to_z3(z3::context& ctx) const;
    virtual std::vector<ExprPtr> children() const
    {
        std::vector<ExprPtr> res;
        for (auto e : m_exprs)
            res.push_back(e);
        return res;
    }

    const std::vector<BoolExprPtr>& exprs() const { return m_exprs; }

    friend class ExprBuilder;
};
typedef std::shared_ptr<const BoolAndExpr> BoolAndExprPtr;

#define GEN_BINARY_LOGICAL_EXPR_CLASS(NAME, NAME_SHARED, KIND)                 \
    class NAME final : public BoolExpr                                         \
    {                                                                          \
      private:                                                                 \
        static const Kind ekind = KIND;                                        \
        BVExprPtr         m_lhs;                                               \
        BVExprPtr         m_rhs;                                               \
                                                                               \
      protected:                                                               \
        NAME(BVExprPtr lhs, BVExprPtr rhs) : m_lhs(lhs), m_rhs(rhs) {}         \
                                                                               \
      public:                                                                  \
        virtual const Kind kind() const { return ekind; };                     \
        virtual ExprPtr    clone() const                                       \
        {                                                                      \
            return ExprPtr(new NAME(m_lhs, m_rhs));                            \
        }                                                                      \
        virtual uint64_t             hash() const;                             \
        virtual bool                 eq(ExprPtr other) const;                  \
        virtual std::string          to_string() const;                        \
        virtual z3::expr             to_z3(z3::context& ctx) const;            \
        virtual std::vector<ExprPtr> children() const                          \
        {                                                                      \
            return std::vector<ExprPtr>{m_lhs, m_rhs};                         \
        }                                                                      \
        BVExprPtr lhs() const { return m_lhs; }                                \
        BVExprPtr rhs() const { return m_rhs; }                                \
        friend class ExprBuilder;                                              \
    };                                                                         \
    typedef std::shared_ptr<const NAME> NAME_SHARED;

GEN_BINARY_LOGICAL_EXPR_CLASS(UltExpr, UltExprPtr, Kind::ULT)
GEN_BINARY_LOGICAL_EXPR_CLASS(UleExpr, UleExprPtr, Kind::ULE)
GEN_BINARY_LOGICAL_EXPR_CLASS(UgtExpr, UgtExprPtr, Kind::UGT)
GEN_BINARY_LOGICAL_EXPR_CLASS(UgeExpr, UgeExprPtr, Kind::UGE)
GEN_BINARY_LOGICAL_EXPR_CLASS(SltExpr, SltExprPtr, Kind::SLT)
GEN_BINARY_LOGICAL_EXPR_CLASS(SleExpr, SleExprPtr, Kind::SLE)
GEN_BINARY_LOGICAL_EXPR_CLASS(SgtExpr, SgtExprPtr, Kind::SGT)
GEN_BINARY_LOGICAL_EXPR_CLASS(SgeExpr, SgeExprPtr, Kind::SGE)
GEN_BINARY_LOGICAL_EXPR_CLASS(EqExpr, EqExprPtr, Kind::EQ)

} // namespace naaz::expr
