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
#include <iostream>
#include <cstring>
//------------------------------------------------------------------------------
#include "cdc512.hpp"
//------------------------------------------------------------------------------
namespace homeostas {
//------------------------------------------------------------------------------
namespace tests {
//------------------------------------------------------------------------------
void cdc512_test()
{
	bool fail = false;

	try {
#ifndef NDEBUG
        //std::cerr << cdc512::generate_prime() << std::endl;
#endif
		uint8_t t[240] = { 0 };

		for( size_t i = 1; i < sizeof(t); i++ )
			t[i] = t[i - 1] + 1;

		cdc512 ctx1;
		ctx1.init();
		ctx1.update(t, sizeof(t));
		ctx1.final();

        //std::qerr << (s = ctx1.to_string()) << std::endl;

		t[3] ^= 0x40;

		cdc512 ctx2;
		ctx2.init();
		ctx2.update(t, sizeof(t));
		ctx2.final();

        //std::qerr << (s = ctx2.to_string()) << std::endl;
		
        ctx1.generate_entropy();
        const auto s1 = std::to_string(ctx1);
        ctx2 = std::stokey512(s1);
        const auto s2 = std::to_string(ctx2);

        if( s1 != s2 )
            throw std::xruntime_error("bad cdc512 implementation", __FILE__, __LINE__);

	}
    catch (const std::exception & e) {
        std::cerr << e << std::endl;
        fail = true;
    }
    catch (...) {
		fail = true;
	}

    std::cerr << "cdc512 test " << (fail ? "failed" : "passed") << std::endl;
}
//------------------------------------------------------------------------------
} // namespace tests
//------------------------------------------------------------------------------
} // namespace homeostas
//------------------------------------------------------------------------------
