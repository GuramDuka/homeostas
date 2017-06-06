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
#include "std_ext.hpp"
//------------------------------------------------------------------------------
namespace homeostas {
//------------------------------------------------------------------------------
////////////////////////////////////////////////////////////////////////////////
//------------------------------------------------------------------------------
inline auto slen(const char * s) {
    return ::strlen(s);
}
//------------------------------------------------------------------------------
inline auto slen(const wchar_t * s) {
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
namespace tests {
//------------------------------------------------------------------------------
void locale_traits_test();
//------------------------------------------------------------------------------
} // namespace tests
//------------------------------------------------------------------------------
} // namespace homeostas
//------------------------------------------------------------------------------
#endif // LOCALE_TRAITS_HPP_INCLUDED
//------------------------------------------------------------------------------
