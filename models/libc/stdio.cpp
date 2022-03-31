#include "directory.hpp"

#include "../../expr/Expr.hpp"
#include "../../expr/ExprBuilder.hpp"
#include "../../state/State.hpp"
#include "../../util/ioutil.hpp"
#include "../../util/strutil.hpp"
#include "string_utils.hpp"

#define exprBuilder expr::ExprBuilder::The()

namespace naaz::models::libc
{

void puts::exec(state::StatePtr s, executor::ExecutorResult& o_successors) const
{
    auto str = s->get_int_param(m_call_conv, 0);

    if (str->kind() != expr::Expr::Kind::CONST) {
        err("puts") << "str pointer is symbolic" << std::endl;
        exit_fail();
    }

    auto str_addr =
        std::static_pointer_cast<const expr::ConstExpr>(str)->val().as_u64();
    auto resolved_strings = resolve_string(s, str_addr, 0, 256);
    if (resolved_strings.size() != 1) {
        err("puts") << "unable to resolve the string" << std::endl;
        exit_fail();
    }

    auto data = resolved_strings.at(0).str;
    data      = exprBuilder.mk_concat(data, exprBuilder.mk_const('\n', 8));
    s->fs().write(1, data);
    s->arch().handle_return(s, o_successors);
}

#define FLAGS_ZEROPAD        (1)
#define FLAGS_LEFT           (1 << 1)
#define FLAGS_PLUS           (1 << 2)
#define FLAGS_SPACE          (1 << 3)
#define FLAGS_HASH           (1 << 4)
#define FLAGS_WIDTH_AS_PARAM (1 << 5)
#define FLAGS_PREC_AS_PARAM  (1 << 6)
#define FLAGS_CHAR           (1 << 7)
#define FLAGS_SHORT          (1 << 8)
#define FLAGS_LONG           (1 << 9)
#define FLAGS_LONG_LONG      (1 << 10)
#define FLAGS_HAS_WIDTH      (1 << 11)
#define FLAGS_HAS_PRECISION  (1 << 12)

struct format_token_t {
    bool        is_specifier;
    int         flags     = 0;
    unsigned    width     = 0;
    unsigned    precision = 0;
    char        specifier = 0;
    std::string str;
};

// internal test if char is a digit (0-9)
// \return true if char is a digit
static inline bool _is_digit(char ch) { return (ch >= '0') && (ch <= '9'); }

// internal ASCII string to unsigned int conversion
static unsigned int _atoi(const char** str)
{
    unsigned int i = 0U;
    while (_is_digit(**str)) {
        i = i * 10U + (unsigned int)(*((*str)++) - '0');
    }
    return i;
}

static std::vector<format_token_t> process_tokens(const std::string& format)
{
    std::vector<format_token_t> res;

    int i = 0;
    while (i < format.size()) {
        // format specifier: %[flags][width][.precision][length]

        format_token_t ft;
        if (format[i] == '%') {
            ft.is_specifier = true;

            ++i;
            const char* needle = &format[i];

            // evaluate flags
            unsigned n;
            do {
                switch (*needle) {
                    case '0':
                        ft.flags |= FLAGS_ZEROPAD;
                        needle++;
                        n = 1U;
                        break;
                    case '-':
                        ft.flags |= FLAGS_LEFT;
                        needle++;
                        n = 1U;
                        break;
                    case '+':
                        ft.flags |= FLAGS_PLUS;
                        needle++;
                        n = 1U;
                        break;
                    case ' ':
                        ft.flags |= FLAGS_SPACE;
                        needle++;
                        n = 1U;
                        break;
                    case '#':
                        ft.flags |= FLAGS_HASH;
                        needle++;
                        n = 1U;
                        break;
                    default:
                        n = 0U;
                        break;
                }
            } while (n);

            // evaluate width field
            if (_is_digit(*needle)) {
                ft.flags |= FLAGS_HAS_WIDTH;
                ft.width = _atoi(&needle);
            } else if (*needle == '*') {
                ft.flags |= FLAGS_HAS_WIDTH;
                ft.flags |= FLAGS_WIDTH_AS_PARAM;
                needle++;
            }

            // evaluate precision field
            if (*needle == '.') {
                ft.flags |= FLAGS_HAS_PRECISION;
                needle++;
                if (_is_digit(*needle)) {
                    ft.precision = _atoi(&needle);
                } else if (*needle == '*') {
                    ft.flags |= FLAGS_PREC_AS_PARAM;
                    needle++;
                }
            }

            // evaluate length field
            switch (*needle) {
                case 'l':
                    ft.flags |= FLAGS_LONG;
                    needle++;
                    if (*needle == 'l') {
                        ft.flags |= FLAGS_LONG_LONG;
                        needle++;
                    }
                    break;
                case 'h':
                    ft.flags |= FLAGS_SHORT;
                    needle++;
                    if (*needle == 'h') {
                        ft.flags |= FLAGS_CHAR;
                        needle++;
                    }
                    break;
                case 't':
                    ft.flags |=
                        (sizeof(ptrdiff_t) == sizeof(long) ? FLAGS_LONG
                                                           : FLAGS_LONG_LONG);
                    needle++;
                    break;
                case 'j':
                    ft.flags |=
                        (sizeof(intmax_t) == sizeof(long) ? FLAGS_LONG
                                                          : FLAGS_LONG_LONG);
                    needle++;
                    break;
                case 'z':
                    ft.flags |=
                        (sizeof(size_t) == sizeof(long) ? FLAGS_LONG
                                                        : FLAGS_LONG_LONG);
                    needle++;
                    break;
                default:
                    break;
            }

            // evaluate specifier
            switch (*needle) {
                case 'd':
                case 'i':
                case 'u':
                case 'x':
                case 'X':
                case 'o':
                case 'b':
                case 'f':
                case 'F':
                case 'E':
                case 'g':
                case 'G':
                case 'c':
                case 's':
                case 'p':
                case '%':
                    ft.specifier = *needle;
                    needle++;
                    break;
                default:
                    // not a specifier...
                    ft.is_specifier = false;
                    ft.str          = format.substr(i, needle - &format[i]);
                    break;
            }

            i += needle - &format[i];
            res.push_back(ft);
        } else {

            ft.is_specifier = false;
            char c          = 0;
            while (i < format.size()) {
                c = format[i++];
                if (c == '%') {
                    i--;
                    break;
                }
                ft.str += c;
            }
            res.push_back(ft);
        }
    }

    return res;
}

static expr::BVExprPtr process_specifier(const format_token_t& ft,
                                         state::StatePtr s, expr::BVExprPtr v)
{
    // TODO: it implements only a very small portion of the specifiers (and
    // flags)

    if (!ft.is_specifier) {
        err("process_specifier")
            << "the input token is not a specifier" << std::endl;
        exit_fail();
    }

    switch (ft.specifier) {
        case '%':
            return exprBuilder.mk_const(expr::BVConst((const uint8_t*)"%", 1));
        case 's': {
            if (v->kind() != expr::Expr::Kind::CONST) {
                err("process_specifier")
                    << "\"%s\": symbolic pointer" << std::endl;
                exit_fail();
            }
            auto v_ = std::static_pointer_cast<const expr::ConstExpr>(v);
            auto resolved_strings = resolve_string(
                s, v_->val().as_u64(), 0,
                (ft.flags & FLAGS_HAS_PRECISION) ? ft.precision : 256);
            if (resolved_strings.size() != 1) {
                err("process_specifier")
                    << "\"%s\": unable to resolve the string" << std::endl;
                exit_fail();
            }

            auto str = resolved_strings.at(0).str;
            if (str->size() == 8)
                // In the case in which the string is empty, we return a space
                // (since we cannot construct an empty BV!)
                // Its fine for now
                return exprBuilder.mk_const(
                    expr::BVConst((const uint8_t*)" ", 1));

            str = exprBuilder.mk_extract(str, str->size() - 1, 8);
            return str;
        }
        case 'd': {
            expr::BVExprPtr expr;
            if (ft.flags & FLAGS_LONG || ft.flags & FLAGS_LONG_LONG)
                expr = v;
            else
                expr = exprBuilder.mk_extract(v, 31, 0);

            expr::BVConst const_expr;
            if (expr->kind() != expr::Expr::Kind::CONST) {
                // FIXME: do not concretize (maybe)
                const_expr = s->solver().evaluate(expr);
            } else {
                const_expr =
                    std::static_pointer_cast<const expr::ConstExpr>(expr)
                        ->val();
            }
            int64_t v     = const_expr.as_s64();
            auto    v_str = std::to_string(v);
            return exprBuilder.mk_const(
                expr::BVConst((const uint8_t*)v_str.data(), v_str.size()));
        }
        default:
            break;
    }

    err("parse_specifier") << "unsupported specifier \"" << ft.str << "\""
                           << std::endl;
    exit_fail();
}

static expr::BVExprPtr process_format_string(const std::string& format,
                                             state::StatePtr    s,
                                             int                arg_offset = 1)
{
    expr::BVExprPtr data   = nullptr;
    auto            tokens = process_tokens(format);

    int specifier_idx = arg_offset;
    for (auto& token : tokens) {
        if (!token.is_specifier) {
            auto token_bv = exprBuilder.mk_const(expr::BVConst(
                (const uint8_t*)token.str.data(), token.str.size()));
            data = data ? exprBuilder.mk_concat(data, token_bv) : token_bv;
        } else {
            if (token.flags & FLAGS_WIDTH_AS_PARAM) {
                auto width = s->get_int_param(CallConv::CDECL, specifier_idx++);
                if (width->kind() != expr::Expr::Kind::CONST) {
                    err("printf") << "unsupported symbolic width" << std::endl;
                    exit_fail();
                }
                auto width_ =
                    std::static_pointer_cast<const expr::ConstExpr>(width);
                token.width = width_->val().as_u64();
            }
            if (token.flags & FLAGS_PREC_AS_PARAM) {
                auto prec = s->get_int_param(CallConv::CDECL, specifier_idx++);
                if (prec->kind() != expr::Expr::Kind::CONST) {
                    err("printf") << "unsupported symbolic prec" << std::endl;
                    exit_fail();
                }
                auto prec_ =
                    std::static_pointer_cast<const expr::ConstExpr>(prec);
                token.precision = prec_->val().as_u64();
            }

            // FIXME: it can be a float!
            auto op_val   = s->get_int_param(CallConv::CDECL, specifier_idx++);
            auto token_bv = process_specifier(token, s, op_val);
            data = data ? exprBuilder.mk_concat(data, token_bv) : token_bv;
        }
    }

    if (!data) {
        err("printf") << "no tokens" << std::endl;
        exit_fail();
    }
    return data;
}

void printf::exec(state::StatePtr           s,
                  executor::ExecutorResult& o_successors) const
{
    auto format_str = s->get_int_param(m_call_conv, 0);
    if (format_str->kind() != expr::Expr::Kind::CONST) {
        err("printf") << "format string pointer is symbolic" << std::endl;
        exit_fail();
    }

    auto format_str_addr =
        std::static_pointer_cast<const expr::ConstExpr>(format_str)
            ->val()
            .as_u64();
    auto resolved_strings = resolve_string(s, format_str_addr, 0, 256);
    if (resolved_strings.size() != 1) {
        err("printf") << "unable to resolve the string" << std::endl;
        exit_fail();
    }

    auto resolved_format = resolved_strings.at(0);
    if (resolved_format.str->kind() != expr::Expr::Kind::CONST) {
        err("printf") << "the format string is symbolic" << std::endl;
        exit_fail();
    }

    auto format_bytes =
        std::static_pointer_cast<const expr::ConstExpr>(resolved_format.str)
            ->val()
            .as_data();
    std::string format((char*)format_bytes.data(), format_bytes.size());

    auto processed_format = process_format_string(format, s);

    auto second_param = s->arch().get_int_param(CallConv::CDECL, *s, 1);
    if (second_param->kind() == expr::Expr::Kind::CONST) {
        auto a = s->read_buf(second_param, 44);
    }

    s->fs().write(1, processed_format);
    s->arch().handle_return(s, o_successors);
}

void sprintf::exec(state::StatePtr           s,
                   executor::ExecutorResult& o_successors) const
{
    auto str_ptr = s->get_int_param(m_call_conv, 0);
    if (str_ptr->kind() != expr::Expr::Kind::CONST) {
        err("sprintf") << "str_ptr is symbolic" << std::endl;
        exit_fail();
    }

    auto str_ptr_addr = std::static_pointer_cast<const expr::ConstExpr>(str_ptr)
                            ->val()
                            .as_u64();

    auto format_str = s->get_int_param(m_call_conv, 1);
    if (format_str->kind() != expr::Expr::Kind::CONST) {
        err("sprintf") << "format string pointer is symbolic" << std::endl;
        exit_fail();
    }

    auto format_str_addr =
        std::static_pointer_cast<const expr::ConstExpr>(format_str)
            ->val()
            .as_u64();
    auto resolved_strings = resolve_string(s, format_str_addr, 0, 256);
    if (resolved_strings.size() != 1) {
        err("sprintf") << "unable to resolve the string" << std::endl;
        exit_fail();
    }

    auto resolved_format = resolved_strings.at(0);
    if (resolved_format.str->kind() != expr::Expr::Kind::CONST) {
        err("sprintf") << "the format string is symbolic" << std::endl;
        exit_fail();
    }

    auto format_bytes =
        std::static_pointer_cast<const expr::ConstExpr>(resolved_format.str)
            ->val()
            .as_data();
    std::string format((char*)format_bytes.data(), format_bytes.size());

    auto format_processed = process_format_string(format, s, 2);
    s->write_buf(str_ptr_addr, format_processed);
    s->arch().handle_return(s, o_successors);
}

} // namespace naaz::models::libc
