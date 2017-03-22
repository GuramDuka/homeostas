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
namespace homeostas {
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
} // namespace homeostas
//------------------------------------------------------------------------------
