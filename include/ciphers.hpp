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
#ifndef CIPHERS_HPP_INCLUDED
#define CIPHERS_HPP_INCLUDED
//------------------------------------------------------------------------------
#pragma once
//------------------------------------------------------------------------------
#include "config.h"
//------------------------------------------------------------------------------
#include "port.hpp"
#include "locale_traits.hpp"
#include "std_ext.hpp"
#include "scope_exit.hpp"
#include "cdc512.hpp"
#include "rand.hpp"
//------------------------------------------------------------------------------
namespace homeostas {
//------------------------------------------------------------------------------
////////////////////////////////////////////////////////////////////////////////
//------------------------------------------------------------------------------
struct light_cipher : protected cdc512 {
    light_cipher() : cdc512(leave_uninitialized) {}

    void init(const std::key512 & key) {
        cdc512::operator =(key);
        mask_ring = std::end(mask);
    }

    template <typename InpIt, typename OutIt>
    void encode(InpIt first, InpIt last, OutIt d_first, OutIt d_last) {
        assert( std::distance(first, last) == std::distance(d_first, d_last) );

        std::transform(first, last, d_first, d_last, [&] (const auto & a) {
            if( mask_ring == std::end(mask) ) {
                mask = *this;
                this->update(this->begin(), this->end());
                mask_ring = std::begin(mask);
            }

            return (typename OutIt::value_type) (a ^ *mask_ring++);
        });
    }

    template <typename InpRange, typename OutRange>
    void encode(InpRange s_range, OutRange d_range) {
        encode(std::begin(s_range), std::end(s_range), std::begin(d_range), std::end(d_range));
    }

    std::key512 mask;
    decltype(std::begin(mask)) mask_ring;
};
//------------------------------------------------------------------------------
////////////////////////////////////////////////////////////////////////////////
//------------------------------------------------------------------------------
// rand<7, uint64_t> expected period is: 2^^8511
struct strong_cipher : protected rand<7, uint64_t> {
    typedef rand<7, uint64_t> base;

    void init(const std::key512 & key) {
        base::init(std::begin(key), std::end(key));
        mask_ring = std::end(mask);
    }

    template <typename InpIt, typename OutIt>
    void encode(InpIt first, InpIt last, OutIt d_first, OutIt d_last) {
        assert( std::distance(first, last) == std::distance(d_first, d_last) );

        std::transform(first, last, d_first, d_last, [&] (const auto & a) {
            if( mask_ring == std::end(mask) ) {
                *reinterpret_cast<value_type *>(mask) = this->get();
                mask_ring = std::begin(mask);
            }

            return (typename OutIt::value_type) (a ^ *mask_ring++);
        });
    }

    template <typename InpRange, typename OutRange>
    void encode(InpRange s_range, OutRange d_range) {
        encode(std::begin(s_range), std::end(s_range), std::begin(d_range), std::end(d_range));
    }

    uint8_t mask[sizeof(value_type)];
    decltype(std::begin(mask)) mask_ring;
};
//------------------------------------------------------------------------------
} // namespace homeostas
//------------------------------------------------------------------------------
#endif // CIPHERS_HPP_INCLUDED
//------------------------------------------------------------------------------
