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
#include "std_ext.hpp"
//------------------------------------------------------------------------------
namespace homeostas {
//------------------------------------------------------------------------------
////////////////////////////////////////////////////////////////////////////////
//------------------------------------------------------------------------------
template <size_t SIZE = 64>
struct integer {
    typedef uint32_t  word;
    typedef int32_t  sword;
    typedef uint64_t dword;

    enum {
        length = SIZE / sizeof(word),
        bit_size = length * sizeof(word) * CHAR_BIT
    };

    word lo_dummy_ = 0;
    word data_[length];
    word hi_dummy_ = 0;

    integer() {
    }

    integer(size_t v) {
        memset(this, 0, sizeof(*this));
        *reinterpret_cast<size_t *>(data_) = v;
    }

    integer & operator = (size_t v) {
        memset(this, 0, sizeof(*this));
        *reinterpret_cast<size_t *>(data_) = v;
        return *this;
    }

    operator size_t () const {
        return *reinterpret_cast<const size_t *>(data_);
    }

    operator bool () const {
        return !z();
    }

    sword isign() const {
        return ((sword) data_[length]) >> (sizeof(word) * CHAR_BIT - 1);
    }

    integer isub(const integer & p1) const {
        integer result;

        word cf = 0;
        word * r = result.data_;
        const word * e = r + length;
        const word * d0 = data_;
        const word * d1 = p1.data_;
        word s0 = d0[length];
        word s1 = d1[length];
        dword q = 0;

        while( r < e ) {
            q = (dword) *d0++ - *d1++ - cf;
            cf = (q >> sizeof(word) * CHAR_BIT) & 1;
            *r++ = (word) q;
        }

        *r = (word) (q = (dword) s0 - s1 - cf);

        return result;
    }

    integer operator - (const integer & v) const {
        return isub(v);
    }

    integer & operator -= (const integer & v) {
        return *this = isub(v);
    }

    integer isal(uintptr_t bit_count) const {
        if( bit_count == 0 )
            return *this;

        integer result;

        if( bit_count >= bit_size ) {
            memset(result.data_, 0, sizeof(result.data_));
            return result;
        }

        uintptr_t new_bit_size = length * sizeof(word) * CHAR_BIT + bit_count;
        new_bit_size += -(intptr_t) new_bit_size & (sizeof(word) * CHAR_BIT - 1);
        uintptr_t new_length = new_bit_size / (sizeof(word) * CHAR_BIT);

        uintptr_t offset = bit_count / (sizeof(word) * CHAR_BIT);
        uintptr_t shift = bit_count & (sizeof(word) * CHAR_BIT - 1);

        union {
            word * w;
            dword * d;
        } dst;

        dst.w = result.data_ + new_length - 2;

        union {
            const word * w;
            const dword * d;
        } src;

        src.w = data_ + length - 1;

        memset(result.data_, 0, offset * sizeof(word));

        if( shift == 0 ) {
            memcpy(result.data_ + offset, data_, length * sizeof(word));
        }
        else {
            for( intptr_t i = length - 1; i >= 0; i--, dst.w--, src.w-- )
                *dst.d = (*src.d) << shift;
        }

        return result;
    }

    integer operator << (uintptr_t bit_count) const {
        return isal(bit_count);
    }

    integer & operator <<= (uintptr_t bit_count) {
        return *this = isal(bit_count);
    }

    bool z() const { // is zero
        if( data_[0] != 0 )
            return false;

        for( intptr_t i = length; i > 0; i-- )
            if( data_[i] != 0 )
                return false;

        return true;
    }

    bool operator ! () const {
        return z();
    }

    intptr_t icompare_value() const {
        return isign() < 0 ? -1 : z() ? 0 : 1;
    }

    intptr_t icompare(const integer & v) const {
        return (*this - v).icompare_value();
    }

    bool operator >= (const integer & v) const {
        return (*this - v).icompare_value() >= 0;
    }

    template <typename T = word>
    void bit(uintptr_t i, const T v) {
        data_[i / (sizeof(word) * CHAR_BIT)] |= word(v) << (i & (sizeof(word) * CHAR_BIT - 1));
    }

    uintptr_t bit(uintptr_t i) const {
        return data_[i / (sizeof(word) * CHAR_BIT)] >> (i & (sizeof(word) * CHAR_BIT - 1)) & 1;
    }

