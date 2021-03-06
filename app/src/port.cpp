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
#include <atomic>
#include <ctime>
#include <cstring>
#if __GNUG__ > 0 && __GNUG__ < 5 && __ANDROID__
#   include <unistd.h>
#   include <sys/utsname.h>
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
// global namespace
//------------------------------------------------------------------------------
#if _WIN32 || (__GNUG__ > 0 && __GNUG__ < 5 && __ANDROID__)
//------------------------------------------------------------------------------
extern "C" int getdomainname(char * name, size_t len)
{
#if _WIN32
    DWORD dwSize = DWORD(len);

    if( GetComputerNameExA(ComputerNamePhysicalDnsDomain, name, &dwSize) == FALSE ) {
        if( GetLastError() == ERROR_MORE_DATA ) {
            errno = EINVAL;
            return -1;
        }
        else {
            errno = EFAULT;
            return -1; //_com_issue_error(HRESULT_FROM_WIN32(GetLastError()));
        }
        //buf = (char *) alloca(dwSize + 1);
        //if( GetComputerNameExA((COMPUTER_NAME_FORMAT)ComputerNameDnsDomain, buf, &dwSize) == 0 ) {
            //hr =   HRESULT_FROM_WIN32(GetLastError());
            //errno = EFAULT;
            //return -1;
        //}
    }
    else if( GetLastError() != NO_ERROR ){
        errno = EFAULT;
        return -1; //_com_issue_error(HRESULT_FROM_WIN32(GetLastError()));
    }
#else
    utsname uts;

    if( uname(&uts) == -1 )
        return -1;

    // Note: getdomainname()'s behavior varies across implementations when len is
    // too small.  bionic follows the historical libc policy of returning EINVAL,
    // instead of glibc's policy of copying the first len bytes without a NULL
    // terminator.
    if( std::slen(uts.domainname) >= len ) {
        errno = EINVAL;
        return -1;
    }

    strncpy(name, uts.domainname, len);
#endif

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
    int err = 0;

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
            if( err != ENOTDIR && err != EEXIST )
#endif
                throw std::xruntime_error("Error create directory, " + std::to_string(err), __FILE__, __LINE__);
        }

        return r;
    };

    if( make() ) {
        auto i = path_name.rfind(path_delimiter[0]);

        if( i == std::string::npos )
            throw std::xruntime_error("Error create directory", __FILE__, __LINE__);

        mkdir(std::string(path_name, 0, i));

        if( make() )
            throw std::xruntime_error("Error create directory", __FILE__, __LINE__);
    }

    return err;
}
//------------------------------------------------------------------------------
int access(const std::string & path_name, int mode)
{
#if _WIN32
    return ::_waccess(QString::fromStdString(path_name).toStdWString().c_str(), mode);
#else
    return ::access(path_name.c_str(), mode);
#endif
}
//------------------------------------------------------------------------------
std::string getenv(const std::string & var_name)
{
#if _WIN32
    return QString::fromWCharArray(
        ::_wgetenv(QString::fromStdString(var_name).toStdWString().c_str())
    ).toStdString();
#else
    return ::getenv(var_name.c_str());
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
        throw std::xruntime_error("Access denied to user home directory", __FILE__, __LINE__);
#else
    s = ::getenv("HOME");

    if( access(s, R_OK | W_OK | X_OK) != 0 )
        throw std::xruntime_error("Access denied to user home directory", __FILE__, __LINE__);
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

#if _MSC_VER
    int pid = _getpid();
#else
    int pid = getpid();
#endif

    if( access(dir, R_OK | W_OK | X_OK) != 0 )
        throw std::xruntime_error("access denied to directory: " + dir, __FILE__, __LINE__);

	int try_n = 0;
    char s[4 * (sizeof(int) * 3 + 2) + 1];

	do {
		struct timespec ts;
        clock_gettime(CLOCK_REALTIME, ts);

        int m = int(ts.tv_sec ^ uintptr_t(&s[0]) ^ uintptr_t(&s));
        int n = int(ts.tv_nsec ^ uintptr_t(&s[0]) ^ uintptr_t(&s));

        snprintf(s, sizeof(s), "-%d-%d-%x-%x", pid, index.fetch_add(1), m, n);
    }
    while( !access(s, F_OK) && try_n++ < MAXTRIES );

	if( try_n >= MAXTRIES )
        throw std::xruntime_error("function temp_name MAXTRIES reached", __FILE__, __LINE__);

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
            throw std::xruntime_error("Failed to get current work directory, " + std::to_string(err), __FILE__, __LINE__);
		}

		s.resize(s.size() << 1);
    }

    s.resize(std::slen(s.c_str()));

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
int clock_gettime(int dummy, struct timespec & ct)
{
#if _WIN32
    if( dummy == CLOCK_REALTIME ) {
        constexpr const uint64_t secs_epoch  = 11644473600u; // Seconds between 1.1.1601 and 1.1.1970
        constexpr const uint64_t nsecs_epoch = secs_epoch * 10000000u; // units of 100 nsec
        static thread_local uint64_t freq = 0, start = 0, stp = 0;
        FILETIME f;
        ULARGE_INTEGER q;
        LARGE_INTEGER t;

        if( QueryPerformanceFrequency(&t) != FALSE && t.QuadPart != 0 && freq != uint64_t(t.QuadPart) ) {
            freq = t.QuadPart;
            GetSystemTimeAsFileTime(&f);
            QueryPerformanceCounter(&t);
            q.LowPart = f.dwLowDateTime;
            q.HighPart = f.dwHighDateTime;
            q.QuadPart = (q.QuadPart - nsecs_epoch) * 100u; // nanoseconds
            // to avoid overflow calculate separately seconds and nanoseconds
            start = (q.QuadPart / 1000000000u) * freq + (q.QuadPart % 1000000000u) * freq / 1000000000u; // freq ticks
            stp = t.QuadPart;
        }
        else
            freq = 0;

        if( freq == 0 ) {
            GetSystemTimeAsFileTime(&f);
            q.LowPart = f.dwLowDateTime;
            q.HighPart = f.dwHighDateTime;

            ct.tv_sec  = decltype(ct.tv_sec) (q.QuadPart / 10000000u) - secs_epoch;
            ct.tv_nsec = decltype(ct.tv_nsec) ((q.QuadPart % 10000000u) * 100u); // FILETIME is in units of 100 nsec.
        }
        else {
            QueryPerformanceCounter(&t);
            t.QuadPart -= stp;
            t.QuadPart += start;
            ct.tv_sec = decltype(ct.tv_sec) (t.QuadPart / freq);
            //uint64_t q = t.QuadPart % freq;
            //freq -> 1000000000ns
            //q    -> x?
            ct.tv_nsec = decltype(ct.tv_nsec) ((t.QuadPart % freq) * 1000000000u / freq);
        }

        return 0;
    }
#   if __MINGW32__
    return ::clock_gettime(dummy, &ct);
#   else
    errno = ENOSYS;
    return -1;
#   endif
#else
    return ::clock_gettime(dummy, &ct);
#endif
}
//------------------------------------------------------------------------------
uint64_t clock_gettime_ns()
{
    struct timespec ts;

    if( clock_gettime(CLOCK_REALTIME, ts) != 0 )
        throw std::xruntime_error("clock_gettime failed", __FILE__, __LINE__);

    return 1000000000ull * ts.tv_sec + ts.tv_nsec;
}
//------------------------------------------------------------------------------
uint64_t entropy_fast()
{
    auto f = &entropy_fast;
    union {
        struct {
            uint64_t ts;
            void * p0;
            uintptr_t p1;
            void * p2;
            uintptr_t p3;
            uint8_t ff[sizeof(f)];
        };
        uint64_t g[16]; // some garbage on stack
    } a;

    a.ts = clock_gettime_ns();
    memcpy(&a.ff, &f, sizeof(f));
    a.p0 = &a;
    a.p1 = uintptr_t(&a) ^ uintptr_t(a.ts);
    a.p2 = (void *) std::rhash(a.p1);
    a.p3 = uintptr_t(a.ts) ^ uintptr_t(a.p0);

    uint64_t h = clock_gettime_ns() ^ std::rhash(a.ts >> 5) ^ (a.ts << 3);

    for( auto g : a.g )
        h = std::ihash(g, h);

    return h;
}
//------------------------------------------------------------------------------
} // namespace homeostas
//------------------------------------------------------------------------------
