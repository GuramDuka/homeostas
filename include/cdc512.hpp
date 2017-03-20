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
#ifndef CDC512_HPP_INCLUDED
#define CDC512_HPP_INCLUDED
//------------------------------------------------------------------------------
#pragma once
//------------------------------------------------------------------------------
#include "config.h"
//------------------------------------------------------------------------------
#include <iterator>
//------------------------------------------------------------------------------
#include "locale_traits.hpp"
//------------------------------------------------------------------------------
namespace spacenet {
//------------------------------------------------------------------------------
////////////////////////////////////////////////////////////////////////////////
//------------------------------------------------------------------------------
struct cdc512_data {
    union {
        struct {
            uint64_t a, b, c, d, e, f, g, h;
        };
        uint64_t digest64[8];
        uint8_t digest[sizeof(uint64_t) * 8];
    };

    void shuffle();
    void shuffle(const cdc512_data & v);
};
//---------------------------------------------------------------------------
struct cdc512 : public cdc512_data {
    uint64_t p;

    cdc512() {
        init();
    }
    
    cdc512(leave_uninitialized_type) {}
    
    template <class InputIt>
    cdc512(InputIt first, InputIt last) {
        init();
        update(first, last);
        finish();
    }

    template <class InputIt, typename Container>
    cdc512(Container & c, InputIt first, InputIt last) {
        init();
        update(first, last);
        finish();
        c.assign(std::begin(digest), std::end(digest));
    }

    void init();
    void update(const void * data, uintptr_t size);
    void finish();

    template <typename Container>
    void finish(Container & c) {
        finish();
        c.assign(std::begin(digest), std::end(digest));
    }

    template <typename InputIt>
    void update(InputIt first, InputIt last) {
        update(&(*first), (last - first) * sizeof(*first));
    }

    template <typename InputIt, typename Container>
    void flush(Container & c, InputIt first, InputIt last) {
        update(first, last);
        finish();
        c.assign(std::begin(digest), std::end(digest));
    }
    
    auto begin() {
        return std::begin(digest);
    }

    auto end() {
        return std::end(digest);
    }
    
    std::string to_string() const;
    std::string to_short_string() const;
    static std::string generate_prime();
};
//------------------------------------------------------------------------------
namespace tests {
//------------------------------------------------------------------------------
void cdc512_test();
//------------------------------------------------------------------------------
} // namespace tests
//------------------------------------------------------------------------------
} // namespace spacenet
//------------------------------------------------------------------------------
#endif // CDC512_HPP_INCLUDED
//------------------------------------------------------------------------------
