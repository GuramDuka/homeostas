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
#ifndef STD_EXT_HPP_INCLUDED
#define STD_EXT_HPP_INCLUDED
//------------------------------------------------------------------------------
#pragma once
//------------------------------------------------------------------------------
#include "config.h"
//------------------------------------------------------------------------------
#include <algorithm>
#include <sstream>
#include <iomanip>
#include <QtDebug>
#include <QString>
//------------------------------------------------------------------------------
namespace std {
//------------------------------------------------------------------------------
////////////////////////////////////////////////////////////////////////////////
//------------------------------------------------------------------------------
class qdbgbuf : public stringbuf {
    public:
        qdbgbuf() {}

        virtual int sync() {
            // add this->str() to database here
            // (optionally clear buffer afterwards)
            auto s = this->str();

            if( !s.empty() && s.back() == '\n' )
                s.pop_back();

            qDebug().nospace().noquote() << QString::fromStdString(s);
            this->str(string());
            return 0;
        }
};
//------------------------------------------------------------------------------
extern ostream qerr;
//------------------------------------------------------------------------------
inline ostream & operator << (ostream & os, const std::exception & e) {
    return os << e.what();
}
//------------------------------------------------------------------------------
inline ostream & operator << (ostream & os, const QString & s) {
    return os << s.toStdString();
}
//------------------------------------------------------------------------------
inline string operator + (const string & s1, const char * s2) {
    return s1 + string(s2);
}
//------------------------------------------------------------------------------
template <typename T, typename T1, typename T2, typename T3> inline
T str_replace(const T1 & subject, const T2 & search, const T3 & replace) {
    T s;
    auto sb = std::begin(search), se = std::end(search);
    const auto l = se - sb;
    auto b = std::begin(subject), e = std::end(subject);
    auto i = b;

    for(;;) {
        i = std::search(b, e, sb, se);

		if( i == e )
			break;

		s.append(b, i).append(replace);
		b = i + l;
	}

    s.append(b, i);

	return s;
}
//------------------------------------------------------------------------------
template <const std::size_t buffer_size = 4096, class size_type = std::size_t> inline
void stream_copy_n(std::ostream & out, std::istream & in, const size_type & size) {
	char buffer[buffer_size];
	size_type count = size;

	//in.exceptions(std::istream::failbit);
	//out.exceptions(std::ostream::failbit);

	while( count > 0 ) {
		size_type r = count > buffer_size ? size_type(buffer_size) : count;

		in.read(buffer, r);
		//if( in.bad() || in.gcount() != r )
		//	throw std::runtime_error("stream read error");

		out.write(buffer, r);
		//if( out.bad() )
		//	throw std::runtime_error("stream write error");

		count -= r;
	}
}
//------------------------------------------------------------------------------
template <const std::size_t buffer_size = 4096> inline
void copy_eof(std::ostream & out, std::istream & in) {
	char buffer[buffer_size];

	for(;;) {
		in.read(buffer, buffer_size);
		std::size_t r = in.gcount();
		out.write(buffer, r);

		if( in.eof() )
			break;
	}
}
//------------------------------------------------------------------------------
#if __GNUC__ > 0 && __GNUC__ < 5
//------------------------------------------------------------------------------
template <typename T> inline string to_string(const T & v) {
    ostringstream s;
    s << v;
    return s.str();
}
//------------------------------------------------------------------------------
#endif
//------------------------------------------------------------------------------
template <class H> inline
H ihash(const char * val) {
    H h = 0;

    while( *val != '\0' ) {
        h += *val++;
        h += h << 9;
        h ^= h >> 5;
    }

    h += h << 3;
    h ^= h >> 10;
    h += h << 14;

    return h;
}
//------------------------------------------------------------------------------
template <typename H, typename T> inline
H ihash(const T & v) {
    H h = 0;
    const uint8_t * val = reinterpret_cast<const uint8_t *>(&v);

    for( auto i = sizeof(v); i > 0; i-- ) {
        h += *val++;
        h += h << (sizeof(h) * CHAR_BIT / 4 + 1); // when H 32 bit wide then 9
        h ^= h >> (sizeof(h) * CHAR_BIT / 5 - 1); // when H 32 bit wide then 5
    }

    h += h << (sizeof(h) * CHAR_BIT / 10);        // when H 32 bit wide then 3
    h ^= h >> (sizeof(h) * CHAR_BIT / 3);         // when H 32 bit wide then 10
    h += h << (sizeof(h) * CHAR_BIT / 3 - 2);     // when H 32 bit wide then 14

    return h;
}
//------------------------------------------------------------------------------
template <typename H, typename InputIt> inline
H ihash(InputIt first, InputIt last) {
    H h = 0;

    while( first != last ) {
        h += *first++;
        h += h << 9;
        h ^= h >> 5;
    }

    h += h << 3;
    h ^= h >> 10;
    h += h << 14;

    return h;
}
//------------------------------------------------------------------------------
inline uint64_t rhash(uint64_t x) {
    x ^= UINT64_C(0xf7f7f7f7f7f7f7f7);
    x *= UINT64_C(0x8364abf78364abf7);
    x = (x << 26) | (x >> 38);
    x ^= UINT64_C(0xf00bf00bf00bf00b);
    x *= UINT64_C(0xf81bc437f81bc437);
    return x;
}

inline uint32_t rhash(uint32_t x) {
    x ^= 0xf7f7f7f7;
    x *= 0x8364abf7;
    x = (x << 13) | (x >> 19);
    x ^= 0xf00bf00b;
    x *= 0xf81bc437;
    return x;
}

inline uint16_t rhash(uint16_t x) {
    x ^= 0xf7f7;
    x *= 0xabf7;
    x = (x << 7) | (x >> 9);
    x ^= 0xf00b;
    x *= 0xc437;
    return x;
}
//------------------------------------------------------------------------------
inline std::string to_string_ellapsed(uint64_t ns) {
    auto a		= ns / 1000000000ull;
    auto nsecs	= ns % 1000000000ull;
    auto days	= a / (60 * 60 * 24);
    auto hours	= a / (60 * 60) - days * 24;
    auto mins	= a / 60 - days * 24 * 60 - hours * 60;
    auto secs	= a - days * 24 * 60 * 60 - hours * 60 * 60 - mins * 60;
    std::ostringstream s;

    auto out = [&s] (uint64_t v, streamsize w = 2, char prefix = ':') {
        s << prefix << right << setfill('0') << setw(w) << v;
    };

    if( days != 0 ) {
        s << days; out(hours); out(mins); out(secs); out(nsecs, 9, '.');
    }
    else if( hours != 0 ) {
        out(hours); out(mins); out(secs); out(nsecs, 9, '.');
    }
    else if( mins != 0 ) {
        out(mins); out(secs); out(nsecs, 9, '.');
    }
    else if( secs != 0 ) {
        out(secs); out(nsecs, 9, '.');
    }
    else
        out(nsecs, 9, '.');

    return s.str();
}
//------------------------------------------------------------------------------
} // namespace std
//------------------------------------------------------------------------------
#endif // STD_EXT_HPP_INCLUDED
//------------------------------------------------------------------------------