    integer div(const integer & divider, integer * p_mod) const {
        integer q, temp_r, & r = p_mod == NULL ? temp_r : *p_mod;

        memset(q.data_, 0, sizeof(q.data_));
        memset(r.data_, 0, sizeof(r.data_));

        for( intptr_t i = length * sizeof(word) * CHAR_BIT - 1; i >= 0; i-- ){
            r <<= 1;
            r.bit(0, bit(i));

            if( r >= divider ){
                r -= divider;
                q.bit(i, 1);
            }
        }

        return q;
    }
};
//------------------------------------------------------------------------------
////////////////////////////////////////////////////////////////////////////////
//------------------------------------------------------------------------------
struct cdc512 : public std::key512 {
    typedef std::key512 base;

    /*auto begin() const {
        return base::begin();
    }

    auto begin() {
        return base::begin();
    }

    auto end() const {
        return base::end();
    }

    auto end() {
        return base::end();
    }*/

    uint64_t p;

    cdc512() : std::key512(std::leave_uninitialized) {}
    cdc512(std::leave_uninitialized_type t) : std::key512(t) {}
    cdc512(std::zero_initialized_type t) : std::key512(t) {}

    cdc512(const std::key512 & o) : std::key512(o), p(0) {}

    cdc512 & operator = (const std::key512 & o) {
        std::key512::operator = (o);
        p = 0;
        return *this;
    }

    cdc512(const cdc512 & o) : std::key512(o), p(o.p) {}

    cdc512 & operator = (const cdc512 & o) {
        std::key512::operator = (o);
        p = o.p;
        return *this;
    }

    template <class InitIt, class InputIt>
    cdc512(InitIt i_first, InitIt i_last, InputIt first, InputIt last) : std::key512(std::leave_uninitialized) {
        init(i_first, i_last);
        update(first, last);
        final();
    }

    template <class InputIt>
    cdc512(InputIt first, InputIt last) : std::key512(std::leave_uninitialized) {
        init();
        update(first, last);
        final();
    }

    template <typename InputIt, typename
        std::enable_if<std::is_same<typename std::iterator_traits<InputIt>::value_type, value_type>::value>::type * = nullptr
    >
    auto & init(InputIt first, InputIt last) {
        std::copy(first, last, begin(), end());
        p = 0;
        return *this;
    }

    template <typename InputIt, typename
        std::enable_if<!std::is_same<typename std::iterator_traits<InputIt>::value_type, value_type>::value>::type * = nullptr
    >
    auto & init(InputIt first, InputIt last) {
        std::transform(first, last, begin(), end(), [] (const auto & a) {
            return value_type(a);
        });
        p = 0;
        return *this;
    }

    cdc512 & init(const std::key512 & o) {
        std::key512::operator = (o);
        p = 0;
        return *this;
    }

    template <typename InputIt, typename
        std::enable_if<std::is_same<typename std::iterator_traits<InputIt>::iterator_category, std::random_access_iterator_tag>::value>::type * = nullptr
    >
    auto & update(InputIt first, InputIt last) {
        return update(&*first, (last - first) * sizeof(*first));
    }

    cdc512 & init();
    cdc512 & update(const void * data, uintptr_t size);
    cdc512 & final();

    /*std::string to_string() const;

    std::string to_short_string(
        const char * abc = nullptr,
        char delimiter = '\0',
        size_t interval = 0) const;

    void from_short_string(const std::string & s, const char * abc = nullptr);*/

    cdc512 & generate_entropy_fast();
    cdc512 & generate_entropy(std::vector<uint8_t> * p_entropy = nullptr);
};
//------------------------------------------------------------------------------
namespace tests {
//------------------------------------------------------------------------------
void cdc512_test();
//------------------------------------------------------------------------------
} // namespace tests
//------------------------------------------------------------------------------
} // namespace homeostas
//------------------------------------------------------------------------------
#if __GNUC__ > 0 && __GNUC__ < 5 && __ANDROID__
//------------------------------------------------------------------------------
namespace std {
//------------------------------------------------------------------------------
inline string to_string(const homeostas::cdc512 & b) {
    return to_string(*static_cast<const key512 *>(&b));
}
//------------------------------------------------------------------------------
} // namespace std
//------------------------------------------------------------------------------
#endif
//------------------------------------------------------------------------------
#endif // CDC512_HPP_INCLUDED
//------------------------------------------------------------------------------
