extern "C" {

/*
   Copyright (C) 1995-2018 Free Software Foundation, Inc.

   The GNU C Library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Lesser General Public
   License as published by the Free Software Foundation; either
   version 2.1 of the License, or (at your option) any later version.

   The GNU C Library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Lesser General Public License for more details.

   You should have received a copy of the GNU Lesser General Public
   License along with the GNU C Library; if not, see
   <http://www.gnu.org/licenses/>.  */

/*
   Copyright (C) 1983 Regents of the University of California.
   All rights reserved.

   Redistribution and use in source and binary forms, with or without
   modification, are permitted provided that the following conditions
   are met:

   1. Redistributions of source code must retain the above copyright
      notice, this list of conditions and the following disclaimer.
   2. Redistributions in binary form must reproduce the above copyright
      notice, this list of conditions and the following disclaimer in the
      documentation and/or other materials provided with the distribution.
   4. Neither the name of the University nor the names of its contributors
      may be used to endorse or promote products derived from this software
      without specific prior written permission.

   THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
   ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
   IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
   ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
   FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
   DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
   OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
   HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
   LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
   OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
   SUCH DAMAGE.*/

/*
 * This is derived from the Berkeley source:
 *	@(#)random.c	5.5 (Berkeley) 7/6/88
 * It was reworked for the GNU C Library by Roland McGrath.
 * Rewritten to be reentrant by Ulrich Drepper, 1995
 */

#include <limits.h>
#include <stddef.h>
#include <stdlib.h>
#include <stdint.h>

/* Linear congruential.  */
#define TYPE_0  0
#define BREAK_0 8
#define DEG_0   0
#define SEP_0   0

/* x**7 + x**3 + 1.  */
#define TYPE_1  1
#define BREAK_1 32
#define DEG_1   7
#define SEP_1   3

/* x**15 + x + 1.  */
#define TYPE_2  2
#define BREAK_2 64
#define DEG_2   15
#define SEP_2   1

/* x**31 + x**3 + 1.  */
#define TYPE_3  3
#define BREAK_3 128
#define DEG_3   31
#define SEP_3   3

/* x**63 + x + 1.  */
#define TYPE_4  4
#define BREAK_4 256
#define DEG_4   63
#define SEP_4   1

/* Array versions of the above information to make code run faster.
   Relies on fact that TYPE_i == i.  */

#define MAX_TYPES 5 /* Max number of types above.  */

struct random_poly_info {
    int seps[MAX_TYPES];
    int degrees[MAX_TYPES];
};

static const struct random_poly_info random_poly_info = {
    {SEP_0, SEP_1, SEP_2, SEP_3, SEP_4}, {DEG_0, DEG_1, DEG_2, DEG_3, DEG_4}};

static int __random_r(struct random_data* buf, int32_t* result)
{
    int32_t* state;

    if (buf == NULL || result == NULL)
        return -1;

    state = buf->state;

    if (buf->rand_type == TYPE_0) {
        int32_t val = ((state[0] * 1103515245U) + 12345U) & 0x7fffffff;
        state[0]    = val;
        *result     = val;
    } else {
        int32_t* fptr    = buf->fptr;
        int32_t* rptr    = buf->rptr;
        int32_t* end_ptr = buf->end_ptr;
        uint32_t val;

        val = *fptr += (uint32_t)*rptr;
        /* Chucking least random bit.  */
        *result = val >> 1;
        ++fptr;
        if (fptr >= end_ptr) {
            fptr = state;
            ++rptr;
        } else {
            ++rptr;
            if (rptr >= end_ptr)
                rptr = state;
        }
        buf->fptr = fptr;
        buf->rptr = rptr;
    }
    return 0;
}

static int __srandom_r(unsigned int seed, struct random_data* buf)
{
    int      type;
    int32_t* state;
    long int i;
    int32_t  word;
    int32_t* dst;
    int      kc;

    if (buf == NULL)
        return -1;
    type = buf->rand_type;
    if ((unsigned int)type >= MAX_TYPES)
        return -1;

    state = buf->state;
    /* We must make sure the seed is not 0.  Take arbitrarily 1 in this case. */
    if (seed == 0)
        seed = 1;
    state[0] = seed;
    if (type == TYPE_0)
        return 0;

    dst  = state;
    word = seed;
    kc   = buf->rand_deg;
    for (i = 1; i < kc; ++i) {
        /* This does:
         state[i] = (16807 * state[i - 1]) % 2147483647;
       but avoids overflowing 31 bits.  */
        long int hi = word / 127773;
        long int lo = word % 127773;
        word        = 16807 * lo - 2836 * hi;
        if (word < 0)
            word += 2147483647;
        *++dst = word;
    }

    buf->fptr = &state[buf->rand_sep];
    buf->rptr = &state[0];
    kc *= 10;
    while (--kc >= 0) {
        int32_t discard;
        (void)__random_r(buf, &discard);
    }

    return 0;
}

static int __initstate_r(unsigned int seed, char* arg_state, size_t n,
                         struct random_data* buf)
{
    if (buf == NULL)
        return -1;

    int32_t* old_state = buf->state;
    if (old_state != NULL) {
        int old_type = buf->rand_type;
        if (old_type == TYPE_0)
            old_state[-1] = TYPE_0;
        else
            old_state[-1] = (MAX_TYPES * (buf->rptr - old_state)) + old_type;
    }

    int type;
    if (n >= BREAK_3)
        type = n < BREAK_4 ? TYPE_3 : TYPE_4;
    else if (n < BREAK_1) {
        if (n < BREAK_0)
            return -1;

        type = TYPE_0;
    } else
        type = n < BREAK_2 ? TYPE_1 : TYPE_2;

    int degree     = random_poly_info.degrees[type];
    int separation = random_poly_info.seps[type];

    buf->rand_type = type;
    buf->rand_sep  = separation;
    buf->rand_deg  = degree;
    int32_t* state = &((int32_t*)arg_state)[1]; /* First location.  */
    /* Must set END_PTR before srandom.  */
    buf->end_ptr = &state[degree];

    buf->state = state;

    __srandom_r(seed, buf);

    state[-1] = TYPE_0;
    if (type != TYPE_0)
        state[-1] = (buf->rptr - state) * MAX_TYPES + type;

    return 0;
}

int __setstate_r(char* arg_state, struct random_data* buf)
{
    int32_t* new_state = 1 + (int32_t*)arg_state;
    int      type;
    int      old_type;
    int32_t* old_state;
    int      degree;
    int      separation;

    if (arg_state == NULL || buf == NULL)
        return -1;

    old_type  = buf->rand_type;
    old_state = buf->state;
    if (old_type == TYPE_0)
        old_state[-1] = TYPE_0;
    else
        old_state[-1] = (MAX_TYPES * (buf->rptr - old_state)) + old_type;

    type = new_state[-1] % MAX_TYPES;
    if (type < TYPE_0 || type > TYPE_4)
        return -1;

    buf->rand_deg = degree = random_poly_info.degrees[type];
    buf->rand_sep = separation = random_poly_info.seps[type];
    buf->rand_type             = type;

    if (type != TYPE_0) {
        int rear  = new_state[-1] / MAX_TYPES;
        buf->rptr = &new_state[rear];
        buf->fptr = &new_state[(rear + separation) % degree];
    }
    buf->state = new_state;
    /* Set end_ptr too.  */
    buf->end_ptr = &new_state[degree];

    return 0;
}

static int32_t randtbl[DEG_3 + 1] = {
    TYPE_3,

    -1726662223, 379960547,   1735697613,  1040273694,  1313901226, 1627687941,
    -179304937,  -2073333483, 1780058412,  -1989503057, -615974602, 344556628,
    939512070,   -1249116260, 1507946756,  -812545463,  154635395,  1388815473,
    -1926676823, 525320961,   -1009028674, 968117788,   -123449607, 1284210865,
    435012392,   -2017506339, -911064859,  -370259173,  1132637927, 1398500161,
    -205601318,
};

static struct random_data state_template = {
    .fptr = &randtbl[SEP_3 + 1],
    .rptr = &randtbl[1],

    .state = &randtbl[1],

    .rand_type = TYPE_3,
    .rand_deg  = DEG_3,
    .rand_sep  = SEP_3,

    .end_ptr = &randtbl[sizeof(randtbl) / sizeof(randtbl[0])]};
}

