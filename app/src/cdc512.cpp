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
#include "config.h"
//------------------------------------------------------------------------------
#include <random>
#include <cstring>
#include <iomanip>
//------------------------------------------------------------------------------
#include "numeric/ii.hpp"
#include "std_ext.hpp"
#include "cdc512.hpp"
#include "rand.hpp"
#include "port.hpp"
//------------------------------------------------------------------------------
namespace homeostas {
//------------------------------------------------------------------------------
cdc512 & cdc512::init()
{
    auto d = reinterpret_cast<uint64_t *>(data());

    d[0] = htole64(0x46F87CB1B3EB6319ull);
    d[1] = htole64(0x7D7E68848EA8773Aull);
    d[2] = htole64(0x18EEE71638D8563Aull);
    d[3] = htole64(0xD5DB16BCFDF2D51Dull);
    d[4] = htole64(0x4A878FB7B7463866ull);
    d[5] = htole64(0xF8ED636BF755D298ull);
    d[6] = htole64(0x2FF191FF69798254ull);
    d[7] = htole64(0x8D3F9964239E6334ull);

    p = 0;

    return *this;
}
//---------------------------------------------------------------------------
cdc512 & cdc512::update(const void * data, uintptr_t size)
{
	p += size;

    while( size >= sizeof(std::shuffler512) ) {
        shuffle(*reinterpret_cast<const std::shuffler512 *>(data));
        data = (const uint8_t *) data + sizeof(std::shuffler512);
        size -= sizeof(std::shuffler512);
	}

	if( size > 0 ) {
        std::shuffler512 pad;
		
        memcpy(&pad, data, size);
        memset(reinterpret_cast<uint8_t *>(&pad) + size, 0, sizeof(pad) - size);

		shuffle(pad);
	}

    return *this;
}
//---------------------------------------------------------------------------
cdc512 & cdc512::final()
{
    if( p ) {
        std::shuffler512 pad;

        pad.a = p;
        pad.b = p << 1;
        pad.c = p << 2;
        pad.d = p << 3;
        pad.e = p << 4;
        pad.f = p << 5;
        pad.g = p << 6;
        pad.h = p << 7;

        pad.shuffle(pad);
        pad.shuffle(*reinterpret_cast<std::shuffler512 *>(data()));
        shuffle(pad);
    }

    return *this;
}
//---------------------------------------------------------------------------
/*std::string cdc512::to_string() const
{
    std::ostringstream s;
	
	s.fill('0');
	s.width(2);
	s.unsetf(std::ios::dec);
    s.setf(std::ios::hex | std::ios::uppercase);

    for( const auto & c : *this )
        s << uint16_t(c);

	return s.str();
}
//---------------------------------------------------------------------------
std::string cdc512::to_short_string(const char * abc, char delimiter, size_t interval) const
{
    std::string s;

    if( abc == nullptr )
        abc = "._,=~!@#$%^&-+0123456789abcdefghijklmnopqrstuvwxyz";

    size_t l = slen(abc), i = 0;

    //for( auto a : digest64 )
    //    while( a ) {
    //        s.push_back(abc[a % l]);
    //        a /= l;
    //
    //        if( delimiter != '\0' && interval != 0 && ++i == interval ) {
    //            s.push_back(delimiter);
    //            i = 0;
    //        }
    //    }

    nn::integer a(data(), size()), d = l, mod;

    while( a.is_notz() ) {
        a = a.div(l, &mod);
        s.push_back(abc[size_t(mod)]);

        if( delimiter != '\0' && interval != 0 && ++i == interval ) {
            s.push_back(delimiter);
            i = 0;
        }
    }

    if( delimiter != '\0' && interval != 0 && s.back() == delimiter )
        s.pop_back();

    return s;
}
//---------------------------------------------------------------------------
void cdc512::from_short_string(const std::string & s, const char * abc)
{
    if( abc == nullptr )
        abc = "._,=~!@#$%^&-+0123456789abcdefghijklmnopqrstuvwxyz";

    size_t l = slen(abc);
    nn::integer a(0), m(1);

    for( const auto & c : s ) {
        const char * p = std::strchr(abc, c);

        if( p == nullptr )
            continue;

        a += m * nn::integer(p - abc);
        m *= l;
    }

    memcpy(data(), a.data(), std::min(a.size(), size()));
}*/
//---------------------------------------------------------------------------
cdc512 & cdc512::generate_entropy_fast()
{
    auto f = std::bind(&cdc512::generate_entropy_fast, this);
    union {
        struct {
            uint64_t ts;
            void * p0;
            uintptr_t p1;
            void * p2;
            uintptr_t p3;
            uint8_t ff[sizeof(f)];
        };
        uint64_t g[16]; // some garbage on stack
    } a;

    a.ts = clock_gettime_ns();
    memcpy(&a.ff, &f, sizeof(f));
    a.p0 = &a;
    a.p1 = std::rhash(uintptr_t(&a)) ^ uintptr_t(&a) ^ uintptr_t(a.ts);
    a.p2 = (void *) std::rhash(a.p1);
    a.p3 = uintptr_t(a.ts) ^ uintptr_t(a.p0);

    a.g[9] = std::rhash(a.g[3] >> 5) ^ (a.ts << 3);
    a.g[14] = clock_gettime_ns() ^ std::rhash(a.ts >> 5) ^ (a.ts << 3);
    a.g[15] = std::rhash(a.g[8]) ^ (a.ts << 17);

    //init();
    update(&a, sizeof(a));
    final();

    return *this;
}
//---------------------------------------------------------------------------
cdc512 & cdc512::generate_entropy(std::vector<uint8_t> * p_entropy)
{
    std::vector<uint8_t> e, & entropy = p_entropy == nullptr ? e : *p_entropy;

    typedef homeostas::rand<8, uint64_t> rand;
    rand r;

    auto clock_ns = clock_gettime_ns();

    while( clock_ns && entropy.size() < rand::srand_size ) {
        uint8_t q = uint8_t(clock_ns & 0xff);
        if( q != 0 )
            entropy.push_back(clock_ns & 0xff);
        clock_ns >>= 8;
    }

    std::random_device rd;
    std::uniform_int_distribution<int> dist(1, 15);

    while( entropy.size() < rand::srand_size )
        entropy.push_back(dist(rd));

    r.srand(&entropy[0], entropy.size());
    r.warming();

    rand::value_type v = 0;

    for( auto & q : *this )
        for(;;) {
            if( v == 0 )
                v = r.get();

            if( (v & 0xf) != 0 && ((v >> 4) & 0xf) != 0 ) {
                q = v & 0xff;
                v >>= 8;
                break;
            }

            v >>= 8;
        }

    return *this;
}
//---------------------------------------------------------------------------
} // namespace homeostas
//------------------------------------------------------------------------------
