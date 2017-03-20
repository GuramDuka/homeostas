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
#ifndef SCOPE_EXIT_HPP_INCLUDED
#define SCOPE_EXIT_HPP_INCLUDED
//------------------------------------------------------------------------------
#pragma once
//------------------------------------------------------------------------------
namespace std {
//------------------------------------------------------------------------------
////////////////////////////////////////////////////////////////////////////////
//------------------------------------------------------------------------------
template <typename Lambda>
class AtScopeExit {
private:
	const Lambda & lambda_;
public:
	~AtScopeExit() { lambda_(); }
	AtScopeExit(const Lambda & lambda) : lambda_(lambda) {}
};
//------------------------------------------------------------------------------
////////////////////////////////////////////////////////////////////////////////
//------------------------------------------------------------------------------
template <typename Lambda>
class ScopeExit {
private:
	Lambda lambda_;
	void operator = (const ScopeExit<Lambda> & obj);
public:
	~ScopeExit() noexcept {
		lambda_();
	}
	ScopeExit(const Lambda lambda) noexcept : lambda_(lambda) {}
	ScopeExit(const ScopeExit<Lambda> & obj) noexcept : lambda_(obj.lambda_) {}
	ScopeExit(ScopeExit<Lambda> && obj) noexcept : lambda_(obj.lambda_) {}
};
//------------------------------------------------------------------------------
template <typename Lambda> inline
auto scope_exit(Lambda lambda) {
	return ScopeExit<Lambda>(lambda);
}
//------------------------------------------------------------------------------
} // namespace std
//------------------------------------------------------------------------------
#define AtScopeExit_INTERNAL2(lname, aname, ...) \
    const auto lname = [&]() { __VA_ARGS__; }; \
    std::AtScopeExit<decltype(lname)> aname(lname);

#define AtScopeExit_TOKENPASTE(x, y) AtScopeExit_ ## x ## y

#define AtScopeExit_INTERNAL1(ctr, ...) \
    AtScopeExit_INTERNAL2(AtScopeExit_TOKENPASTE(func_, ctr), \
                   AtScopeExit_TOKENPASTE(instance_, ctr), __VA_ARGS__)

#define at_scope_exit(...) AtScopeExit_INTERNAL1(__COUNTER__, __VA_ARGS__)
//------------------------------------------------------------------------------
#endif // SCOPE_EXIT_HPP_INCLUDED
//------------------------------------------------------------------------------
