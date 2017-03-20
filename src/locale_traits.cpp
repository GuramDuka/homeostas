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
#if _WIN32
#include <windows.h>
#endif
//------------------------------------------------------------------------------
#include "locale_traits.hpp"
//------------------------------------------------------------------------------
namespace spacenet {
//------------------------------------------------------------------------------
#if __GNUC__ >= 5 || _MSC_VER
//------------------------------------------------------------------------------
template <> thread_local const std::collate<char> * locale_traits<char>::coll =
	&std::use_facet<std::collate<char>>(std::locale());
//------------------------------------------------------------------------------
static std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> converter;
//------------------------------------------------------------------------------
#endif
//------------------------------------------------------------------------------
std::wstring str2wstr(const std::string & str)
{
    try {
#if _WIN32
        size_t sz = str.size();

        if( sz > INT_MAX )
            throw std::range_error("String is too big");

        size_t charsNeeded = MultiByteToWideChar(
            CP_UTF8, 0, str.c_str(), int(sz), NULL, 0);

        if( charsNeeded == 0 )
            throw std::range_error("Failed converting UTF-8 string to UTF-16");

        std::wstring buffer;

        buffer.resize(charsNeeded);

        int charsConverted = MultiByteToWideChar(
            CP_UTF8, 0, str.c_str(), int(sz), &buffer[0], int(buffer.size()));

        if( charsConverted == 0 )
            throw std::range_error("Failed converting UTF-8 string to UTF-16");

        return buffer;
#elif __GNUC__ >= 5
        return converter.from_bytes(str);
#else
        throw std::range_error("Not implemented");
#endif
    }
    catch( std::range_error & ) {
        size_t length = str.length();
        std::wstring result;

        result.reserve(length);

        for( auto c : str )
            result.push_back(c);

        return result;
    }
}
//------------------------------------------------------------------------------
#if __GNUC__ >= 5 || _MSC_VER
//------------------------------------------------------------------------------
std::string wstr2str(const std::wstring & str)
{
    return converter.to_bytes(str);
}
//------------------------------------------------------------------------------
#endif
//------------------------------------------------------------------------------
#if _WIN32 && (__GNUC__ >= 5 || _MSC_VER)
//------------------------------------------------------------------------------
std::string str2utf(const string & str)
{
    return converter.to_bytes(str.c_str());
}
//------------------------------------------------------------------------------
string utf2str(const std::string & str)
{
    try {
        size_t sz = str.size();

        if( sz > INT_MAX )
            throw std::range_error("String is too big");

        size_t charsNeeded = MultiByteToWideChar(
            CP_UTF8, 0, str.c_str(), int(sz), NULL, 0);

        if( charsNeeded == 0 )
            throw std::range_error("Failed converting UTF-8 string to UTF-16");

        string buffer;

        buffer.resize(charsNeeded);

        int charsConverted = MultiByteToWideChar(
            CP_UTF8, 0, str.c_str(), int(sz), &buffer[0], int(buffer.size()));

        if( charsConverted == 0 )
            throw std::range_error("Failed converting UTF-8 string to UTF-16");

        return buffer;
    }
    catch( std::range_error & ) {
        size_t length = str.length();
        string result;

        result.reserve(length);

        for( auto c : str )
            result.push_back(c);

        return result;
    }
}
//------------------------------------------------------------------------------
#endif
//------------------------------------------------------------------------------
} // namespace spacenet
//------------------------------------------------------------------------------