#include "directory.hpp"
#include "../../expr/Expr.hpp"
#include "../../expr/ExprBuilder.hpp"
#include "../../state/State.hpp"
#include "../../state/PluginManager.hpp"
#include "../../util/ioutil.hpp"

#define exprBuilder expr::ExprBuilder::The()

namespace naaz::models::libc
{

class RandPlugin final : public state::Plugin
{
    struct random_data m_state;

  public:
    RandPlugin() { m_state = state_template; }
    RandPlugin(const RandPlugin& other) { m_state = other.m_state; }

    struct random_data* state() { return &m_state; }

    virtual const std::string& name()
    {
        static std::string g_name = "libc::rand";
        return g_name;
    }
    virtual std::shared_ptr<Plugin> clone()
    {
        return std::make_shared<RandPlugin>(*this);
    }
};

void srand::exec(state::StatePtr           s,
                 executor::ExecutorResult& o_successors) const
{
    auto seed = s->get_int_param(m_call_conv, 0);

    expr::BVConst const_seed;
    if (seed->kind() != expr::Expr::Kind::CONST) {
        auto const_seed_opt = s->solver().evaluate(seed);
        if (!const_seed_opt.has_value())
            throw executor::UnsatStateException();
        const_seed = const_seed_opt.value();
    } else
        const_seed =
            std::static_pointer_cast<const expr::ConstExpr>(seed)->val();

    state::PluginPtr pl = s->pm().get_plugin("libc::rand");
    if (pl == nullptr) {
        pl = RandPlugin().clone();
    }

    struct random_data* state =
        std::static_pointer_cast<RandPlugin>(pl)->state();

    if (__srandom_r(const_seed.as_u64(), state) < 0) {
        err("srand") << "failed __srandom_r" << std::endl;
        exit_fail();
    }

    s->arch().handle_return(s, o_successors);
}

void rand::exec(state::StatePtr s, executor::ExecutorResult& o_successors) const
{
    state::PluginPtr pl = s->pm().get_plugin("libc::rand");
    if (pl == nullptr) {
        pl = RandPlugin().clone();
    }

    struct random_data* state =
        std::static_pointer_cast<RandPlugin>(pl)->state();

    int32_t res;
    if (__random_r(state, &res) < 0) {
        err("rand") << "failed __random_r" << std::endl;
        exit_fail();
    }

    s->arch().set_return_int_value(m_call_conv, *s,
                                   exprBuilder.mk_const(res, 32));
    s->arch().handle_return(s, o_successors);
}

} // namespace naaz::models::libc
