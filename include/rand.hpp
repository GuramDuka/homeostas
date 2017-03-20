/*-
 * The MIT License (MIT)
 *
 * Copyright (c) 2017 Guram Duka
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */
//------------------------------------------------------------------------------
/*

    C++ TEMPLATE VERSION OF Robert J. Jenkins Jr.'s
    ISAAC Random Number Generator.

    Ported from vanilla C to to template C++ class
    by Quinn Tyler Jackson on 16-23 July 1998.

        quinn@qtj.net

    The function for the expected period of this
    random number generator, according to Jenkins is:

        f(a,b) = 2**((a+b*(3+2^^a)-1)

        (where a is ALPHA and b is bitwidth)

    So, for a bitwidth of 32 and an ALPHA of 8,
    the expected period of ISAAC is:

        2^^(8+32*(3+2^^8)-1) = 2^^8295

    Jackson has been able to run implementations
    with an ALPHA as high as 16, or

        2^^2097263

 * Modified in 2017 by Guram Duka
*/
//---------------------------------------------------------------------------
#ifndef RAND_HPP_INCLUDED
#define RAND_HPP_INCLUDED
//------------------------------------------------------------------------------
#pragma once
//------------------------------------------------------------------------------
#include "config.h"
//------------------------------------------------------------------------------
namespace spacenet {
//------------------------------------------------------------------------------
////////////////////////////////////////////////////////////////////////////////
//------------------------------------------------------------------------------
template <int ALPHA = 8, class T = uint32_t>
class rand {
public:
    typedef uint8_t byte;
    enum { N = (1 << ALPHA) };

    T get() {
        if( rc_.randcnt == 0 ){
            isaac(&rc_, T());
            rc_.randcnt = N;
        }

        return rc_.randrsl[--rc_.randcnt];
    }

    void srand(T a = 0, T b = 0, T c = 0) {
        for( int i = 0; i < N; i++ )
            rc_.randrsl[i] = 0;

        rc_.randa = a;
        rc_.randb = b;
        rc_.randc = c;

        randinit(&rc_, true);
    }

    void srand(const void * data, size_t count) {
        auto l = count < sizeof(rc_.randrsl) ? count : sizeof(rc_.randrsl);
        memcpy(rc_.randrsl, data, l);
        memset(rc_.randrsl, 0, sizeof(rc_.randrsl) - l);

        rc_.randa = rc_.randrsl[0];
        rc_.randb = rc_.randrsl[1];
        rc_.randc = rc_.randrsl[2];

        shuffle(rc_.randa, rc_.randb, rc_.randc, rc_.randrsl[0], rc_.randrsl[1], rc_.randrsl[2], rc_.randrsl[3], rc_.randrsl[4]);

        randinit(&rc_, true);
    }

protected:
    struct randctx {
        T randrsl[N];
        T randmem[N];
        T randa;
        T randb;
        T randc;
        int randcnt;
    } rc_;

    void randinit(randctx * ctx, bool bUseSeed) {
        T a, b, c, d, e, f, g, h;

        a = b = c = d = e = f = g = h = GOLDEN_RATIO(T());

        T * m = ctx->randmem;
        T * r = ctx->randrsl;

        if( !bUseSeed ) {
            ctx->randa = 0;
            ctx->randb = 0;
            ctx->randc = 0;
        }

        // scramble it
        for( int i = 0; i < 4; ++i )
            shuffle(a, b, c, d, e, f, g, h);

        if( bUseSeed ){
            // initialize using the contents of r[] as the seed

            for( int i = 0; i < N; i += 8 ){
                a+=r[i  ]; b+=r[i+1]; c+=r[i+2]; d+=r[i+3];
                e+=r[i+4]; f+=r[i+5]; g+=r[i+6]; h+=r[i+7];

                shuffle(a, b, c, d, e, f, g, h);

                m[i  ]=a; m[i+1]=b; m[i+2]=c; m[i+3]=d;
                m[i+4]=e; m[i+5]=f; m[i+6]=g; m[i+7]=h;
            }

            //do a second pass to make all of the seed affect all of m

            for( int i = 0; i < N; i += 8 ){
                a+=m[i  ]; b+=m[i+1]; c+=m[i+2]; d+=m[i+3];
                e+=m[i+4]; f+=m[i+5]; g+=m[i+6]; h+=m[i+7];

                shuffle(a, b, c, d, e, f, g, h);

                m[i  ]=a; m[i+1]=b; m[i+2]=c; m[i+3]=d;
                m[i+4]=e; m[i+5]=f; m[i+6]=g; m[i+7]=h;
            }
        }
        else {
            // fill in mm[] with messy stuff
            for( int i = 0; i < N; i += 8 ) {
                shuffle(a, b, c, d, e, f, g, h);

                m[i  ]=a; m[i+1]=b; m[i+2]=c; m[i+3]=d;
                m[i+4]=e; m[i+5]=f; m[i+6]=g; m[i+7]=h;
            }
        }

        isaac(ctx, T());    // fill in the first set of results
        ctx->randcnt = N;   // prepare to use the first set of results
    }

