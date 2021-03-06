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
#ifndef ID_HPP_INCLUDED
#define ID_HPP_INCLUDED
//------------------------------------------------------------------------------
#include "tlsf.hpp"
//------------------------------------------------------------------------------
namespace nn {  // namespace NaturalNumbers
//------------------------------------------------------------------------------
template <typename A, typename B>
static inline const A & imax(const A & a, const B & b)
{
	return a > b ? a : b;
}
//------------------------------------------------------------------------------
template <typename A, typename B>
static inline const A & imin(const A & a, const B & b)
{
	return a < b ? a : b;
}
//------------------------------------------------------------------------------
// autodetect maximum word size
#ifndef SIZEOF_WORD
#   if __GNUC__ && __x86_64__
#      define SIZEOF_WORD 8
#   endif
#endif
//------------------------------------------------------------------------------
#if SIZEOF_WORD == 1
typedef uint8_t word;
typedef int8_t sword;
typedef uint16_t dword;
typedef int16_t sdword;
#elif SIZEOF_WORD == 2
typedef uint16_t word;
typedef int16_t sword;
typedef uint32_t dword;
typedef int32_t sdword;
#elif SIZEOF_WORD == 8 && __GNUC__
typedef uint64_t word;
typedef int64_t sword;
typedef __uint128_t dword;
typedef __int128_t sdword;
#else
typedef uint32_t word;
typedef int32_t sword;
typedef uint64_t dword;
typedef int64_t sdword;
#if SIZEOF_WORD != 4
#undef SIZEOF_WORD
#define SIZEOF_WORD 4
#endif
#endif
//------------------------------------------------------------------------------
#if SIZEOF_WORD < 8
typedef uintmax_t umaxword_t;
#else
typedef __uint128_t umaxword_t;
#endif
//------------------------------------------------------------------------------
class integer;
class numeric;
//------------------------------------------------------------------------------
/////////////////////////////////////////////////////////////////////////////
//------------------------------------------------------------------------------
typedef class nn_integer_data {
    protected:
        nn_integer_data(intptr_t ref_count, uintptr_t length)
            : ref_count_(ref_count), length_(length), dummy_(0) {}
    public:
		~nn_integer_data() {}

        nn_integer_data(umaxword_t data)
            : ref_count_(1), length_(sizeof(data) / sizeof(data_[0])), dummy_(0) {
            for( size_t i = 0; i < sizeof(data) / sizeof(data_[0]); i++ ) {
#if BYTE_ORDER == LITTLE_ENDIAN
                data_[i] = (word) data;
#elif BYTE_ORDER == BIG_ENDIAN
                data_[i] = std::htole((word) data);
#endif
                data >>= sizeof(data_[0]) * CHAR_BIT;
            }
        }

#if DISABLE_ATOMIC_COUNTER
        mutable uintptr_t ref_count_;
#else
        mutable std::atomic_uintptr_t ref_count_;
#endif
        mutable uintptr_t length_;
#if SIZEOF_WORD == 1
        mutable uintptr_t dummy_;		// for shift down overflow bits
        mutable word data_[16];
#elif SIZEOF_WORD == 2
        mutable uintptr_t dummy_;		// for shift down overflow bits
        mutable word data_[8];
#elif SIZEOF_WORD == 4
        mutable uintptr_t dummy_;
        mutable word data_[4];
#elif SIZEOF_WORD == 8
        mutable uintptr_t dummy_;
        mutable word data_[4];
#else
#error Invalid macro SIZEOF_WORD
#endif

	typedef nn_integer_data * nn_integer;

	nn_integer add_ref() const {
	  ref_count_++;
	  return const_cast<nn_integer>(this);
	}

	static size_t get_size(uintptr_t length) {
		return offsetof(nn_integer_data, data_) + (length + 2) * sizeof(data_[0]);
	}

	static nn_integer nn_new(uintptr_t length) {
#if ENABLE_TLSF
        nn_integer p = (nn_integer) tlsf::TLSF::instance()->malloc(get_size(length));
#else
        nn_integer p = (nn_integer) std::malloc(get_size(length));
#endif

		if( p != nullptr )
			new (p) nn_integer_data(1, length);

		return p;
	}

	void release() {
        if( --ref_count_ == 0 ) {
            this->~nn_integer_data();
#if __GNUG__ && !__clang__
#   pragma GCC diagnostic push
#   pragma GCC diagnostic ignored "-Wfree-nonheap-object"
#endif
#   if ENABLE_TLSF
            tlsf::TLSF::instance()->free(this);
#   else
            std::free(this);
#   endif
#if __GNUG__ && !__clang__
#   pragma GCC diagnostic pop
#endif
        }
    }

	sword isign() const {
#if BYTE_ORDER == LITTLE_ENDIAN
        return ((sword) data_[length_]) >> (sizeof(word) * CHAR_BIT - 1);
#elif BYTE_ORDER == BIG_ENDIAN
        return ((sword) std::letoh(data_[length_])) >> (sizeof(word) * CHAR_BIT - 1);
#endif
	}

	word high_word() const {
#if BYTE_ORDER == LITTLE_ENDIAN
        return data_[length_ - 1];
#elif BYTE_ORDER == BIG_ENDIAN
        return std::letoh(data_[length_ - 1]);
#endif
	}

	void normalize() {
		while( length_ > 1 && data_[length_] == data_[length_ - 1] )
			length_--;
	}

	nn_integer addm(const nn_integer p1) const {
		nn_integer result = nn_new(p1->length_);

		dword q = 0;
		word cf = 0;
		word * r = result->data_;
		const word * e = r + length_;
		const word * d0 = data_;
		const word * d1 = p1->data_;
		word s0 = d0[length_];
		word s1 = d1[p1->length_];

		while( r < e ){
#if BYTE_ORDER == LITTLE_ENDIAN
            q = (dword) *d0++ + *d1++ + cf;
            cf = (q >> sizeof(word) * CHAR_BIT) & 1;
            *r++ = (word) q;
#elif BYTE_ORDER == BIG_ENDIAN
            q = (dword) std::letoh(*d0++) + std::letoh(*d1++) + cf;
            cf = (q >> sizeof(word) * CHAR_BIT) & 1;
            *r++ = std::htole((word) q);
#endif
		}

		e = result->data_ + p1->length_;

		while( r < e ){
			q = (dword) s0 + *d1++ + cf;
			cf = (q >> sizeof(word) * CHAR_BIT) & 1;
			*r++ = (word) q;
		}

		*r++ = (word) (q = (dword) s0 + s1 + cf);
		cf = (q >> sizeof(word) * CHAR_BIT) & 1;
		*r = (dword) s0 + s1 + cf;

		// if overflow then grow size
		result->length_++;
		result->normalize();

		return result;
	}

	nn_integer addz(const nn_integer p1) const {
		nn_integer result = nn_new(length_);

		dword q = 0;
		word cf = 0;
		word * r = result->data_;
		const word * e = r + result->length_;
		const word * d0 = data_;
		const word * d1 = p1->data_;
		word s0 = d0[length_];
		word s1 = d1[p1->length_];

		while( r < e ){
			q = (dword) *d0++ + *d1++ + cf;
			cf = (q >> sizeof(word) * CHAR_BIT) & 1;
			*r++ = (word) q;
		}

		*r++ = (word) (q = (dword) s0 + s1 + cf);
		cf = (q >> sizeof(word) * CHAR_BIT) & 1;
		*r = (dword) s0 + s1 + cf;

		// if overflow then grow size
		result->length_++;
		result->normalize();

		return result;
	}

	nn_integer addp(const nn_integer p1) const {
		nn_integer result = nn_new(length_);

		dword q = 0;
		word cf = 0;
		word * r = result->data_;
		const word * e = r + p1->length_;
		const word * d0 = data_;
		const word * d1 = p1->data_;
		word s0 = d0[length_];
		word s1 = d1[p1->length_];

		while( r < e ){
			q = (dword) *d0++ + *d1++ + cf;
			cf = (q >> sizeof(word) * CHAR_BIT) & 1;
			*r++ = (word) q;
		}

		e = result->data_ + length_;

		while( r < e ){
			q = (dword) *d0++ + s1 + cf;
			cf = (q >> sizeof(word) * CHAR_BIT) & 1;
			*r++ = (word) q;
		}

		*r++ = (word) (q = (dword) s0 + s1 + cf);
		cf = (q >> sizeof(word) * CHAR_BIT) & 1;
		*r = (dword) s0 + s1 + cf;

		// if overflow then grow size
		result->length_++;
		result->normalize();

		return result;
	}

	nn_integer subm(const nn_integer p1) const {
		nn_integer result = nn_new(p1->length_);

		dword q = 0;
		word cf = 0;
		word * r = result->data_;
		const word * e = r + length_;
		const word * d0 = data_;
		const word * d1 = p1->data_;
		word s0 = d0[length_];
		word s1 = d1[p1->length_];

		while( r < e ){
			q = (dword) *d0++ - *d1++ - cf;
			cf = (q >> sizeof(word) * CHAR_BIT) & 1;
			*r++ = (word) q;
		}

		e = result->data_ + p1->length_;

		while( r < e ){
			q = (dword) s0 - *d1++ - cf;
			cf = (q >> sizeof(word) * CHAR_BIT) & 1;
			*r++ = (word) q;
		}

		*r++ = (word) (q = (dword) s0 - s1 - cf);
		cf = (q >> sizeof(word) * CHAR_BIT) & 1;
		*r = (dword) s0 - s1 - cf;

		// if overflow then grow size
		result->length_++;
		result->normalize();

		return result;
	}

	nn_integer subz(const nn_integer p1) const {
		nn_integer result = nn_new(length_);

		dword q = 0;
		word cf = 0;
		word * r = result->data_;
		const word * e = r + result->length_;
		const word * d0 = data_;
		const word * d1 = p1->data_;
		word s0 = d0[length_];
		word s1 = d1[p1->length_];

		while( r < e ){
			q = (dword) *d0++ - *d1++ - cf;
			cf = (q >> sizeof(word) * CHAR_BIT) & 1;
			*r++ = (word) q;
		}

		*r++ = (word) (q = (dword) s0 - s1 - cf);
		cf = (q >> sizeof(word) * CHAR_BIT) & 1;
		*r = (dword) s0 - s1 - cf;

		// if overflow then grow size
		result->length_++;
		result->normalize();

		return result;
	}

	nn_integer subp(const nn_integer p1) const {
		nn_integer result = nn_new(length_);

		dword q = 0;
		word cf = 0;
		word * r = result->data_;
		const word * e = r + p1->length_;
		const word * d0 = data_;
		const word * d1 = p1->data_;
		word s0 = d0[length_];
		word s1 = d1[p1->length_];

		while( r < e ){
			q = (dword) *d0++ - *d1++ - cf;
			cf = (q >> sizeof(word) * CHAR_BIT) & 1;
			*r++ = (word) q;
		}

		e = result->data_ + length_;

		while( r < e ){
			q = (dword) *d0++ - s1 - cf;
			cf = (q >> sizeof(word) * CHAR_BIT) & 1;
			*r++ = (word) q;
		}

		*r++ = (word) (q = (dword) s0 - s1 - cf);
		cf = (q >> sizeof(word) * CHAR_BIT) & 1;
		*r = (dword) s0 - s1 - cf;

		// if overflow then grow size
		result->length_++;
		result->normalize();

		return result;
	}

	nn_integer iadd(const nn_integer v) const {
		intptr_t c = length_ - v->length_;

		if( c > 0 )
			return addp(v);

		if( c < 0 )
			return addm(v);

		return addz(v);
	}

	nn_integer isub(const nn_integer v) const {
		intptr_t c = length_ - v->length_;

		if( c > 0 )
			return subp(v);

		if( c < 0 )
			return subm(v);

		return subz(v);
	}

	nn_integer isal(uintptr_t bit_count) const;
	nn_integer isar(uintptr_t bit_count) const;

	bool z() const { // is zero
		if( data_[0] != 0 )
			return false;

		for( intptr_t i = length_; i > 0; i-- )
			if( data_[i] != 0 )
				return false;

		return true;
	}

	bool nz() const { // iz not zero
		for( intptr_t i = length_; i >= 0; i-- )
			if( data_[i] != 0 )
				return true;

		return false;
	}

	intptr_t icompare(const nn_integer v1) const {

		intptr_t c = 0;

		//if( isign() < 0 ) {
		//	if( v1->isign() < 0 )
		//		high_word()

		if( c == 0 ) {
			nn_integer a = isub(v1);

			c = a->isign() < 0 ? -1 : a->z() ? 0 : 1;

			a->release();

		}

		return c;
	}

	void accumulate_with_carry(uintptr_t index, word value) {
		dword q;

		while( value != 0 && index < length_ ) {
			q = (dword) data_[index] + value;
			data_[index] = (word) q;
			value = word(q >> (sizeof(word) * CHAR_BIT));
			index++;
		}
	}

	void imul(const nn_integer b, uintptr_t i, word m) {
		for( uintptr_t j = 0; j < b->length_; j++ ) {
			if( b->data_[j] == 0 )
				continue;

			dword q = (dword) m * b->data_[j];

			accumulate_with_carry(i + j, word(q));
			accumulate_with_carry(i + j + 1, word(q >> (sizeof(word) * CHAR_BIT)));
		}
	}

	void imul(const nn_integer a, const nn_integer b) {
		for( uintptr_t i = 0; i < a->length_; i++ ) {
			if( a->data_[i] == 0 )
				continue;

			//for( uintptr_t j = 0; j < b.proxy_->length_; j++ ) {
			//	if( b.proxy_->data_[j] == 0 )
			//		continue;

			//	union {
			//		struct {
			//			word lo;
			//			word hi;
			//		};
			//		dword q;
			//	};
			//	q = (dword) a.proxy_->data_[i] * b.proxy_->data_[j];

			//	r->accumulate_with_carry(i + j, lo);
			//	r->accumulate_with_carry(i + j + 1, hi);
			//}
			imul(b, i, a->data_[i]);

		}
	}

	nn_integer inot() const {
		nn_integer result = nn_new(length_);

		for( intptr_t i = length_ + 1; i >= 0; i-- )
			result->data_[i] = ~data_[i];

		return result;
	}

	nn_integer iand(const nn_integer v) const {
		nn_integer result;
		uintptr_t i;

		if( length_ > v->length_ ){
			result = nn_new(length_);

			for( i = 0; i < v->length_; i++ )
				result->data_[i] = data_[i] & v->data_[i];

			while( i < length_ + 2 ){
				result->data_[i] = data_[i] & v->data_[v->length_];
				i++;
			}
		}
		else if( length_ < v->length_ ){
			result = nn_new(v->length_);

			for( i = 0; i < length_; i++ )
				result->data_[i] = data_[i] & v->data_[i];

			while( i < v->length_ + 2 ){
				result->data_[i] = data_[length_] & v->data_[i];
				i++;
			}
		}
		else {
			result = nn_new(length_);

			for( i = 0; i < length_ + 2; i++ )
				result->data_[i] = data_[i] & v->data_[i];
		}

		return result;
	}

	nn_integer ior(const nn_integer v) const {
		nn_integer result;
		uintptr_t i;

		if( length_ > v->length_ ){
			result = nn_new(length_);

			for( i = 0; i < v->length_; i++ )
				result->data_[i] = data_[i] | v->data_[i];

			while( i < length_ + 2 ){
				result->data_[i] = data_[i] | v->data_[v->length_];
				i++;
			}
		}
		else if( length_ < v->length_ ){
			result = nn_new(v->length_);

			for( i = 0; i < length_; i++ )
				result->data_[i] = data_[i] | v->data_[i];

			while( i < v->length_ + 2 ){
				result->data_[i] = data_[length_] | v->data_[i];
				i++;
			}
		}
		else {
			result = nn_new(length_);

			for( i = 0; i < length_ + 2; i++ )
				result->data_[i] = data_[i] | v->data_[i];
		}

		return result;
	}

	nn_integer ixor(const nn_integer v) const {
		nn_integer result;
		uintptr_t i;

		if( length_ > v->length_ ){
			result = nn_new(length_);

			for( i = 0; i < v->length_; i++ )
				result->data_[i] = data_[i] ^ v->data_[i];

			while( i < length_ + 2 ){
				result->data_[i] = data_[i] ^ v->data_[v->length_];
				i++;
			}
		}
		else if( length_ < v->length_ ){
			result = nn_new(v->length_);

			for( i = 0; i < length_; i++ )
				result->data_[i] = data_[i] ^ v->data_[i];

			while( i < v->length_ + 2 ){
				result->data_[i] = data_[length_] ^ v->data_[i];
				i++;
			}
		}
		else {
			result = nn_new(length_);

			for( i = 0; i < length_ + 2; i++ )
				result->data_[i] = data_[i] ^ v->data_[i];
		}

		return result;
	}

	uint8_t ibit(uintptr_t i) const {
		assert( i < length_ * sizeof(word) * CHAR_BIT );
		return data_[i / (sizeof(word) * CHAR_BIT)] >> (i & (sizeof(word) * CHAR_BIT - 1)) & 1;
	}

	template <typename T = word>
	void sbit(uintptr_t i,const T v = 1) const {
		assert( i < length_ * sizeof(word) * CHAR_BIT );
		data_[i / (sizeof(word) * CHAR_BIT)] |= word(v) << (i & (sizeof(word) * CHAR_BIT - 1));
	}

	uintptr_t count_trailing_zeros() const {
		uintptr_t n;
		word * p = data_;

		for( n = 0; n < length_ * sizeof(word) * CHAR_BIT; n += sizeof(word) * CHAR_BIT, p++ ){
			word x = *p;

			if( x != 0 ){
#if __GNUC__ && SIZEOF_WORD <= 4
				n += __builtin_ctz(x);
#elif __GNUC__ && SIZEOF_WORD == 8
				n += __builtin_ctzll(x);
#else
				uintptr_t a = 1;
#if SIZEOF_WORD >= 8
				if( (x & 0xFFFFFFFF) == 0 ) { a = a + 32; x = x >> 32; }
#endif
#if SIZEOF_WORD >= 4
				if( (x & 0x0000FFFF) == 0 ) { a = a + 16; x = x >> 16; }
#endif
#if SIZEOF_WORD >= 2
				if( (x & 0x000000FF) == 0 ) { a = a +  8; x = x >>  8; }
#endif
				if( (x & 0x0000000F) == 0 ) { a = a +  4; x = x >>  4; }
				if( (x & 0x00000003) == 0 ) { a = a +  2; x = x >>  2; }

				n += a - (x & 1);
#endif
				break;
			}
		}

		return n;
	}

} nn_integer_data;
//------------------------------------------------------------------------------
typedef nn_integer_data::nn_integer nn_integer;
//------------------------------------------------------------------------------
#if !__clang__
/* Quick and dirty conversion from a single character to its hex equivelent */
constexpr uint8_t hex_char2int(char input)
{
    return
    ((input >= 'a') && (input <= 'f'))
    ? (input - 'a')
    : ((input >= 'A') && (input <= 'F'))
    ? (input - 'A')
    : ((input >= '0') && (input <= '9'))
    ? (input - '0')
    : throw std::exception{};
}

/* Position the characters into the appropriate nibble */
constexpr uint8_t hex_char(char high, char low)
{
    return (hex_char2int(high) << 4) | (hex_char2int(low));
}

/* Adapter that performs sets of 2 characters into a single byte and combine the results into a uniform initialization list used to initialize T */
template <typename T, size_t length, size_t ... index>
constexpr T hex_string(const char (&input)[length], const std::index_sequence<index...>&)
{
    return T{hex_char(input[index * 2], input[index * 2 + 1])...};
}

/* Entry function */
template <typename T, std::size_t length>
constexpr T hex_string(const char (&input)[length])
{
    return hex_string<T>(input, std::make_index_sequence<(length / 2)>{});
}

constexpr auto Y = hex_string<std::array<std::uint8_t, 3>>("ABCDEF");
#endif

#if (__GNUC__ > 0 && __GNUC__ < 5) || _MSC_VER
template <typename T> constexpr const T uint_max(const T m = T(1))
{
	return (m << 3) + (m << 1) > m ? uint_max<T>((m << 3) + (m << 1)) : m;
}
#else
template <typename T> constexpr const T uint_max()
{
	T m = 1u;

	for (;;) {
		T n = T(m * 10u);
		if( n <= m ) // overflow
			break;
		m = n;
	}

	return m;
}
#endif

extern nn_integer_data nn_izero;
extern nn_integer_data nn_ione;
extern nn_integer_data nn_itwo;
extern nn_integer_data nn_ifour;
extern nn_integer_data nn_ifive;
extern nn_integer_data nn_isix;
extern nn_integer_data nn_ieight;
extern nn_integer_data nn_iten;
// 1000000000u								== 0x3B9ACA00
// 10000000000000000000u					== 0x8AC7230489E80000
// 100000000000000000000000000000000000000u	== 0x4B3B4CA85A86C47A098A224000000000
#if SIZEOF_WORD == 1
extern nn_integer_data nn_maxull;
#elif SIZEOF_WORD == 2
extern nn_integer_data nn_maxull;
#elif SIZEOF_WORD == 4
extern nn_integer_data nn_maxull;
#elif SIZEOF_WORD == 8
extern nn_integer_data nn_maxull;
#endif
//------------------------------------------------------------------------------
static inline nn_integer nn_new(uintptr_t length)
{
	nn_integer p = nn_integer_data::nn_new(length);

    if( p == nullptr || length == 0 )
		throw std::bad_alloc();

	return p;
}
//------------------------------------------------------------------------------
static inline nn_integer nn_init_illong(long long v)
{
	switch( v ){
		case   0 : return nn_izero.add_ref();
		case   1 : return nn_ione.add_ref();
		case   2 : return nn_itwo.add_ref();
		case   4 : return nn_ifour.add_ref();
		case   5 : return nn_ifive.add_ref();
		case   6 : return nn_isix.add_ref();
		case   8 : return nn_ieight.add_ref();
		case  10 : return nn_iten.add_ref();
		default  : ;
	}

	nn_integer p = nn_new(sizeof(v) / sizeof(word));

	memcpy(p->data_, &v, sizeof(v));
	p->data_[p->length_] = v >> (sizeof(v) * CHAR_BIT - 1);
	p->normalize();

	return p;
}
//------------------------------------------------------------------------------
static inline nn_integer nn_init_iullong(unsigned long long v)
{
	switch( v ){
		case   0 : return nn_izero.add_ref();
		case   1 : return nn_ione.add_ref();
		case   2 : return nn_itwo.add_ref();
		case   4 : return nn_ifour.add_ref();
		case   5 : return nn_ifive.add_ref();
		case   6 : return nn_isix.add_ref();
		case   8 : return nn_ieight.add_ref();
		case  10 : return nn_iten.add_ref();
#if SIZEOF_WORD < 2
		case 1000000000u :
			return nn_maxull.add_ref();
#elif SIZEOF_WORD < 8
		case 10000000000000000000ull :
			return nn_maxull.add_ref();
#endif
		default  : ;
	}

	nn_integer p = nn_new(sizeof(v) / sizeof(word));

	memcpy(p->data_, &v, sizeof(v));
	p->data_[p->length_] = 0;
	p->normalize();

	return p;
}
//------------------------------------------------------------------------------
static inline nn_integer nn_init_ulong(long v)
{
	return nn_init_illong(v);
}
//------------------------------------------------------------------------------
static inline nn_integer nn_init_iulong(unsigned long v)
{
	return nn_init_iullong(v);
}
//------------------------------------------------------------------------------
#if __GNUG__ >= 5 && NDEBUG
#pragma GCC push_options
#pragma GCC optimize("O3")
#endif
//------------------------------------------------------------------------------
inline nn_integer nn_integer_data::isal(uintptr_t bit_count) const
{
	if( bit_count == 0 )
		return add_ref();

	uintptr_t new_bit_size = length_ * sizeof(word) * CHAR_BIT + bit_count;
	new_bit_size += -(intptr_t) new_bit_size & (sizeof(word) * CHAR_BIT - 1);

	nn_integer result = nn_new(new_bit_size / (sizeof(word) * CHAR_BIT));

	uintptr_t offset = bit_count / (sizeof(word) * CHAR_BIT);
	uintptr_t shift = bit_count & (sizeof(word) * CHAR_BIT - 1);

	union {
		word * w;
		dword * d;
	} dst, src;

	src.w = data_ + length_ - 1;
	dst.w = result->data_ + result->length_ - 2;

	memset(result->data_, 0, offset * sizeof(word));

	if( shift == 0 ){
		//dst.w++;
		memcpy(result->data_ + offset,data_,length_ * sizeof(word));
	}
	else {
		for( intptr_t i = length_ - 1; i >= 0; i--, dst.w--, src.w-- )
            *dst.d = (*src.d) << shift;
	}

	result->data_[result->length_ + 1] = result->data_[result->length_] = isign();
	result->normalize();

	//#if BYTE_ORDER == BIG_ENDIAN
	//#error Not implemented, ENOSYS
	//#elif BYTE_ORDER == LITTLE_ENDIAN
	//#error Not implemented, ENOSYS
	//#endif // BYTE_ORDER

	return result;
}
//------------------------------------------------------------------------------
#if __GNUG__ >= 5 && NDEBUG
#pragma GCC pop_options
#endif
//------------------------------------------------------------------------------
inline nn_integer nn_integer_data::isar(uintptr_t bit_count) const
{
	if( bit_count == 0 )
		return add_ref();

	uintptr_t bit_size = length_ * sizeof(word) * CHAR_BIT;

	if( bit_count >= bit_size )
		return nn_izero.add_ref();

	intptr_t new_bit_size = bit_size - bit_count;
	new_bit_size += -intptr_t(new_bit_size) & (sizeof(word) * CHAR_BIT - 1);

	nn_integer result = nn_new(new_bit_size / (sizeof(word) * CHAR_BIT));

	uintptr_t offset = bit_count / (sizeof(word) * CHAR_BIT);
	uintptr_t shift = bit_count & (sizeof(word) * CHAR_BIT - 1);

	union {
		word * w;
		dword * d;
	} dst, src;

	src.w = data_ + offset;
	dst.w = result->data_;

	//if( offset > 0 )
	//  src_word++;

	if( shift == 0 ){
		memcpy(result->data_,data_ + offset,result->length_ * sizeof(word));
	}
	else {
		for( intptr_t i = result->length_ - 1; i >= 0; i--, dst.w++, src.w++ )
			*dst.d = *src.d >> shift;
	}

	result->dummy_ = 0;
	result->data_[result->length_ + 1] = result->data_[result->length_] = isign();
	result->normalize();

	return result;
}
//------------------------------------------------------------------------------
} // namespace NaturalNumbers
//------------------------------------------------------------------------------
#endif // ID_HPP_INCLUDED
//------------------------------------------------------------------------------
