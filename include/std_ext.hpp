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
#include <array>
#include <vector>
#include <cctype>
#include <locale>
#include <cerrno>
#include <iterator>
#include <type_traits>
#include <string>
#include <memory>
#if QT_CORE_LIB
#   include <QString>
#endif
//------------------------------------------------------------------------------
#include "numeric/ii.hpp"
//------------------------------------------------------------------------------
namespace std {
//------------------------------------------------------------------------------
inline size_t slen(const char * s) {
    return ::strlen(s);
}
//------------------------------------------------------------------------------
inline size_t slen(const wchar_t * s) {
    return ::wcslen(s);
}
//------------------------------------------------------------------------------
inline auto scat(char * s1, const char * s2) {
    return ::strcat(s1, s2);
}
//------------------------------------------------------------------------------
inline auto scat(wchar_t * s1, const wchar_t * s2) {
    return ::wcscat(s1, s2);
}
//------------------------------------------------------------------------------
inline auto scmp(const char * s1, const char * s2) {
    return ::strcmp(s1, s2);
}
//------------------------------------------------------------------------------
inline auto scmp(const wchar_t * s1, const wchar_t * s2) {
    return ::wcscmp(s1, s2);
}
//------------------------------------------------------------------------------
template <typename C>
struct container_traits {
private:
    template <typename _Elem, size_t N>
    static char (&is_array_helper(const array<_Elem, N> &))[1];
    static char (&is_array_helper(...))[2];

    template <typename _Elem, typename _Alloc>
    static char (&is_vector_helper(const vector<_Elem, _Alloc> &))[1];
    static char (&is_vector_helper(...))[2];

    template <typename _Elem, typename _Traits , typename _Alloc>
    static char (&is_string_helper(const basic_string<_Elem, _Traits, _Alloc> &))[1];
    static char (&is_string_helper(...))[2];

    template <typename _Ty>
    static char (&is_unique_ptr_helper(const unique_ptr<_Ty> &))[1];
    static char (&is_unique_ptr_helper(...))[2];

    template <typename _Ty>
    static char (&is_shared_ptr_helper(const shared_ptr<_Ty> &))[1];
    static char (&is_shared_ptr_helper(...))[2];

