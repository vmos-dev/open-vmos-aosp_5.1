/**
 * Copyright(c) 2011 Trusted Logic.   All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 *  * Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *  * Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in
 *    the documentation and/or other materials provided with the
 *    distribution.
 *  * Neither the name Trusted Logic nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef __S_VERSION_H__
#define __S_VERSION_H__

/*
 * Usage: define S_VERSION_BUILD on the compiler's command line.
 *
 * Then set:
 * - S_VERSION_OS
 * - S_VERSION_PLATFORM
 * - S_VERSION_MAIN
 * - S_VERSION_ENG is optional
 * - S_VERSION_PATCH is optional
 * - S_VERSION_BUILD = 0 if S_VERSION_BUILD not defined or empty
 */
#if defined(WIN32)
#define S_VERSION_OS "W"          /* "W" for Windows PC (XP, Vista…) */
#define S_VERSION_PLATFORM "X"    /* "X" for ix86 PC simulators */
#elif defined(__ANDROID32__)
#define S_VERSION_OS "A"          /* "A" for Android */
#define S_VERSION_PLATFORM "G"    /* "G" for 4430 */
#elif defined(LINUX)
#define S_VERSION_OS "L"          /* "L" for Linux */
#define S_VERSION_PLATFORM "X"    /* "X" for ix86 PC simulators */
#else
#define S_VERSION_OS "X"          /* "X" for Secure-World */
#define S_VERSION_PLATFORM "G"    /* "G" for 4430 */
#endif

/*
 * This version number must be updated for each new release
 */
#define S_VERSION_MAIN  "01.04"
#define S_VERSION_RESOURCE 1,4,0,S_VERSION_BUILD

/*
* If this is a patch or engineering version use the following
* defines to set the version number. Else set these values to 0.
*/
#define S_VERSION_PATCH 11
#define S_VERSION_ENG 0

#ifdef S_VERSION_BUILD
/* TRICK: detect if S_VERSION is defined but empty */
#if 0 == S_VERSION_BUILD-0
#undef  S_VERSION_BUILD
#define S_VERSION_BUILD 0
#endif
#else
/* S_VERSION_BUILD is not defined */
#define S_VERSION_BUILD 0
#endif

#define __STRINGIFY(X) #X
#define __STRINGIFY2(X) __STRINGIFY(X)

#if S_VERSION_ENG != 0
#define _S_VERSION_ENG "e" __STRINGIFY2(S_VERSION_ENG)
#else
#define _S_VERSION_ENG ""
#endif

#if S_VERSION_PATCH != 0
#define _S_VERSION_PATCH "p" __STRINGIFY2(S_VERSION_PATCH)
#else
#define _S_VERSION_PATCH ""
#endif

#if !defined(NDEBUG) || defined(_DEBUG)
#define S_VERSION_VARIANT "D   "
#else
#define S_VERSION_VARIANT "    "
#endif

#define S_VERSION_STRING \
	"SMC" \
	S_VERSION_OS \
	S_VERSION_PLATFORM \
	S_VERSION_MAIN \
	_S_VERSION_PATCH \
	_S_VERSION_ENG \
	"."  __STRINGIFY2(S_VERSION_BUILD) " " \
	S_VERSION_VARIANT

#endif /* __S_VERSION_H__ */
