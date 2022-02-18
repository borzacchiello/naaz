#pragma once

#include <cstdint>
#include <memory>

namespace naaz::expr
{

class Expr;
typedef std::shared_ptr<const Expr> ExprPtr;
typedef std::weak_ptr<const Expr>   WeakExprPtr;

class ExprBuilder;

class Expr
{
  public:
    enum Kind { SYM, CONST, EXTRACT, CONCAT, ADD, SUB };

    virtual const Kind kind() const = 0;
    virtual size_t     size() const = 0;

    virtual uint64_t hash() const            = 0;
    virtual bool     eq(ExprPtr other) const = 0;
    virtual void     pp() const              = 0;
    virtual ExprPtr  clone() const           = 0;

    friend class ExprBuilder;
};

class SymExpr : public Expr
{
  private:
    static const Kind ekind = Kind::SYM;

    std::string m_name;
    size_t      m_size;

  protected:
    SymExpr(const std::string& name, size_t bits) : m_name(name), m_size(bits)
    {
    }

  public:
    virtual const Kind kind() const { return ekind; }
    virtual size_t     size() const { return m_size; }
    virtual ExprPtr    clone() const
    {
        return ExprPtr(new SymExpr(m_name, m_size));
    }

    virtual uint64_t hash() const;
    virtual bool     eq(ExprPtr other) const;
    virtual void     pp() const;

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
    ConstExpr(__uint128_t val, size_t size) : m_val(val), m_size(size) {}

  public:
    virtual const Kind kind() const { return ekind; };
    virtual size_t     size() const { return m_size; };
    virtual ExprPtr    clone() const
    {
        return ExprPtr(new ConstExpr(m_val, m_size));
    }

    virtual uint64_t hash() const;
    virtual bool     eq(ExprPtr other) const;
    virtual void     pp() const;

    __uint128_t val() const { return m_val; }

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

    virtual uint64_t hash() const;
    virtual bool     eq(ExprPtr other) const;
    virtual void     pp() const;

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

    virtual uint64_t hash() const;
    virtual bool     eq(ExprPtr other) const;
    virtual void     pp() const;

    ExprPtr lhs() const { return m_lhs; }
    ExprPtr rhs() const { return m_rhs; }

    friend class ExprBuilder;
};
typedef std::shared_ptr<const ConcatExpr> ConcatExprPtr;

class AddExpr : public Expr
{
  private:
    static const Kind ekind = Kind::ADD;

    ExprPtr m_lhs;
    ExprPtr m_rhs;
    size_t  m_size;

  protected:
    AddExpr(ExprPtr lhs, ExprPtr rhs)
        : m_lhs(lhs), m_rhs(rhs), m_size(lhs->size())
    {
    }

  public:
    virtual const Kind kind() const { return ekind; };
    virtual size_t     size() const { return m_size; };
    virtual ExprPtr clone() const { return ExprPtr(new AddExpr(m_lhs, m_rhs)); }

    virtual uint64_t hash() const;
    virtual bool     eq(ExprPtr other) const;
    virtual void     pp() const;

    ExprPtr lhs() const { return m_lhs; }
    ExprPtr rhs() const { return m_rhs; }

    friend class ExprBuilder;
};
typedef std::shared_ptr<const AddExpr> AddExprPtr;

class SubExpr : public Expr
{
  private:
    static const Kind ekind = Kind::SUB;

    ExprPtr m_lhs;
    ExprPtr m_rhs;
    size_t  m_size;

  protected:
    SubExpr(ExprPtr lhs, ExprPtr rhs)
        : m_lhs(lhs), m_rhs(rhs), m_size(lhs->size())
    {
    }

  public:
    virtual const Kind kind() const { return ekind; };
    virtual size_t     size() const { return m_size; };
    virtual ExprPtr clone() const { return ExprPtr(new SubExpr(m_lhs, m_rhs)); }

    virtual uint64_t hash() const;
    virtual bool     eq(ExprPtr other) const;
    virtual void     pp() const;

    ExprPtr lhs() const { return m_lhs; }
    ExprPtr rhs() const { return m_rhs; }

    friend class ExprBuilder;
};
typedef std::shared_ptr<const SubExpr> SubExprPtr;

} // namespace naaz::expr