    template <typename U//,
        //typename = typename iterator_traits<U>::difference_type,
        //typename = typename iterator_traits<U>::pointer,
        //typename = typename iterator_traits<U>::reference,
        //typename = typename iterator_traits<U>::value_type,
        //typename = typename iterator_traits<U>::iterator_category
    >
    static char (&is_iterator_helper(const iterator_traits<U> &))[1];
    static char (&is_iterator_helper(...))[2];
public:
    constexpr static bool is_array =
        sizeof(is_array_helper(declval<C>())) == sizeof(char[1]);
    constexpr static bool is_vector =
        sizeof(is_vector_helper(declval<C>())) == sizeof(char[1]);
    constexpr static bool is_array_or_vector =
        sizeof(is_array_helper(declval<C>())) == sizeof(char[1]) ||
        sizeof(is_vector_helper(declval<C>())) == sizeof(char[1]);
    constexpr static bool is_string =
        sizeof(is_string_helper(declval<C>())) == sizeof(char[1]);
    constexpr static bool is_string_or_vector =
        sizeof(is_string_helper(declval<C>())) == sizeof(char[1]) ||
        sizeof(is_vector_helper(declval<C>())) == sizeof(char[1]);
    constexpr static bool is_array_or_vector_or_string =
        sizeof(is_array_helper(declval<C>())) == sizeof(char[1]) ||
        sizeof(is_vector_helper(declval<C>())) == sizeof(char[1]) ||
        sizeof(is_string_helper(declval<C>())) == sizeof(char[1]);
    constexpr static bool is_unique_ptr =
        sizeof(is_unique_ptr_helper(declval<C>())) == sizeof(char[1]);
    constexpr static bool is_shared_ptr =
        sizeof(is_shared_ptr_helper(declval<C>())) == sizeof(char[1]);
    constexpr static bool is_unique_or_shared_ptr =
        sizeof(is_unique_ptr_helper(declval<C>())) == sizeof(char[1]) ||
        sizeof(is_shared_ptr_helper(declval<C>())) == sizeof(char[1]);
    constexpr static bool is_iterator =
        sizeof(is_iterator_helper(declval<C>())) == sizeof(char[1]);
};
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
template <class InputIt, class OutputIt> inline
OutputIt copy(InputIt first, InputIt last,
                   OutputIt d_first, OutputIt d_last)
{
    while( first != last && d_first != d_last )
        *d_first++ = (decltype(*d_first)) (*first++);

    return d_first;
}
//------------------------------------------------------------------------------
template <class InputIt, class OutputIt, class InserterIt> inline
OutputIt copy(InputIt first, InputIt last,
                   OutputIt d_first, OutputIt d_last, InserterIt inserter)
{
    while( first != last && d_first != d_last )
        *d_first++ = (decltype(*d_first)) (*first++);
    while( first != last )
        *inserter++ = (decltype(*inserter)) (*first++);

    return d_first;
}
//------------------------------------------------------------------------------
template <class InputIt, class OutputIt, class UnaryOperation> inline
OutputIt transform(InputIt first, InputIt last,
                   OutputIt d_first, OutputIt d_last,
                   UnaryOperation unary_op)
{
    while( first != last && d_first != d_last )
        *d_first++ = unary_op(*first++);

    return d_first;
}
//------------------------------------------------------------------------------
template <class InputIt, class OutputIt, class InserterIt, class UnaryOperation> inline
OutputIt transform(InputIt first, InputIt last,
                   OutputIt d_first, OutputIt d_last,
                   InserterIt inserter,
                   UnaryOperation unary_op)
{
    while( first != last && d_first != d_last )
        *d_first++ = unary_op(*first++);
    while( first != last )
        *inserter++ = unary_op(*first++);

    return d_first;
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

        if( in.gcount() == 0 || in.eof() || in.fail() || in.bad() )
            break;

		out.write(buffer, r);

        if( out.eof() || out.fail() || out.bad() )
            break;

		count -= r;
	}
}
//------------------------------------------------------------------------------
template <const std::size_t buffer_size = 4096> inline
void copy_eof(std::ostream & out, std::istream & in) {
	char buffer[buffer_size];

	for(;;) {
		in.read(buffer, buffer_size);
        auto r = in.gcount();
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
template <typename H, typename T> inline
H ihash(const T & v, H h = H(0)) {
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
template <typename H, typename InputIt,
    typename enable_if<container_traits<InputIt>::is_iterator>::type * = nullptr
> inline
H ithash(InputIt first, InputIt last, H h = H(0)) {
    while( first != last ) {
        h += H(*first++);
        h += h << (sizeof(h) * CHAR_BIT / 4 + 1);
        h ^= h >> (sizeof(h) * CHAR_BIT / 5 - 1);
    }

    h += h << (sizeof(h) * CHAR_BIT / 10);
    h ^= h >> (sizeof(h) * CHAR_BIT / 3);
    h += h << (sizeof(h) * CHAR_BIT / 3 - 2);

    return h;
}
//------------------------------------------------------------------------------
template <typename H, typename InputIt, typename PredT> inline
H ithash(InputIt first, InputIt last, PredT pred, H h = H(0)) {
    while( first != last ) {
        h += H(pred(*first++));
        h += h << (sizeof(h) * CHAR_BIT / 4 + 1);
        h ^= h >> (sizeof(h) * CHAR_BIT / 5 - 1);
    }

    h += h << (sizeof(h) * CHAR_BIT / 10);
    h ^= h >> (sizeof(h) * CHAR_BIT / 3);
    h += h << (sizeof(h) * CHAR_BIT / 3 - 2);

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
//------------------------------------------------------------------------------
inline uint32_t rhash(uint32_t x) {
    x ^= 0xf7f7f7f7;
    x *= 0x8364abf7;
    x = (x << 13) | (x >> 19);
    x ^= 0xf00bf00b;
    x *= 0xf81bc437;
    return x;
}
//------------------------------------------------------------------------------
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
////////////////////////////////////////////////////////////////////////////////
//------------------------------------------------------------------------------
template <typename T>
class ptr_iterator : public iterator<random_access_iterator_tag, T> {
public:
#if __GNUG__
    typedef typename iterator<random_access_iterator_tag, T>::pointer pointer;
    typedef typename iterator<random_access_iterator_tag, T>::reference reference;
    typedef typename iterator<random_access_iterator_tag, T>::difference_type difference_type;
    typedef typename iterator<random_access_iterator_tag, T>::value_type value_type;
#endif
    typedef ptr_iterator<T> iterator;

    ~ptr_iterator() {}
    ptr_iterator() : ptr_(nullptr) {}
    ptr_iterator(T * ptr) : ptr_(ptr) {}

    iterator  operator++(int)                       { return ptr_++; }        // postfix
    iterator& operator++()                          { ++ptr_; return *this; } // prefix
    reference operator* () const                    { return *ptr_; }
    pointer   operator->() const                    { return ptr_; }
    iterator  operator+ (difference_type v)   const { return ptr_ + v; }
    iterator  operator- (difference_type v)   const { return ptr_ - v; }
    iterator& operator+=(difference_type v)   const { return ptr_ += v; }
    iterator& operator-=(difference_type v)   const { return ptr_ -= v; }
    bool      operator==(const iterator& rhs) const { return ptr_ == rhs.ptr_; }
    bool      operator!=(const iterator& rhs) const { return ptr_ != rhs.ptr_; }
    bool      operator>=(const iterator& rhs) const { return ptr_ >= rhs.ptr_; }
    bool      operator<=(const iterator& rhs) const { return ptr_ <= rhs.ptr_; }
    bool      operator> (const iterator& rhs) const { return ptr_ >  rhs.ptr_; }
    bool      operator< (const iterator& rhs) const { return ptr_ <  rhs.ptr_; }

    difference_type operator - (const iterator & v) const { return ptr_ - v.ptr_; }

    pointer get() const {
        ptr_;
    }
protected:
    pointer ptr_;
};
//------------------------------------------------------------------------------
//template <typename T> inline
//ptr_iterator<T> begin(T * val, int) {
//    return ptr_iterator<T>(val);
//}
//------------------------------------------------------------------------------
//template <typename T> inline
//ptr_iterator<T> end(T * val, int) {
//    return ptr_iterator<T>(val);
//}
//------------------------------------------------------------------------------
template <typename T>
class ptr_range {
public:
    typedef typename ptr_iterator<T>::iterator iterator;
    typedef typename ptr_iterator<T>::pointer pointer;
    typedef typename ptr_iterator<T>::reference reference;
    typedef typename ptr_iterator<T>::difference_type difference_type;
    typedef typename ptr_iterator<T>::value_type value_type;

    ptr_range(pointer ptr, difference_type size) : ptr_(ptr), end_(ptr + size) {}

    template <typename ItT,
        typename std::enable_if<
            std::is_same<ItT, ptr_iterator<typename ItT::value_type>>::value
        >::type * = nullptr
    >
    ptr_range(ItT first, ItT last) : ptr_(first.get()), end_(last.get()) {}

    iterator begin() const {
        return ptr_;
    }

    iterator begin() {
        return ptr_;
    }

    iterator end() const {
        return end_;
    }

    iterator end() {
        return end_;
    }

    pointer get() const {
        return ptr_;
    }
protected:
    pointer ptr_;
    pointer end_;
};
//------------------------------------------------------------------------------
template <typename T> inline
ptr_range<T> make_range(T * ptr, size_t length) {
    return ptr_range<T>(ptr, length);
}
//------------------------------------------------------------------------------
template <typename InpT, class OutputIt> inline
OutputIt copy(ptr_range<InpT> inp, OutputIt d_first, OutputIt d_last)
{
    return copy(inp.begin(), inp.end(), d_first, d_last);
}
//------------------------------------------------------------------------------
template <typename InpT, typename OutputIt, typename InserterIt> inline
OutputIt copy(ptr_range<InpT> inp, OutputIt d_first, OutputIt d_last,
              InserterIt inserter)
{
    return copy(inp.begin(), inp.end(), d_first, d_last, inserter);
}
//------------------------------------------------------------------------------
template <class InpT, class OutputIt, class UnaryOperation> inline
OutputIt transform(ptr_range<InpT> inp,
                   OutputIt d_first, OutputIt d_last,
                   UnaryOperation unary_op)
{
    return transform(inp.begin(), inp.end(), d_first, d_last, unary_op);
}
//------------------------------------------------------------------------------
template <class InpT, class OutputIt, class InserterIt, class UnaryOperation> inline
OutputIt transform(ptr_range<InpT> inp,
                   OutputIt d_first, OutputIt d_last,
                   InserterIt inserter,
                   UnaryOperation unary_op)
{
    return transform(inp.begin(), inp.end(), d_first, d_last, inserter, unary_op);
}
//------------------------------------------------------------------------------
template <typename InpT, typename OutT, typename OutputIt> inline
OutputIt copy(ptr_range<InpT> inp, ptr_range<OutT> out)
{
    return copy(inp.begin(), inp.end(), out.begin(), out.end());
}
//------------------------------------------------------------------------------
template <typename InpT, typename OutT, typename OutputIt, typename InserterIt> inline
OutputIt copy(ptr_range<InpT> inp, ptr_range<OutT> out, InserterIt inserter)
{
    return copy(inp.begin(), inp.end(), out.begin(), out.end(), inserter);
}
//------------------------------------------------------------------------------
template <typename InpT, typename OutT, typename OutputIt, class UnaryOperation> inline
OutputIt transform(ptr_range<InpT> inp,
                   ptr_range<OutT> out,
                   UnaryOperation unary_op)
{
    return transform(inp.begin(), inp.end(), out.begin(), out.end(), unary_op);
}
//------------------------------------------------------------------------------
template <typename InpT, typename OutT, typename OutputIt, class InserterIt, class UnaryOperation> inline
OutputIt transform(ptr_range<InpT> inp,
                   ptr_range<OutT> out,
                   InserterIt inserter,
                   UnaryOperation unary_op)
{
    return transform(inp.begin(), inp.end(), out.begin(), out.end(), inserter, unary_op);
}
//------------------------------------------------------------------------------
////////////////////////////////////////////////////////////////////////////////
//------------------------------------------------------------------------------
struct shuffler512 {
    uint64_t a, b, c, d, e, f, g, h;

    void shuffle(const shuffler512 & v) {
#if BYTE_ORDER == LITTLE_ENDIAN
        a -= v.e; f ^= v.h >>  9; h += v.a;
        b -= v.f; g ^= v.a <<  9; a += v.b;
        c -= v.g; h ^= v.b >> 23; b += v.c;
        d -= v.h; a ^= v.c << 15; c += v.d;
        e -= v.a; b ^= v.d >> 14; d += v.e;
        f -= v.b; c ^= v.e << 20; e += v.f;
        g -= v.c; d ^= v.f >> 17; f += v.g;
        h -= v.d; e ^= v.g << 14; g += v.h;
#endif
#if BYTE_ORDER == BIG_ENDIAN
        a = le64toh(a) - le64toh(v.e); f = le64toh(f) ^ (le64toh(v.h) >>  9); h = le64toh(h) + le64toh(v.a);
        b = le64toh(b) - le64toh(v.f); g = le64toh(g) ^ (le64toh(v.a) <<  9); a = le64toh(a) + le64toh(v.b);
        c = le64toh(c) - le64toh(v.g); h = le64toh(h) ^ (le64toh(v.b) >> 23); b = le64toh(b) + le64toh(v.c);
        d = le64toh(d) - le64toh(v.h); a = le64toh(a) ^ (le64toh(v.c) << 15); c = le64toh(c) + le64toh(v.d);
        e = le64toh(e) - le64toh(v.a); b = le64toh(b) ^ (le64toh(v.d) >> 14); d = le64toh(d) + le64toh(v.e);
        f = le64toh(f) - le64toh(v.b); c = le64toh(c) ^ (le64toh(v.e) << 20); e = le64toh(e) + le64toh(v.f);
        g = le64toh(g) - le64toh(v.c); d = le64toh(d) ^ (le64toh(v.f) >> 17); f = le64toh(f) + le64toh(v.g);
        h = le64toh(h) - le64toh(v.d); e = le64toh(e) ^ (le64toh(v.g) << 14); g = le64toh(g) + le64toh(v.h);
#endif
    }
};
//------------------------------------------------------------------------------
////////////////////////////////////////////////////////////////////////////////
//------------------------------------------------------------------------------
class key512 : public array<uint8_t, 64> {
public:
    typedef array<uint8_t, 64> base;

    static constexpr size_type ssize() noexcept { return 64; }

    ~key512() {}
    key512() : base() {}
    key512(std::initializer_list<value_type> l) {
        copy(l.begin(), l.end(), begin(), end());
    }

    key512(leave_uninitialized_type) {}

    key512(zero_initialized_type) {
        base::fill(value_type(0));
    }

    key512(const key512 & o) : base(o) {}
    key512(key512 && o) : base(o) {}

    key512(const ptr_range<uint8_t> & range) {
        std::copy(std::begin(range), std::end(range), begin(), end());
    }

    template <size_t _Size>
    key512(const uint8_t (&o) [_Size]) {
        std::copy(std::begin(o), std::end(o), begin(), end());
    }

    template <size_t _Size>
    key512 & operator = (const uint8_t (&o) [_Size]) {
        std::copy(std::begin(o), std::end(o), begin(), end());
        return *this;
    }

    key512 & operator = (const key512 & o) {
        base::operator = (o);
        return *this;
    }

    key512 & operator = (key512 && o) {
        base::operator = (o);
        return *this;
    }

    bool operator == (const key512 & o) const {
        auto s = reinterpret_cast<const uint64_t *>(data());
        auto d = reinterpret_cast<const uint64_t *>(o.data());
        return s[0] == d[0] && s[1] == d[1] && s[2] == d[2] && s[3] == d[3] && s[4] == d[4] && s[5] == d[5] && s[6] == d[6] && s[7] == d[7];
    }

    bool operator != (const key512 & o) const {
        auto s = reinterpret_cast<const uint64_t *>(data());
        auto d = reinterpret_cast<const uint64_t *>(o.data());
        return s[0] != d[0] && s[1] != d[1] && s[2] != d[2] && s[3] != d[3] && s[4] != d[4] && s[5] != d[5] && s[6] != d[6] && s[7] != d[7];
    }

    bool is_zero() const {
        auto d = reinterpret_cast<const uint64_t *>(data());
        return d[0] == 0 && d[1] == 0 && d[2] == 0 && d[3] == 0 && d[4] == 0 && d[5] == 0 && d[6] == 0 && d[7] == 0;
    }

    void shuffle() {
        reinterpret_cast<shuffler512 *>(data())->shuffle(*reinterpret_cast<const shuffler512 *>(data()));
    }

    void shuffle(const key512 & v) {
        reinterpret_cast<shuffler512 *>(data())->shuffle(*reinterpret_cast<const shuffler512 *>(v.data()));
    }

    void shuffle(const shuffler512 & v) {
        reinterpret_cast<shuffler512 *>(data())->shuffle(v);
    }

    template <size_t _Size>
    void shuffle(const uint8_t (&v) [_Size]) {
        static_assert( _Size == 64, "_Size == 64");
        reinterpret_cast<shuffler512 *>(data())->shuffle(*reinterpret_cast<const shuffler512 *>(v));
    }
};
//------------------------------------------------------------------------------
inline string to_string(
    const key512 & b,
    const char * abc = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ",
    char delimiter = '-',
    size_t interval = 9)
{
    string s;
    size_t l = slen(abc), i = 0;

    nn::integer a(b.data(), b.size()), d = l, mod;

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
inline key512 stokey512(const std::string & s,
    const char * abc = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ")
{
    size_t l = slen(abc);
    nn::integer a(0), m(1);

    for( const auto & c : s ) {
        const char * p = ::strchr(abc, c);

        if( p == nullptr )
            continue;

        a += m * nn::integer(p - abc);
        m *= l;
    }

    return key512(std::make_range((uint8_t *) a.data(), a.size()));
}
//---------------------------------------------------------------------------
////////////////////////////////////////////////////////////////////////////////
//------------------------------------------------------------------------------
class blob : public vector<uint8_t> {
public:
    typedef vector<uint8_t> base;
    typedef typename base::iterator iterator;
    typedef typename base::value_type value_type;
    typedef typename base::pointer pointer;
    typedef typename base::const_pointer const_pointer;
    typedef typename base::allocator_type allocator_type;

    blob() : base() {}

    blob(const blob & o) : base(o) {}
    blob(blob && o) : base(o) {}

    template <class _Iter>
    blob(_Iter _First, _Iter _Last) : base(_First, _Last) {}

    blob & operator = (const blob & o) {
        base::operator = (o);
        return *this;
    }

    blob & operator = (blob && o) {
        base::operator = (o);
        return *this;
    }
};
//------------------------------------------------------------------------------
inline string to_string(const blob & b)
{
    ostringstream s;

    s.fill('0');
    s.width(2);
    s.unsetf(ios::dec);
    s.setf(ios::hex | ios::uppercase);

    for( const auto & c : b )
        s << /*setw(2) <<*/ uint16_t(c);

    return s.str();
}
//---------------------------------------------------------------------------
////////////////////////////////////////////////////////////////////////////////
//------------------------------------------------------------------------------
template <typename Lambda>
class AtScopeExit {
private:
    const Lambda & lambda_;
public:
    ~AtScopeExit() { lambda_(); }
    AtScopeExit(const Lambda & lambda) : lambda_(lambda) {}
};
//------------------------------------------------------------------------------
////////////////////////////////////////////////////////////////////////////////
//------------------------------------------------------------------------------
template <typename Lambda>
class ScopeExit {
private:
    Lambda lambda_;
    void operator = (const ScopeExit<Lambda> & obj);
public:
    ~ScopeExit() noexcept {
        lambda_();
    }
    ScopeExit(const Lambda lambda) noexcept : lambda_(lambda) {}
    ScopeExit(const ScopeExit<Lambda> & obj) noexcept : lambda_(obj.lambda_) {}
    ScopeExit(ScopeExit<Lambda> && obj) noexcept : lambda_(obj.lambda_) {}
};
//------------------------------------------------------------------------------
template <typename Lambda> inline
auto scope_exit(Lambda lambda) {
    return ScopeExit<Lambda>(lambda);
}
//------------------------------------------------------------------------------
#define AtScopeExit_INTERNAL2(lname, aname, ...) \
    const auto lname = [&]() { __VA_ARGS__; }; \
    std::AtScopeExit<decltype(lname)> aname(lname);

#define AtScopeExit_TOKENPASTE(x, y) AtScopeExit_ ## x ## y

#define AtScopeExit_INTERNAL1(ctr, ...) \
    AtScopeExit_INTERNAL2(AtScopeExit_TOKENPASTE(func_, ctr), \
                   AtScopeExit_TOKENPASTE(instance_, ctr), __VA_ARGS__)

#define at_scope_exit(...) AtScopeExit_INTERNAL1(__COUNTER__, __VA_ARGS__)
//------------------------------------------------------------------------------
} // namespace std
//------------------------------------------------------------------------------
namespace homeostas { namespace tests {
//------------------------------------------------------------------------------
void locale_traits_test();
//------------------------------------------------------------------------------
}} // namespace homeostas::tests
//------------------------------------------------------------------------------
#endif // STD_EXT_HPP_INCLUDED
//------------------------------------------------------------------------------
