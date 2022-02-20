#pragma once

#include <cstdint>
#include <memory>
#include <vector>

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
        EXTRACT,
        CONCAT,
        ZEXT,

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
        EQ
    };

    virtual const Kind kind() const = 0;
    virtual size_t     size() const = 0;

    virtual uint64_t    hash() const            = 0;
    virtual bool        eq(ExprPtr other) const = 0;
    virtual ExprPtr     clone() const           = 0;
    virtual std::string to_string() const       = 0;

    friend class ExprBuilder;
};

class SymExpr : public Expr
{
  private:
    static const Kind ekind = Kind::SYM;

    std::string m_name;
    size_t      m_size;

  protected:
    SymExpr(const std::string& name, size_t bits);

  public:
    virtual const Kind kind() const { return ekind; }
    virtual size_t     size() const { return m_size; }
    virtual ExprPtr    clone() const
    {
        return ExprPtr(new SymExpr(m_name, m_size));
    }

    virtual uint64_t    hash() const;
    virtual bool        eq(ExprPtr other) const;
    virtual std::string to_string() const;

    const std::string& name() const { return m_name; }

    friend class ExprBuilder;
};
typedef std::shared_ptr<const SymExpr> SymExprPtr;

class ConstExpr : public Expr
{
  private:
    static const Kind ekind = Kind::CONST;

    __uint128_t m_val;
    size_t      m_size;

  protected:
    ConstExpr(__uint128_t val, size_t size);

  public:
    virtual const Kind kind() const { return ekind; };
    virtual size_t     size() const { return m_size; };
    virtual ExprPtr    clone() const
    {
        return ExprPtr(new ConstExpr(m_val, m_size));
    }

    virtual uint64_t    hash() const;
    virtual bool        eq(ExprPtr other) const;
    virtual std::string to_string() const;

    __uint128_t val() const { return m_val; }
    __int128_t  sval() const;

    friend class ExprBuilder;
};
typedef std::shared_ptr<const ConstExpr> ConstExprPtr;

class ExtractExpr : public Expr
{
  private:
    static const Kind ekind = Kind::EXTRACT;

    ExprPtr  m_expr;
    uint32_t m_high;
    uint32_t m_low;

  protected:
    ExtractExpr(ExprPtr expr, uint32_t high, uint32_t low)
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

    virtual uint64_t    hash() const;
    virtual bool        eq(ExprPtr other) const;
    virtual std::string to_string() const;

    ExprPtr  expr() const { return m_expr; }
    uint32_t high() const { return m_high; }
    uint32_t low() const { return m_low; }

    friend class ExprBuilder;
};
typedef std::shared_ptr<const ExtractExpr> ExtractExprPtr;

class ConcatExpr : public Expr
{
  private:
    static const Kind ekind = Kind::CONCAT;

    ExprPtr m_lhs;
    ExprPtr m_rhs;
    size_t  m_size;

  protected:
    ConcatExpr(ExprPtr lhs, ExprPtr rhs)
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

    virtual uint64_t    hash() const;
    virtual bool        eq(ExprPtr other) const;
    virtual std::string to_string() const;

    ExprPtr lhs() const { return m_lhs; }
    ExprPtr rhs() const { return m_rhs; }

    friend class ExprBuilder;
};
typedef std::shared_ptr<const ConcatExpr> ConcatExprPtr;

class ZextExpr : public Expr
{
  private:
    static const Kind ekind = Kind::ZEXT;

    ExprPtr m_expr;
    size_t  m_size;

  protected:
    ZextExpr(ExprPtr expr, size_t size);

  public:
    virtual const Kind kind() const { return ekind; };
    virtual size_t     size() const { return m_size; };
    virtual ExprPtr    clone() const
    {
        return ExprPtr(new ZextExpr(m_expr, m_size));
    }

    virtual uint64_t    hash() const;
    virtual bool        eq(ExprPtr other) const;
    virtual std::string to_string() const;

    ExprPtr expr() const { return m_expr; }

    friend class ExprBuilder;
};
typedef std::shared_ptr<const ZextExpr> ZextExprPtr;

class NegExpr : public Expr
{
  private:
    static const Kind ekind = Kind::NEG;

    ExprPtr m_expr;
    size_t  m_size;

  protected:
    NegExpr(ExprPtr expr) : m_expr(expr), m_size(m_expr->size()) {}

  public:
    virtual const Kind kind() const { return ekind; };
    virtual size_t     size() const { return m_size; };
    virtual ExprPtr    clone() const { return ExprPtr(new NegExpr(m_expr)); }

    virtual uint64_t    hash() const;
    virtual bool        eq(ExprPtr other) const;
    virtual std::string to_string() const;

    ExprPtr expr() const { return m_expr; }

    friend class ExprBuilder;
};
typedef std::shared_ptr<const NegExpr> NegExprPtr;

class AddExpr : public Expr
{
  private:
    static const Kind ekind = Kind::ADD;

    std::vector<ExprPtr> m_children;
    size_t               m_size;

  protected:
    AddExpr(std::vector<ExprPtr> children)
        : m_children(children), m_size(children.at(0)->size())
    {
    }

  public:
    virtual const Kind kind() const { return ekind; };
    virtual size_t     size() const { return m_size; };
    virtual ExprPtr clone() const { return ExprPtr(new AddExpr(m_children)); }

    virtual uint64_t    hash() const;
    virtual bool        eq(ExprPtr other) const;
    virtual std::string to_string() const;

    const std::vector<ExprPtr>& children() const { return m_children; }

    friend class ExprBuilder;
};
typedef std::shared_ptr<const AddExpr> AddExprPtr;

class NotExpr : public Expr
{
  private:
    static const Kind ekind = Kind::NOT;

    ExprPtr m_expr;

  protected:
    NotExpr(ExprPtr expr);

  public:
    virtual const Kind kind() const { return ekind; };
    virtual size_t     size() const { return 1; };
    virtual ExprPtr    clone() const { return ExprPtr(new NotExpr(m_expr)); }

    virtual uint64_t    hash() const;
    virtual bool        eq(ExprPtr other) const;
    virtual std::string to_string() const;

    ExprPtr expr() const { return m_expr; }

    friend class ExprBuilder;
};
typedef std::shared_ptr<const NotExpr> NotExprPtr;

#define GEN_BINARY_LOGICAL_EXPR_CLASS(NAME, NAME_SHARED, KIND)                 \
    class NAME : public Expr                                                   \
    {                                                                          \
      private:                                                                 \
        static const Kind ekind = KIND;                                        \
        ExprPtr           m_lhs;                                               \
        ExprPtr           m_rhs;                                               \
                                                                               \
      protected:                                                               \
        NAME(ExprPtr lhs, ExprPtr rhs) : m_lhs(lhs), m_rhs(rhs) {}             \
                                                                               \
      public:                                                                  \
        virtual const Kind kind() const { return ekind; };                     \
        virtual size_t     size() const { return 1; };                         \
        virtual ExprPtr    clone() const                                       \
        {                                                                      \
            return ExprPtr(new NAME(m_lhs, m_rhs));                            \
        }                                                                      \
        virtual uint64_t    hash() const;                                      \
        virtual bool        eq(ExprPtr other) const;                           \
        virtual std::string to_string() const;                                 \
        ExprPtr             lhs() const { return m_lhs; }                      \
        ExprPtr             rhs() const { return m_rhs; }                      \
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
