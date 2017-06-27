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
#include <stdexcept>
#include <exception>
#include <vector>
#include <cctype>
#include <locale>
#include <cerrno>
#if QT_CORE_LIB
#   include <QString>
#endif
//------------------------------------------------------------------------------
namespace std {
//------------------------------------------------------------------------------
template <typename T> inline
constexpr const T & xmin(const T & a, const T & b) {
    return a < b ? a : b;
}
//------------------------------------------------------------------------------
template <typename T> inline
constexpr const T & xmax(const T & a, const T & b) {
    return a > b ? a : b;
}
//------------------------------------------------------------------------------
////////////////////////////////////////////////////////////////////////////////
//------------------------------------------------------------------------------
class xruntime_error : public runtime_error {
public:
    xruntime_error(const char * msg, const char * file, int line) :
        runtime_error(xmsg(msg, file, line))
    {
    }

    xruntime_error(const string & msg, const char * file, int line) :
        xruntime_error(msg.c_str(), file, line) {}

#if QT_CORE_LIB
    xruntime_error(const wchar_t * msg, const char * file, int line) :
        xruntime_error(QString::fromWCharArray(msg).toStdString().c_str(), file, line) {}

    xruntime_error(const wstring & msg, const char * file, int line) :
        xruntime_error(QString::fromStdWString(msg).toStdString().c_str(), file, line) {}

    xruntime_error(const QString & msg, const char * file, int line) :
        xruntime_error(msg.toStdString(), file, line) {}
#endif
private:
    string xmsg(const char * msg, const char * file, int line) {
        ostringstream s;
        s << msg << ", " << file << ", " << line;
        return s.str();
    }

    void get_stack_trace() {

        //__builtin_frame_address(114);
        //__builtin_return_address(1);

    }

