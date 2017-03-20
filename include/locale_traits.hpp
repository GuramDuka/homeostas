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
#ifndef LOCALE_TRAITS_HPP_INCLUDED
#define LOCALE_TRAITS_HPP_INCLUDED
//------------------------------------------------------------------------------
#pragma once
//------------------------------------------------------------------------------
#include "config.h"
//------------------------------------------------------------------------------
#if HAVE_CODECVT
#include <codecvt>
#endif
#include <type_traits>
#include <locale>
#include <string>
#include <sstream>
#include <regex>
#include <cwchar>
//------------------------------------------------------------------------------
#include "std_ext.hpp"
//------------------------------------------------------------------------------
namespace spacenet {
//------------------------------------------------------------------------------
////////////////////////////////////////////////////////////////////////////////
//------------------------------------------------------------------------------
#if __GNUC__ >= 5 || _MSC_VER
//------------------------------------------------------------------------------
template <typename CharType>
struct locale_traits : public std::char_traits<CharType> {
	static auto lt(CharType a, CharType b) {
#if _WIN32
        return _wcsncoll(&a, &b, 1) < 0;
#else
        return coll->compare(&a, &a + 1, &b, &b + 1) < 0;
#endif
	}

	static auto compare(const CharType * s1, const CharType * s2, size_t n) {
#if _WIN32
        return _wcsncoll(s1, s2, n);
#else
        return coll->compare(s1, s1 + n, s2, s2 + n);
#endif
	}

	//static auto compare(const CharType * s1, size_t n1, const CharType * s2, size_t n2) {
	//	return coll->compare(s1, s1 + n1, s2, s2 + n2);
	//}

    template <typename T>
    static auto compare(const T & s1, const T & s2) {
#if _WIN32
        return wcscoll(&s1[0], &s2[0]);
#else
		return coll->compare(&s1[0], &s1[0] + s1.size(), &s2[0], &s2[0] + s2.size());
#endif
	}

	static thread_local const std::collate<CharType> * coll;

    locale_traits() {
        if
#if __cpp_if_constexpr >= 201606 || __cplusplus >= 201606
        constexpr
#endif
        ( std::is_same<char, CharType>::value ) {
            coll = &std::use_facet<std::collate<char>>(std::locale());
        }
    }
};
//------------------------------------------------------------------------------
#endif
//------------------------------------------------------------------------------
#if _WIN32
//------------------------------------------------------------------------------
#if __GNUC__ >= 5 || _MSC_VER
//------------------------------------------------------------------------------
struct char_traits : public std::char_traits<wchar_t> {
    static int compare(const char_type * _First1, const char_type * _First2, size_t _Count) {
        return _wcsncoll(_First1, _First2, _Count / sizeof(char_type));
    }
};
//typedef std::basic_string<wchar_t, char_traits, std::allocator<wchar_t> > string;
//typedef std::basic_stringstream<wchar_t, char_traits, std::allocator<wchar_t> > stringstream;
//------------------------------------------------------------------------------
#endif
//------------------------------------------------------------------------------
typedef std::wstring string;
typedef std::wstringstream stringstream;
typedef std::wregex regex;
//------------------------------------------------------------------------------
inline string operator + (const wchar_t * s1, const string & s2) {
    return string(s1) + s2;
}
//------------------------------------------------------------------------------
inline string operator + (const string & s1, const std::wstring & s2) {
    return s1 + s2.c_str();
}
//------------------------------------------------------------------------------
#else
//------------------------------------------------------------------------------
//typedef std::basic_string<char, locale_traits<char>> string;
//typedef std::basic_stringstream<char, locale_traits<char>> stringstream;
//typedef std::basic_regex<char, locale_traits<char>> regex;
typedef std::string string;
typedef std::stringstream stringstream;
typedef std::regex regex;
//------------------------------------------------------------------------------
#endif
//------------------------------------------------------------------------------
std::wstring str2wstr(const std::string & str);
std::string wstr2str(const std::wstring & str);
//------------------------------------------------------------------------------
#if _WIN32
//------------------------------------------------------------------------------
std::string str2utf(const string & str);
string utf2str(const std::string & str);
//------------------------------------------------------------------------------
#else
//------------------------------------------------------------------------------
constexpr const std::string & str2utf(const string & str)
{
    return str;
}
//------------------------------------------------------------------------------
constexpr const string & utf2str(const std::string & str)
{
    return str;
}
//------------------------------------------------------------------------------
#endif
//------------------------------------------------------------------------------
inline auto slen(const char * s) {
    return ::strlen(s);
}
//------------------------------------------------------------------------------
inline auto slen(const wchar_t * s) {
    return ::wcslen(s);
}
//------------------------------------------------------------------------------
namespace tests {
//------------------------------------------------------------------------------
void locale_traits_test();
//------------------------------------------------------------------------------
} // namespace tests
//------------------------------------------------------------------------------
} // namespace spacenet
//------------------------------------------------------------------------------
#endif // LOCALE_TRAITS_HPP_INCLUDED
//------------------------------------------------------------------------------