    void rngstep(T mix, T & a, T & b, T * & mm, T * & m, T * & m2, T * & r, T & x, T & y) {
        x = *m;
        a = (a^(mix)) + *(m2++);
        *(m++) = y = ind(mm,x) + a + b;
        *(r++) = b = ind(mm,y>>ALPHA) + x;
    }

    uint32_t ind(uint32_t * mm, uint32_t x) {
        return *(uint32_t *) ((byte *) mm + (x & ((N - 1) << 2)));
    }

    uint64_t ind(uint64_t * mm, uint64_t x) {
        return *(uint64_t *)((byte *) mm + (x & ((N - 1) << 3)));
    }

    const T GOLDEN_RATIO(uint32_t) {
        return uint32_t(0x9e3779b9);
    }

    const T GOLDEN_RATIO(uint64_t) {
        return uint64_t(0x9e3779b97f4a7c13ull);
    }

    void isaac(randctx * ctx, uint32_t) {
        T x, y;

        T * mm = ctx->randmem;
        T * r  = ctx->randrsl;

        T a = ctx->randa;
        T b = ctx->randb + ++ctx->randc;

        T * m    = mm;
        T * m2   = m + (N / 2);
        T * mend = m2;

        for(; m<mend; ){
            rngstep((a<<13), a, b, mm, m, m2, r, x, y);
            rngstep((a>>6) , a, b, mm, m, m2, r, x, y);
            rngstep((a<<2) , a, b, mm, m, m2, r, x, y);
            rngstep((a>>16), a, b, mm, m, m2, r, x, y);
        }

        m2 = mm;

        for(; m2<mend; ){
            rngstep((a<<13), a, b, mm, m, m2, r, x, y);
            rngstep((a>>6) , a, b, mm, m, m2, r, x, y);
            rngstep((a<<2) , a, b, mm, m, m2, r, x, y);
            rngstep((a>>16), a, b, mm, m, m2, r, x, y);
        }

        ctx->randb = b;
        ctx->randa = a;
    }

    void isaac(randctx * ctx, uint64_t){
        T x, y;

        T * mm = ctx->randmem;
        T * r  = ctx->randrsl;

        T a = ctx->randa;
        T b = ctx->randb + ++ctx->randc;

        T * m    = mm;
        T * m2   = m + (N / 2);
        T * mend = m2;

        for(; m<mend; ){
            rngstep(~(a^(a<<21)), a, b, mm, m, m2, r, x, y);
            rngstep(  a^(a>>5)  , a, b, mm, m, m2, r, x, y);
            rngstep(  a^(a<<12) , a, b, mm, m, m2, r, x, y);
            rngstep(  a^(a>>33) , a, b, mm, m, m2, r, x, y);
        }

        m2 = mm;

        for(; m2<mend; ){
            rngstep(~(a^(a<<21)), a, b, mm, m, m2, r, x, y);
            rngstep(  a^(a>>5)  , a, b, mm, m, m2, r, x, y);
            rngstep(  a^(a<<12) , a, b, mm, m, m2, r, x, y);
            rngstep(  a^(a>>33) , a, b, mm, m, m2, r, x, y);
        }

        ctx->randb = b;
        ctx->randa = a;
    }

    void shuffle(
            uint32_t & a, uint32_t & b,
            uint32_t & c, uint32_t & d,
            uint32_t & e, uint32_t & f,
            uint32_t & g, uint32_t & h)
    {
        a^=b<<11; d+=a; b+=c;
        b^=c>>2;  e+=b; c+=d;
        c^=d<<8;  f+=c; d+=e;
        d^=e>>16; g+=d; e+=f;
        e^=f<<10; h+=e; f+=g;
        f^=g>>4;  a+=f; g+=h;
        g^=h<<8;  b+=g; h+=a;
        h^=a>>9;  c+=h; a+=b;
    }

    void shuffle(
            uint64_t & a, uint64_t & b,
            uint64_t & c, uint64_t & d,
            uint64_t & e, uint64_t & f,
            uint64_t & g, uint64_t & h)
    {
        a-=e; f^=h>>9;  h+=a;
        b-=f; g^=a<<9;  a+=b;
        c-=g; h^=b>>23; b+=c;
        d-=h; a^=c<<15; c+=d;
        e-=a; b^=d>>14; d+=e;
        f-=b; c^=e<<20; e+=f;
        g-=c; d^=f>>17; f+=g;
        h-=d; e^=g<<14; g+=h;
    }
};
//---------------------------------------------------------------------------
namespace tests {
//------------------------------------------------------------------------------
void rand_test();
//------------------------------------------------------------------------------
} // namespace tests
//------------------------------------------------------------------------------
} // namespace spacenet
//------------------------------------------------------------------------------
#endif // RAND_HPP_INCLUDED
//------------------------------------------------------------------------------
