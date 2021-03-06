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
#ifndef PORT_HPP_INCLUDED
#define PORT_HPP_INCLUDED
//------------------------------------------------------------------------------
#pragma once
//------------------------------------------------------------------------------
#include "config.h"
//------------------------------------------------------------------------------
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#if __linux__ && !__ANDROID__
#   include <sys/io.h>
#endif
#if (__GNUG__ >= 4 && __MINGW32__) || _MSC_VER
#   include <io.h>
#   include <share.h>
#endif
#if _WIN32
#   include <process.h>
#else
#   include <dirent.h>
#endif
#include <cerrno>
#include <ctime>
//------------------------------------------------------------------------------
#include "std_ext.hpp"
//------------------------------------------------------------------------------
#if _WIN32
//------------------------------------------------------------------------------
#   ifndef CLOCK_REALTIME
#       define CLOCK_REALTIME 0
#   endif
//------------------------------------------------------------------------------
#endif
//------------------------------------------------------------------------------
namespace homeostas {
//------------------------------------------------------------------------------
extern const char path_delimiter[];
//------------------------------------------------------------------------------
std::string getenv(const std::string & var_name);
//------------------------------------------------------------------------------
#if _WIN32
#   ifndef F_OK
#       define F_OK 0
#   endif
#   ifndef X_OK
#       define X_OK 0
#   endif
#   ifndef W_OK
#       define W_OK 2
#   endif
#   ifndef R_OK
#       define R_OK 4
#   endif
#endif
int access(const std::string & path_name, int mode);
//------------------------------------------------------------------------------
int mkdir(const std::string & path_name);
std::string home_path(bool no_back_slash = false);
std::string temp_path(bool no_back_slash = false);
std::string temp_name(std::string dir = std::string(), std::string pfx = std::string());
std::string get_cwd(bool no_back_slash = false);
std::string path2rel(const std::string & path, bool no_back_slash = false);
int clock_gettime(int dummy, struct timespec & ct);
uint64_t clock_gettime_ns();
uint64_t entropy_fast();
//------------------------------------------------------------------------------
} // namespace homeostas
//------------------------------------------------------------------------------
// global namespace
//------------------------------------------------------------------------------
#if _WIN32 || (__GNUG__ > 0 && __GNUG__ < 5 && __ANDROID__)
extern "C" int getdomainname(char * name, size_t len);
#endif
//------------------------------------------------------------------------------
#endif // PORT_HPP_INCLUDED
//------------------------------------------------------------------------------
