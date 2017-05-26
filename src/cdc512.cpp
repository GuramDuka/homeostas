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
#include "cdc512.hpp"
#include "rand.hpp"
#include "rand.hpp"
#include "port.hpp"
//------------------------------------------------------------------------------
namespace homeostas {
//------------------------------------------------------------------------------
void cdc512_data::shuffle()
{
	a -= e; f ^= h >>  9; h += a;
	b -= f; g ^= a <<  9; a += b;
	c -= g; h ^= b >> 23; b += c;
	d -= h; a ^= c << 15; c += d;
	e -= a; b ^= d >> 14; d += e;
	f -= b; c ^= e << 20; e += f;
	g -= c; d ^= f >> 17; f += g;
	h -= d; e ^= g << 14; g += h;
}
//---------------------------------------------------------------------------
void cdc512_data::shuffle(const cdc512_data & v)
{
	//constexpr const uint64_t prime_a = UINT64_C(0x992E367BE6F0EA1E);
	//constexpr const uint64_t prime_b = UINT64_C(0x71DCF41FFACC283F);
	//constexpr const uint64_t prime_c = UINT64_C(0xC9581F48D85ABD75);
	//constexpr const uint64_t prime_d = UINT64_C(0xE4B93335FF1CE990);
	//constexpr const uint64_t prime_e = UINT64_C(0xE51D6424EFEC1E01);
	//constexpr const uint64_t prime_f = UINT64_C(0x353867A0E66C2A39);
	//constexpr const uint64_t prime_g = UINT64_C(0xA8DBF7B782226B67);
	//constexpr const uint64_t prime_h = UINT64_C(0x9F8B7F0DC254488E);
	
	//a -= v.e ^ prime_e; f ^= v.h >>  9; h += v.a ^ prime_a;
	//b -= v.f ^ prime_f; g ^= v.a <<  9; a += v.b ^ prime_b;
	//c -= v.g ^ prime_g; h ^= v.b >> 23; b += v.c ^ prime_c;
	//d -= v.h ^ prime_h; a ^= v.c << 15; c += v.d ^ prime_d;
	//e -= v.a ^ prime_a; b ^= v.d >> 14; d += v.e ^ prime_e;
	//f -= v.b ^ prime_b; c ^= v.e << 20; e += v.f ^ prime_f;
	//g -= v.c ^ prime_c; d ^= v.f >> 17; f += v.g ^ prime_g;
	//h -= v.d ^ prime_d; e ^= v.g << 14; g += v.h ^ prime_h;

	a -= v.e; f ^= v.h >>  9; h += v.a;
	b -= v.f; g ^= v.a <<  9; a += v.b;
	c -= v.g; h ^= v.b >> 23; b += v.c;
	d -= v.h; a ^= v.c << 15; c += v.d;
	e -= v.a; b ^= v.d >> 14; d += v.e;
	f -= v.b; c ^= v.e << 20; e += v.f;
	g -= v.c; d ^= v.f >> 17; f += v.g;
	h -= v.d; e ^= v.g << 14; g += v.h;
}
//---------------------------------------------------------------------------
void cdc512::init()
{
    a = htobe64(0x46F87CB1B3EB6319ull);
    b = htobe64(0x7D7E68848EA8773Aull);
    c = htobe64(0x18EEE71638D8563Aull);
    d = htobe64(0xD5DB16BCFDF2D51Dull);
    e = htobe64(0x4A878FB7B7463866ull);
    f = htobe64(0xF8ED636BF755D298ull);
    g = htobe64(0x2FF191FF69798254ull);
    h = htobe64(0x8D3F9964239E6334ull);

    p = 0;
}
//---------------------------------------------------------------------------
void cdc512::update(const void * data, uintptr_t size)
{
	p += size;

	while( size >= sizeof(cdc512_data) ){
		shuffle(*reinterpret_cast<const cdc512_data *>(data));
		shuffle();
		data = (const uint8_t *) data + sizeof(cdc512_data);
		size -= sizeof(cdc512_data);
	}

	if( size > 0 ) {
		cdc512_data pad;
		
		std::memcpy(&pad, data, size);
		std::memset((uint8_t *) &pad + size, 0, sizeof(cdc512_data) - size);

		shuffle(pad);
		shuffle();
	}
}
//---------------------------------------------------------------------------
void cdc512::finish()
{
    if( p ) {
        cdc512_data pad = { p, p, p, p, p, p, p, p };
	
        shuffle(pad);
        shuffle();
    }
	
    for( auto & v : digest64 )
        v = htobe64(v);
}
//---------------------------------------------------------------------------
std::string cdc512::to_string() const
{
    std::stringstream s;
	
	s.fill('0');
	s.width(2);
	s.unsetf(std::ios::dec);
    s.setf(std::ios::hex | std::ios::uppercase);

	size_t i;
	
	for( i = 0; i < sizeof(digest) - 1; i++ ) {
		s << std::setw(2) << uint16_t(digest[i]);
		if( (i & 1) != 0 )
			s << '-';
	}

	s << std::setw(2) << uint16_t(digest[i]);

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

    nn::integer a(digest, sizeof(digest)), d = l, mod;

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

    memcpy(digest, a.data(), nn::imin(a.size(), sizeof(digest)));
}
//---------------------------------------------------------------------------
void cdc512::generate_entropy(std::vector<uint8_t> * p_entropy)
{
    std::vector<uint8_t> e, & entropy = p_entropy == nullptr ? e : *p_entropy;

    typedef homeostas::rand<8, uint64_t> rand;
    rand r;

    struct timespec ts;
    clock_gettime(CLOCK_REALTIME, &ts);
    auto clock_ns = 1000000000ull * ts.tv_sec + ts.tv_nsec;

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

    for( auto & q : digest )
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
}
//---------------------------------------------------------------------------
#ifndef NDEBUG
//---------------------------------------------------------------------------
std::string cdc512::generate_prime()
{
    std::stringstream s;

    s.fill('0');
    //s.width(2);
    s.unsetf(std::ios::dec);
    s.setf(std::ios::hex | std::ios::uppercase);

    rand<8, uint64_t> r;

    struct timespec ts;
    clock_gettime(CLOCK_REALTIME, &ts);

    r.srand(ts.tv_sec ^ uintptr_t(&s), ts.tv_nsec ^ uintptr_t(&s), ts.tv_sec ^ ts.tv_nsec);
    r.warming();

    uint64_t v = 0;

    for( size_t i = 0; i < sizeof(digest64) / sizeof(digest64[0]); i++ ) {
        uint64_t a = 0;

        for( auto j = sizeof(digest64[0]); j > 0; j-- )
            for(;;) {
                if( v == 0 )
                    v = r.get();

                if( (v & 0xf) != 0 && ((v >> 4) & 0xf) != 0 ) {
                    a = (a << 8) | (v & 0xff);
                    v >>= 8;
                    break;
                }

                v >>= 8;
            }

        s << "\t" << char(i + 'a') << " = htobe64(0x" << a << "ull);\n";
    }

    return s.str();
}
//---------------------------------------------------------------------------
#endif
//---------------------------------------------------------------------------
} // namespace homeostas
//------------------------------------------------------------------------------
