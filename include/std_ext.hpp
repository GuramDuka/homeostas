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
#if __GNUC__ < 5
#include <sstream>
#endif
//------------------------------------------------------------------------------
namespace std {
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
template <const std::size_t buffer_size = 4096, class size_type = std::size_t> inline
void stream_copy_n(std::ostream & out, std::istream & in, const size_type & size) {
	char buffer[buffer_size];
	size_type count = size;

	//in.exceptions(std::istream::failbit);
	//out.exceptions(std::ostream::failbit);

	while( count > 0 ) {
		size_type r = count > buffer_size ? size_type(buffer_size) : count;

		in.read(buffer, r);
		//if( in.bad() || in.gcount() != r )
		//	throw std::runtime_error("stream read error");

		out.write(buffer, r);
		//if( out.bad() )
		//	throw std::runtime_error("stream write error");

		count -= r;
	}
}
//------------------------------------------------------------------------------
template <const std::size_t buffer_size = 4096> inline
void copy_eof(std::ostream & out, std::istream & in) {
	char buffer[buffer_size];

	for(;;) {
		in.read(buffer, buffer_size);
		std::size_t r = in.gcount();
		out.write(buffer, r);

		if( in.eof() )
			break;
	}
}
//------------------------------------------------------------------------------
#if __GNUC__ < 5
//------------------------------------------------------------------------------
template <typename T> inline std::string to_string(const T & v) {
    std::stringstream s;
    s << v;
    return s.str();
}
//------------------------------------------------------------------------------
#endif
//------------------------------------------------------------------------------
} // namespace std
//------------------------------------------------------------------------------
#endif // STD_EXT_HPP_INCLUDED
//------------------------------------------------------------------------------