    //const char * file_;
    //int line_;

#if __GNUG__
    //vector<string> trace_;
#endif
};
//------------------------------------------------------------------------------
inline ostream & operator << (ostream & os, const std::exception & e) {
    return os << e.what();
}
//------------------------------------------------------------------------------
#if QT_CORE_LIB
inline ostream & operator << (ostream & os, const QString & s) {
    return os << s.toStdString();
}
#endif
//------------------------------------------------------------------------------
inline string operator + (const string & s1, const char * s2) {
    return s1 + string(s2);
}
//------------------------------------------------------------------------------
//template <typename T, class InputIt, class _Elem, class _Traits, class _Alloc> inline
template <typename T, typename InputIt,
    typename enable_if<is_integral<T>::value>::type * = nullptr
> inline
T stoint(InputIt first, InputIt last, int base = 10) {
    T result = T(0);

    typedef typename InputIt::value_type value_type;

    auto char_to_int = [] (value_type c) {
        return char_traits<value_type>::to_int_type(c);
    };

    while( ::isspace(char_to_int(*first)) )
        first++;

    auto throw_invalid = [] {
        throw invalid_argument("to_int: invalid string");
    };

    bool neg = false, sign = false;
    InputIt i;

    for( i = first; i != last; i++ ) {
        auto c = *i;
        auto u = char_to_int(c);

        if( ::isspace(u) )
            continue;

        if( char_traits<value_type>::eq(c, '-') ) {
            neg = true;
            sign = true;
        }
        else if( char_traits<value_type>::eq(c, '+') ) {
            sign = true;
        }
        else if( ::isdigit(u) ) {
            result = result * base + (c - '0');
        }
        else if( ::isxdigit(u) ) {
            if( ::isupper(u) )
                result = result * base + (c - 'A');
            else
                result = result * base + (c - 'a');
        }
        else
            throw_invalid();

        if( sign && i != first )
            throw_invalid();
    }

    if( neg ) {
        typedef typename make_signed<T>::type SignedT;
        result = T(-SignedT(result));
    }

    return result;
}
//------------------------------------------------------------------------------
template <typename InputIt> inline
auto stointmax(InputIt first, InputIt last, int base = 10) {
    return stoint<intmax_t>(first, last, base);
}
//------------------------------------------------------------------------------
template <class _InIt1, class _InIt2> inline
bool starts_with(_InIt1 haystack_first, _InIt1 haystack_last,
                 _InIt2 needle_first, _InIt2 needle_last)
{
    return distance(needle_first, needle_last) <= distance(haystack_first, haystack_last)
        && equal(needle_first, needle_last, haystack_first, needle_first);
}
//------------------------------------------------------------------------------
template <class _InIt1, class _InIt2, class _Pred> inline
bool starts_with(_InIt1 haystack_first, _InIt1 haystack_last,
                 _InIt2 needle_first, _InIt2 needle_last, _Pred pred)
{
    return distance(needle_first, needle_last) <= distance(haystack_first, haystack_last)
        && equal(needle_first, needle_last, haystack_first, pred);
}
//------------------------------------------------------------------------------
template <class _Elem, class _Traits, class _Alloc> inline
bool starts_with(
    const basic_string<_Elem, _Traits, _Alloc> & haystack,
    const basic_string<_Elem, _Traits, _Alloc> & needle)
{
    return starts_with(haystack.begin(), haystack.end(), needle.begin(), needle.end());
}
//------------------------------------------------------------------------------
template <class _Elem, class _Traits, class _Alloc, size_t _Size> inline
bool starts_with(
    const basic_string<_Elem, _Traits, _Alloc> & haystack,
    const _Elem (&needle)[_Size])
{
    auto haystack_b = begin(haystack), haystack_e = end(haystack);
    auto needle_b = begin(needle), needle_e = end(needle);

    while( needle_e > needle_b && needle_e[-1] == _Elem(0) )
        needle_e--;

    return starts_with(haystack_b, haystack_e, needle_b, needle_e);
}
//------------------------------------------------------------------------------
template <class _Elem, size_t _SizeH, size_t _SizeN> inline
bool starts_with(
    const _Elem (&haystack)[_SizeH],
    const _Elem (&needle)[_SizeN])
{
    auto haystack_b = begin(haystack), haystack_e = end(haystack);

    while( haystack_e > haystack_b && haystack_e[-1] == _Elem(0) )
        haystack_e--;

    auto needle_b = begin(needle), needle_e = end(needle);

    while( needle_e > needle_b && needle_e[-1] == _Elem(0) )
        needle_e--;

    return starts_with(haystack_b, haystack_e, needle_b, needle_e);
}
//------------------------------------------------------------------------------
template <class _Elem, class _Traits, class _Alloc, size_t _Size> inline
bool starts_with_case(
    const basic_string<_Elem, _Traits, _Alloc> & haystack,
    const basic_string<_Elem, _Traits, _Alloc> & needle)
{
    auto haystack_b = begin(haystack), haystack_e = end(haystack);
    auto needle_b = begin(needle), needle_e = end(needle);

    return starts_with(haystack_b, haystack_e, needle_b, needle_e,
        [] (const auto & a, const auto & b) {
            return ::toupper(_Traits::to_int_type(*a))
                    == ::toupper(_Traits::to_int_type(*b));
        });
}
//------------------------------------------------------------------------------
template <class _Elem, class _Traits, class _Alloc, size_t _Size> inline
bool starts_with_case(
    const basic_string<_Elem, _Traits, _Alloc> & haystack,
    const _Elem (&needle)[_Size])
{
    auto haystack_b = begin(haystack), haystack_e = end(haystack);
    auto needle_b = begin(needle), needle_e = end(needle);

    while( needle_e > needle_b && needle_e[-1] == _Elem(0) )
        needle_e--;

    return starts_with(haystack_b, haystack_e, needle_b, needle_e,
        [] (const auto & a, const auto & b) {
            return ::toupper(_Traits::to_int_type(a))
                    == ::toupper(_Traits::to_int_type(b));
        });
}
//------------------------------------------------------------------------------
template <class _Elem, size_t _SizeH, size_t _SizeN> inline
bool starts_with_case(
    const _Elem (&haystack)[_SizeH],
    const _Elem (&needle)[_SizeN])
{
    auto haystack_b = begin(haystack), haystack_e = end(haystack);

    while( haystack_e > haystack_b && haystack_e[-1] == _Elem(0) )
        haystack_e--;

    auto needle_b = begin(needle), needle_e = end(needle);

    while( needle_e > needle_b && needle_e[-1] == _Elem(0) )
        needle_e--;

    return starts_with(haystack_b, haystack_e, needle_b, needle_e,
        [] (const auto & a, const auto & b) {
            return ::toupper(char_traits<_Elem>::to_int_type(a))
                == ::toupper(char_traits<_Elem>::to_int_type(b));
        });
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
template <class _Elem, class _Traits, class _Alloc, size_t _Size> inline
bool equal(
    const basic_string<_Elem, _Traits, _Alloc> & s1,
    const _Elem (&s2)[_Size])
{
    return equal(begin(s1), end(s1), begin(s2), end(s2));
}
//------------------------------------------------------------------------------
template <class _Elem, class _Traits, class _Alloc, size_t _Size> inline
bool equal_case(
    const basic_string<_Elem, _Traits, _Alloc> & s1,
    const _Elem (&s2)[_Size])
{
    return equal(begin(s1), end(s1), begin(s2), end(s2),
        [] (const auto & a, const auto & b) {
            return ::toupper(char_traits<_Elem>::to_int_type(a))
                == ::toupper(char_traits<_Elem>::to_int_type(b));
        }
    );
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
        //	throw std::xruntime_error("stream read error");

		out.write(buffer, r);
		//if( out.bad() )
        //	throw std::xruntime_error("stream write error");

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
#if __GNUC__ > 0 && __GNUC__ < 5 && __ANDROID__
//------------------------------------------------------------------------------
template <typename T> inline string to_string(const T & v) {
    static thread_local ostringstream s;
    s.str(string()); // clear
    s << v;
    return s.str();
}
//------------------------------------------------------------------------------
inline double stod(const string & s, size_t * p_idx = nullptr) {
    const char * str = s.c_str();
    char * endp;

    errno = 0;
    auto ret = strtod(str, &endp);

    if( endp == str )
        throw std::invalid_argument("stod");

    if( errno == ERANGE )
      throw std::out_of_range("stod");

    if( p_idx != nullptr )
        *p_idx = endp - str;

    return ret;
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
template <typename H, typename InputIt, typename PredT> inline
H ihash(InputIt first, InputIt last, PredT pred) {
    H h = 0;

    while( first != last ) {
        h += H(pred(*first++));
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
    x ^= uint64_t(0xf7f7f7f7f7f7f7f7);
    x *= uint64_t(0x8364abf78364abf7);
    x = (x << 26) | (x >> 38);
    x ^= uint64_t(0xf00bf00bf00bf00b);
    x *= uint64_t(0xf81bc437f81bc437);
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
