/*-
 * The MIT License (MIT)
 *
 * Copyright (c) 2014, 2015, 2016 Guram Duka
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
#include "numeric/id.hpp"
//------------------------------------------------------------------------------
namespace nn {  // namespace NaturalNumbers
//------------------------------------------------------------------------------
nn_integer_data nn_izero(0);
nn_integer_data nn_ione(1);
nn_integer_data nn_itwo(2);
nn_integer_data nn_ifour(4);
nn_integer_data nn_ifive(5);
nn_integer_data nn_isix(6);
nn_integer_data nn_ieight(8);
nn_integer_data nn_iten(10);
// 1000000000u								== 0x3B9ACA00
// 10000000000000000000u					== 0x8AC7230489E80000
// 100000000000000000000000000000000000000u	== 0x4B3B4CA85A86C47A098A224000000000
#if SIZEOF_WORD == 1
//nn_integer_data nn_maxull = { 1, 4, 0, { 0x00, 0xCA, 0x9A, 0x3B } };
nn_integer_data nn_maxull(uint_max<umaxword_t>());
#elif SIZEOF_WORD == 2
//nn_integer_data nn_maxull = { 1, 4, 0, { 0x0000, 0x89E8, 0x2304, 0x8AC7 } };
nn_integer_data nn_maxull(uint_max<umaxword_t>());
#elif SIZEOF_WORD == 4
//nn_integer_data nn_maxull = { 1, 2, 0, { 0x89E80000, 0x8AC72304 } };
nn_integer_data nn_maxull(uint_max<umaxword_t>());
#elif SIZEOF_WORD == 8
//nn_integer_data nn_maxull = { 1, 2, 0, { 0x098A224000000000ull, 0x4B3B4CA85A86C47Aull } };
nn_integer_data nn_maxull(uint_max<umaxword_t>());
#endif
//------------------------------------------------------------------------------
#if ENABLE_STATISTICS
#   if DISABLE_ATOMIC_COUNTER
static uintptr_t integer::stat_iadd_ = 0;
static uintptr_t integer::stat_isub_ = 0;
static uintptr_t integer::stat_imul_ = 0;
static uintptr_t integer::stat_idiv_ = 0;
#   else
static std::atomic_uintptr_t integer::stat_iadd_(0);
static std::atomic_uintptr_t integer::stat_isub_(0);
static std::atomic_uintptr_t integer::stat_imul_(0);
static std::atomic_uintptr_t integer::stat_idiv_(0);
#   endif
#endif
//------------------------------------------------------------------------------
} // namespace NaturalNumbers
//------------------------------------------------------------------------------
