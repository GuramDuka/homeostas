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
#include "config.h"
//------------------------------------------------------------------------------
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#if __GNUC__ >= 5 || _MSC_VER
#include <io.h>
#include <share.h>
#endif
#include <ctime>
#include <cerrno>
#include <cstring>
#if _WIN32
#include <process.h>
#include <windows.h>
#else
#include <dirent.h>
#endif
//------------------------------------------------------------------------------
#if defined(_S_IFDIR) && !defined(S_IFDIR)
#define S_IFDIR _S_IFDIR
#endif
#if defined(_S_IFREG) && !defined(S_IFREG)
#define S_IFREG _S_IFREG
#endif
#if !defined(S_IFLNK)
#define S_IFLNK 0
#endif
#if !defined(S_ILNK)
#define S_ILNK (fmt) false
#endif
//------------------------------------------------------------------------------
#include "port.hpp"
//------------------------------------------------------------------------------
#if _WIN32 && _MSC_VER
//------------------------------------------------------------------------------
int clock_gettime(int /*dummy*/, struct timespec * ct)
{
    static thread_local bool initialized = false;
    static thread_local uint64_t freq = 0;

    if( !initialized ) {
        LARGE_INTEGER f;

        if( QueryPerformanceFrequency(&f) != FALSE )
            freq = f.QuadPart;

        initialized = true;
    }

    LARGE_INTEGER t;

    if( freq == 0 ) {
        FILETIME f;
        GetSystemTimeAsFileTime(&f);

        t.QuadPart = f.dwHighDateTime;
        t.QuadPart <<= 32;
        t.QuadPart |= f.dwLowDateTime;

        ct->tv_sec = decltype(ct->tv_sec)((t.QuadPart / 10000000));
        ct->tv_nsec = decltype(ct->tv_nsec)((t.QuadPart % 10000000) * 100); // FILETIME is in units of 100 nsec.
    }
    else {
        QueryPerformanceCounter(&t);
        ct->tv_sec = decltype(ct->tv_sec)((t.QuadPart / freq));
        //uint64_t q = t.QuadPart % freq;
        //freq -> 1000000000ns
        //q    -> x?
        ct->tv_nsec = decltype(ct->tv_nsec)((t.QuadPart % freq) * 1000000000 / freq);
    }

    return 0;
}
//------------------------------------------------------------------------------
#endif
//------------------------------------------------------------------------------
namespace homeostas {
//------------------------------------------------------------------------------
const char path_delimiter[] =
#if _WIN32
    "\\"
#else
    "/"
#endif
;
//------------------------------------------------------------------------------
int mkdir(const std::string & path_name)
{
    int err;

    auto make = [&] {
        bool r;

        err = 0;
#if _WIN32
        r = CreateDirectoryW(QString::fromStdString(path_name).toStdWString().c_str(), NULL) == FALSE;
        if( r ) {
            err = GetLastError();
            if( err == ERROR_ALREADY_EXISTS ) {
                r = false;
                err = EEXIST;
            }
            if( err != ERROR_PATH_NOT_FOUND && err != ERROR_ALREADY_EXISTS )
#else
        r = ::mkdir(path_name.c_str(), S_IRUSR | S_IWUSR | S_IXUSR) != 0;
        if( r ) {
            err = errno;
            if( err == EEXIST )
                r = false;
            if( err != ENOTDIR && r != EEXIST )
#endif
                throw std::runtime_error("Error create directory, " + std::to_string(err));
        }

        return r;
    };

    if( make() ) {
        auto i = path_name.rfind(path_delimiter[0]);

        if( i == std::string::npos )
            throw std::runtime_error("Error create directory");

        mkdir(std::string(path_name, 0, i));

        if( make() )
            throw std::runtime_error("Error create directory");
    }

    return err;
}
//------------------------------------------------------------------------------
int access(const std::string & path_name, int mode)
{
#if _WIN32
    return _waccess(QString::fromStdString(path_name).toStdWString().c_str(), mode);
#else
    return access(path_name.c_str(), mode);
#endif
}
//------------------------------------------------------------------------------
std::string getenv(const std::string & var_name)
{
#if _WIN32
    return QString::fromWCharArray(
        _wgetenv(QString::fromStdString(var_name).toStdWString().c_str())
    ).toStdString();
#else
    return getenv(var_name.c_str());
#endif
}
//------------------------------------------------------------------------------
std::string home_path(bool no_back_slash)
{
    std::string s;
#if _WIN32
    s = QString::fromWCharArray(_wgetenv(L"HOMEDRIVE")).toStdString()
        + QString::fromWCharArray(_wgetenv(L"HOMEPATH")).toStdString();

    if( access(s, R_OK | W_OK | X_OK) != 0 )
        s = QString::fromWCharArray(_wgetenv(L"USERPROFILE")).toStdString();

    if( access(s, R_OK | W_OK | X_OK) != 0 )
        throw std::runtime_error("Access denied to user home directory");
#else
    s = ::getenv("HOME");

    if( access(s.c_str(), R_OK | W_OK | X_OK) != 0 )
        throw std::runtime_error("Access denied to user home directory");
#endif
    if( no_back_slash ) {
        if( s.back() == path_delimiter[0] )
            s.pop_back();
    }
    else if( s.back() != path_delimiter[0] )
        s.push_back(path_delimiter[0]);

    return s;
}
//------------------------------------------------------------------------------
std::string temp_path(bool no_back_slash)
{
#if _WIN32
    std::wstring ws;

    DWORD a = GetTempPathW(0, NULL);

    ws.resize(a - 1);

    GetTempPathW(a, &ws[0]);

    std::string s = QString::fromStdWString(ws).toStdString();
#else
    std::string s = P_tmpdir;
#endif
    if( no_back_slash ) {
        if( s.back() == path_delimiter[0] )
            s.pop_back();
    }
    else if( s.back() != path_delimiter[0] )
        s.push_back(path_delimiter[0]);

    return s;
}
//------------------------------------------------------------------------------
std::string temp_name(std::string dir, std::string pfx)
{
    constexpr int MAXTRIES = 10000;
	static std::atomic_int index;

	if( dir.empty() )
        dir = temp_path(true);
    if( pfx.empty() )
        pfx = "temp";

#if __GNUC__ >= 5 || _MSC_VER
    int pid = _getpid();
#else
    int pid = getpid();
#endif

    if( access(dir, R_OK | W_OK | X_OK) != 0 )
        throw std::runtime_error("access denied to directory: " + dir);

	int try_n = 0;
    char s[4 * (sizeof(int) * 3 + 2) + 1];

	do {
		struct timespec ts;
		clock_gettime(CLOCK_REALTIME, &ts);

        int m = int(ts.tv_sec ^ uintptr_t(&s[0]) ^ uintptr_t(&s));
        int n = int(ts.tv_nsec ^ uintptr_t(&s[0]) ^ uintptr_t(&s));

        snprintf(s, sizeof(s), "-%d-%d-%x-%x", pid, index.fetch_add(1), m, n);
    }
    while( !access(s, F_OK) && try_n++ < MAXTRIES );

	if( try_n >= MAXTRIES )
        throw std::range_error("function temp_name MAXTRIES reached");

    return dir + path_delimiter + pfx + s;
}
//------------------------------------------------------------------------------
std::string get_cwd(bool no_back_slash)
{
#if _WIN32
    DWORD a = GetCurrentDirectoryW(0, NULL);

    std::wstring ws;
    ws.resize(a - 1);

    GetCurrentDirectoryW(a, &ws[0]);

    std::string s = QString::fromStdWString(ws).toStdString();
#else
    std::string s;

    s.resize(32);

    for(;;) {
		if( getcwd(&s[0], s.size() + 1) != nullptr )
			break;

		if( errno != ERANGE ) {
			auto err = errno;
            throw std::runtime_error("Failed to get current work directory, " + std::to_string(err));
		}

		s.resize(s.size() << 1);
    }

    s.resize(slen(s.c_str()));

    if( s.empty() )
        s = ".";

    if( !no_back_slash )
        s += "/";

	s.shrink_to_fit();
#endif
    if( no_back_slash ) {
        if( s.back() == path_delimiter[0] )
            s.pop_back();
    }
    else if( s.back() != path_delimiter[0] )
        s.push_back(path_delimiter[0]);

    return s;
}
//------------------------------------------------------------------------------
std::string path2rel(const std::string & path, bool no_back_slash)
{
    std::string file_path = path;

    if( !file_path.empty() ) {
#if _WIN32
        file_path = std::str_replace<std::string>(file_path, "/", "\\");
#else
        file_path = std::str_replace<std::string>(file_path, "\\", "/");
#endif
        //file_path = path.find(path_delimiter[0]) == 0 ? path.substr(1) : path;

        if( no_back_slash ) {
            if( file_path.back() == path_delimiter[0] )
                file_path.pop_back();
        }
        else if( file_path.back() != path_delimiter[0] )
            file_path.push_back(path_delimiter[0]);
    }
	return file_path;
}
//------------------------------------------------------------------------------
} // namespace homeostas
//------------------------------------------------------------------------------
