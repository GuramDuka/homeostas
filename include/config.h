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
#ifndef CONFIG_H_INCLUDED
#define CONFIG_H_INCLUDED
//------------------------------------------------------------------------------
#pragma once
//------------------------------------------------------------------------------
#if _MSC_VER
#   pragma execution_character_set("utf-8")
#endif
//------------------------------------------------------------------------------
//#if _WIN32
//// Implies UTF-16 encoding.
//#define CPPX_WITH_SYSCHAR_PREFIX( lit ) L##lit
//#else
//// Implies UTF-8 encoding.
//#   if __GNUC__ >= 5 || _MSC_VER
//#       define CPPX_WITH_SYSCHAR_PREFIX( lit ) u8##lit
//#   else
//#       define CPPX_WITH_SYSCHAR_PREFIX( lit ) lit
//#   endif
//#endif
//#define CPPX_U CPPX_WITH_SYSCHAR_PREFIX
//------------------------------------------------------------------------------
#if _WIN32
#   if _MSC_VER
#       define NOMINMAX
#   endif
#   include <winsock2.h>
#   include <ws2tcpip.h>
#   include <windows.h>
#else
#   include <unistd.h>
#endif
//------------------------------------------------------------------------------
#if !defined(HAVE_READDIR_R)
#   if defined(__GNUC_PREREQ) && defined(__GLIBC_PREREQ)
#       if __GNUC_PREREQ (3, 2) && !__GLIBC_PREREQ(2, 19 + 1)
#           if _POSIX_C_SOURCE || _BSD_SOURCE || _SVID_SOURCE
#               define HAVE_READDIR_R 1
#           endif
#       endif
#   endif
#endif
//------------------------------------------------------------------------------
#if !defined(HAVE_CODECVT)
#   if __GNUC__ >= 5 || _MSC_VER >= 1900
#       define HAVE_CODECVT 1
#   endif
#endif
//------------------------------------------------------------------------------
#if __GNUC__
#   ifndef __STDC_FORMAT_MACROS
#       define __STDC_FORMAT_MACROS 1
#   endif
#   ifndef __STDC_CONSTANT_MACROS
#       define __STDC_CONSTANT_MACROS 1
#   endif
#elif _MSC_VER
#   define __inline__ __forceinline
#   define ____
#else
#   define __inline__ __inline
#   define ____
#endif
//------------------------------------------------------------------------------
#if __linux__ || __BSD__
#   define HAVE_ENDIAN_H 1
#endif
//------------------------------------------------------------------------------
#if __cplusplus
extern "C" {
#endif
//-----------------------------------------------------------------------------
#if HAVE_ENDIAN_H
#   include <endian.h>
#   ifdef htobe16
#       define HAVE_HTOBE16 1
#   endif
#   ifdef htobe32
#       define HAVE_HTOBE32 1
#   endif
#   ifdef htobe64
#       define HAVE_HTOBE64 1
#   endif
#   ifdef le16toh
#       define HAVE_LE16TOH 1
#   endif
#   ifdef le32toh
#       define HAVE_LE32TOH 1
#   endif
#   ifdef le64toh
#       define HAVE_LE64TOH 1
#   endif
#   ifdef be16toh
#       define HAVE_BE16TOH 1
#   endif
#   ifdef be32toh
#       define HAVE_BE32TOH 1
#   endif
#   ifdef be64toh
#       define HAVE_BE64TOH 1
#   endif
#   ifdef htole16
#       define HAVE_HTOLE16 1
#   endif
#   ifdef htole32
#       define HAVE_HTOLE32 1
#   endif
#   ifdef htole64
#       define HAVE_HTOLE64 1
#   endif
//---------------------------------------------------------------------------
#elif _WIN32
#   if defined(__i386__) || defined(__x86_64__) || defined(_M_IX86) || defined(_M_X64)
#       define LITTLE_ENDIAN 1234
#       define BIG_ENDIAN 4321
#   endif
#   define BYTE_ORDER LITTLE_ENDIAN
#   include <inttypes.h>
#   include <stdint.h>
//-----------------------------------------------------------------------------
static __inline__ uint16_t htobe16(uint16_t u)
{
#define HAVE_HTOBE16 1
#if BYTE_ORDER == LITTLE_ENDIAN
  return (uint16_t) ((u >> 8) | ((u & 0xff) << 8));
#else
  return u;
#endif
}
//---------------------------------------------------------------------------
static __inline__ uint32_t htobe32(uint32_t u)
{
#define HAVE_HTOBE32 1
#if BYTE_ORDER == LITTLE_ENDIAN
    u = (u << 16) ^ (u >> 16);
    return ((u & 0x00ff00ff) << 8) ^ ((u >> 8) & 0x00ff00ff);
#else
  return u;
#endif
}
//---------------------------------------------------------------------------
static __inline__ uint64_t htobe64(uint64_t u)
{
#define HAVE_HTOBE64 1
#if BYTE_ORDER == LITTLE_ENDIAN
    u = (u << 32) | (u >> 32);
    u = ((u & 0x0000ffff0000ffffull) << 16) | ((u >> 16) & 0x0000ffff0000ffffull);
    u = ((u & 0x00ff00ff00ff00ffull) <<  8) | ((u >>  8) & 0x00ff00ff00ff00ffull);
#endif
  return u;
}
//---------------------------------------------------------------------------
static __inline__ uint16_t le16toh(uint16_t u)
{
#define HAVE_LE16TOH 1
#if BYTE_ORDER == LITTLE_ENDIAN
  return u;
#else
  return (uint16_t) ((u >> 8) | ((u & 0xff) << 8));
#endif
}
//---------------------------------------------------------------------------
static __inline__ uint32_t le32toh(uint32_t u)
{
#define HAVE_LE32TOH 1
#if BYTE_ORDER == LITTLE_ENDIAN
    return u;
#else
    u = (u << 16) ^ (u >> 16);
    return ((u & 0x00ff00ff) << 8) ^ ((u >> 8) & 0x00ff00ff);
#endif
}
//---------------------------------------------------------------------------
static __inline__ uint64_t le64toh(uint64_t u)
{
#define HAVE_LE64TOH 1
#if BYTE_ORDER == BIG_ENDIAN
    u = (u << 32) | (u >> 32);
    u = ((u & 0x0000ffff0000ffffull) << 16) | ((u >> 16) & 0x0000ffff0000ffffull);
    u = ((u & 0x00ff00ff00ff00ffull) <<  8) | ((u >>  8) & 0x00ff00ff00ff00ffull);
#endif
  return u;
}
//---------------------------------------------------------------------------
static __inline__ uint16_t be16toh(uint16_t u)
{
#define HAVE_BE16TOH 1
#if BYTE_ORDER == LITTLE_ENDIAN
  return (uint16_t) ((u >> 8) | ((u & 0xff) << 8));
#else
  return u;
#endif
}
//---------------------------------------------------------------------------
static __inline__ uint32_t be32toh(uint32_t u)
{
#define HAVE_BE32TOH 1
#if BYTE_ORDER == LITTLE_ENDIAN
    u = (u << 16) ^ (u >> 16);
    return ((u & 0x00ff00ff) << 8) ^ ((u >> 8) & 0x00ff00ff);
#else
    return u;
#endif
}
//---------------------------------------------------------------------------
static __inline__ uint64_t be64toh(uint64_t u)
{
#define HAVE_BE64TOH 1
#if BYTE_ORDER == LITTLE_ENDIAN
    u = (u << 32) | (u >> 32);
    u = ((u & 0x0000ffff0000ffffull) << 16) | ((u >> 16) & 0x0000ffff0000ffffull);
    u = ((u & 0x00ff00ff00ff00ffull) <<  8) | ((u >>  8) & 0x00ff00ff00ff00ffull);
#endif
    return u;
}
//---------------------------------------------------------------------------
static __inline__ uint16_t htole16(uint16_t u)
{
#define HAVE_HTOLE16 1
#if BYTE_ORDER == LITTLE_ENDIAN
  return u;
#else
  return (uint16_t) ((u >> 8) | ((u & 0xff) << 8));
#endif
}
//---------------------------------------------------------------------------
static __inline__ uint32_t htole32(uint32_t u)
{
#define HAVE_HTOLE32 1
#if BYTE_ORDER == LITTLE_ENDIAN
    return u;
#else
    u = (u << 16) ^ (u >> 16);
    return ((u & 0x00ff00ff) << 8) ^ ((u >> 8) & 0x00ff00ff);
#endif
}
//---------------------------------------------------------------------------
static __inline__ uint64_t htole64(uint64_t u)
{
#define HAVE_HTOLE64 1
#if BYTE_ORDER == BIG_ENDIAN
    u = (u << 32) | (u >> 32);
    u = ((u & 0x0000ffff0000ffffull) << 16) | ((u >> 16) & 0x0000ffff0000ffffull);
    u = ((u & 0x00ff00ff00ff00ffull) <<  8) | ((u >>  8) & 0x00ff00ff00ff00ffull);
#endif
    return u;
}
//---------------------------------------------------------------------------
#endif
//------------------------------------------------------------------------------
#ifndef __BSD__
//---------------------------------------------------------------------------
static __inline__ void be16enc(void * pp, uint16_t u)
{
#if HAVE_HTOBE16
  *(uint16_t *) pp = htobe16(u);
#elif BYTE_ORDER == LITTLE_ENDIAN
  uint8_t * p = (uint8_t *) pp;

  p[0] = (uint8_t) ((u >> 8) & 0xff);
  p[1] = (uint8_t) (u & 0xff);
#else
  *(uint16_t *) pp = u;
#endif
}
//---------------------------------------------------------------------------
static __inline__ void be32enc(void * pp, uint32_t u)
{
#if HAVE_HTOBE32
  *(uint32_t *) pp = htobe32(u);
#elif BYTE_ORDER == LITTLE_ENDIAN
    u = (u << 16) ^ (u >> 16);
    *(uint32_t *) pp = ((u & 0x00ff00ff) << 8) ^ ((u >> 8) & 0x00ff00ff);
#else
  *(uint32_t *) pp = u;
#endif
}
//---------------------------------------------------------------------------
static __inline__ void be64enc(void * pp, uint64_t u)
{
#if HAVE_HTOBE64
  *(uint64_t *) pp = htobe64(u);
#elif BYTE_ORDER == LITTLE_ENDIAN
  u = (u << 32) | (u >> 32);
  u = ((u & 0x0000ffff0000ffffull) << 16) | ((u >> 16) & 0x0000ffff0000ffffull);
  *(uint64_t *) pp = ((u & 0x00ff00ff00ff00ffull) <<  8) | ((u >>  8) & 0x00ff00ff00ff00ffull);
#else
  *(uint64_t *) pp = u;
#endif
}
//---------------------------------------------------------------------------
static __inline__ void le16enc(void *pp,uint16_t u)
{
#if HAVE_HTOLE16
  *(uint16_t *) pp = htole16(u);
#elif BYTE_ORDER == LITTLE_ENDIAN
  *(uint16_t *) pp = u;
#else
  uint8_t * p = (uint8_t *) pp;

  p[0] = (uint8_t) ((u >> 8) & 0xff);
  p[1] = (uint8_t) (u & 0xff);
#endif
}
//---------------------------------------------------------------------------
static __inline__ void le32enc(void *pp, uint32_t u)
{
#if HAVE_HTOLE32
    *(uint32_t *) pp = htole32(u);
#elif BYTE_ORDER == LITTLE_ENDIAN
    *(uint32_t *) pp = u;
#else
    u = (u << 16) ^ (u >> 16);
    *(uint32_t *) pp = ((u & 0x00ff00ff) << 8) ^ ((u >> 8) & 0x00ff00ff);
#endif
}
//---------------------------------------------------------------------------
static __inline__ void le64enc(void *pp, uint64_t u)
{
#if HAVE_HTOLE64
  *(uint64_t *) pp = htole64(u);
#elif BYTE_ORDER == LITTLE_ENDIAN
  *(uint64_t *) pp = u;
#else
  u = (u << 32) | (u >> 32);
  u = ((u & 0x0000ffff0000ffffull) << 16) | ((u >> 16) & 0x0000ffff0000ffffull);
  *(uint64_t *) pp = ((u & 0x00ff00ff00ff00ffull) <<  8) | ((u >>  8) & 0x00ff00ff00ff00ffull);
#endif
}
//---------------------------------------------------------------------------
static __inline__ uint16_t be16dec(const void * pp)
{
#if HAVE_BE16TOH
  return be16toh(*(uint16_t *) pp);
#elif BYTE_ORDER == LITTLE_ENDIAN
  uint8_t const * p = (uint8_t const *) pp;
  return (uint16_t) (((p[0] << 8) | p[1]));
#else
  return *(uint16_t *) pp;
#endif
}
//---------------------------------------------------------------------------
static __inline__ uint32_t be32dec(const void * pp)
{
#if HAVE_BE32TOH
  return be32toh(*(uint32_t *) pp);
#elif BYTE_ORDER == LITTLE_ENDIAN
    uint32_t u = *(uint32_t *) pp;
    u = (u << 16) ^ (u >> 16);
    return ((u & 0x00ff00ff) << 8) ^ ((u >> 8) & 0x00ff00ff);
#else
  return *(uint32_t *) pp;
#endif
}
//---------------------------------------------------------------------------
static __inline__ uint64_t be64dec(const void *pp)
{
#if HAVE_BE64TOH
    return be64toh(*(uint64_t *) pp);
#elif BYTE_ORDER == LITTLE_ENDIAN
    uint64_t u = *(uint64_t *) pp;
    u = (u << 32) | (u >> 32);
    u = ((u & 0x0000ffff0000ffffull) << 16) | ((u >> 16) & 0x0000ffff0000ffffull);
    return ((u & 0x00ff00ff00ff00ffull) <<  8) | ((u >>  8) & 0x00ff00ff00ff00ffull);
#else
    return *(uint64_t *) pp;
#endif
}
//---------------------------------------------------------------------------
static __inline__ uint16_t le16dec(const void * pp)
{
#if HAVE_LE16TOH
  return le16toh(*(uint16_t *) pp);
#elif BYTE_ORDER == LITTLE_ENDIAN
  return *(uint16_t *) pp;
#else
  return (uint16_t) ((u >> 8) | ((u & 0xff) << 8));
#endif
}
//---------------------------------------------------------------------------
static __inline__ uint32_t le32dec(const void *pp)
{
#if HAVE_LE32TOH
  return le32toh(*(uint32_t *) pp);
#elif BYTE_ORDER == LITTLE_ENDIAN
  return *(uint32_t *) pp;
#else
    uint32_t u = *(uint32_t *) pp;
    u = (u << 16) ^ (u >> 16);
    return ((u & 0x00ff00ff) << 8) ^ ((u >> 8) & 0x00ff00ff);
#endif
}
//---------------------------------------------------------------------------
static __inline__ uint64_t le64dec(const void *pp)
{
#if HAVE_LE64TOH
  return le64toh(*(uint64_t *) pp);
#elif BYTE_ORDER == LITTLE_ENDIAN
  return *(uint64_t *) pp;
#else
    uint64_t u = *(uint64_t *) pp;
    u = (u << 32) | (u >> 32);
    u = ((u & 0x0000ffff0000ffffull) << 16) | ((u >> 16) & 0x0000ffff0000ffffull);
    return ((u & 0x00ff00ff00ff00ffull) <<  8) | ((u >>  8) & 0x00ff00ff00ff00ffull);
#endif
}
//---------------------------------------------------------------------------
#endif
//---------------------------------------------------------------------------
#if __cplusplus
} // extern "C"
#endif
//-----------------------------------------------------------------------------
#if __cplusplus
//------------------------------------------------------------------------------
// On Android for armeabi-v7a GCC 4.9:  __cplusplus == 201300L
//------------------------------------------------------------------------------
#if (defined(_MSC_VER) && _MSC_VER < 1900) || (!defined(_MSC_VER) && __cplusplus < 201300L)
#   error This code needs at least a C++14 compliant compiler
#endif
//------------------------------------------------------------------------------
#if __GNUG__
#   include <stddef.h>
#endif
#include <ctime>
#include <climits>
#include <cinttypes>
#include <cstdint>
//------------------------------------------------------------------------------
#if !defined(PACKED)
#   if __GNUC__
#       define PACKED __attribute__ ((packed))
#   else
#       define PACKED
#   endif
#endif
//------------------------------------------------------------------------------
namespace std {
struct leave_uninitialized_type {};
constexpr const leave_uninitialized_type leave_uninitialized = {};
struct zero_initialized_type {};
constexpr const zero_initialized_type zero_initialized = {};
}
//------------------------------------------------------------------------------
namespace homeostas { namespace tests { void run_tests(); }}
//------------------------------------------------------------------------------
#endif
//------------------------------------------------------------------------------
#endif // CONFIG_H_INCLUDED
//------------------------------------------------------------------------------
