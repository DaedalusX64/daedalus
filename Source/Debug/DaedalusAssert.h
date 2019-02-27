/*

  Copyright (C) 2001 StrmnNrmn

  This program is free software; you can redistribute it and/or
  modify it under the terms of the GNU General Public License
  as published by the Free Software Foundation; either version 2
  of the License, or (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

*/

#pragma once

#ifndef DEBUG_DAEDALUSASSERT_H_
#define DEBUG_DAEDALUSASSERT_H_

#include "Utility/Macros.h"

// Ideas for the ignored assert taken from Game Programming Gems I

#if defined(__clang__) && __has_feature(cxx_static_assert)

#define DAEDALUS_STATIC_ASSERT( x ) static_assert((x), "Static Assert")

#else
//
// This static assert is bastardised from Boost:
// http://www.boost.org/boost/static_assert.hpp
//  (C) Copyright John Maddock 2000.
//  Use, modification and distribution are subject to the
//  Boost Software License, Version 1.0. (See accompanying file
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//  See http://www.boost.org/libs/static_assert for documentation.
//
template <bool> struct STATIC_ASSERTION_FAILURE;
template <> struct STATIC_ASSERTION_FAILURE<true>{};
template<int x> struct static_assert_test{};

// NB! This should be enabled across all builds regardless of whether DAEDALUS_ENABLE_ASSERTS is set!
// It's a compile-time assert and has no runtime cost!
#define DAEDALUS_STATIC_ASSERT( x )								\
   typedef static_assert_test<sizeof(STATIC_ASSERTION_FAILURE< (bool)( x ) >)>	static_assert_typedef_##__COUNTER__

#endif

#ifdef DAEDALUS_ENABLE_ASSERTS

enum EAssertResult
{
	AR_IGNORE,
	AR_IGNORE_ONCE,
	AR_BREAK,
};

EAssertResult DAEDALUS_VARARG_CALL_TYPE DaedalusAssert( const char * expression, const char * file, unsigned int line, const char * msg, ... );

#ifndef DAEDALUS_HALT
#error DAEDALUS_HALT should be defined in Platform.h
#endif

//
//	Use this api to override the default assert handler, e.g. for logging asserts during a batch process
//
typedef EAssertResult (*DaedalusAssertHook)( const char * expression, const char * file, unsigned int line, const char * formatted_msg, ... );

extern DaedalusAssertHook gAssertHook;

inline void SetAssertHook( DaedalusAssertHook hook )
{
	gAssertHook = hook;
}

//
// Use this to report on problems with the emulator - not the emulated title
//
#define DAEDALUS_ASSERT( e, ... )												\
{																				\
	static bool ignore = false;													\
	if ( !(e) && !ignore )														\
	{																			\
		EAssertResult ar;														\
		if (gAssertHook != NULL)												\
			ar = gAssertHook( #e,  __FILE__, __LINE__, __VA_ARGS__ );			\
		else																	\
			ar = DaedalusAssert( #e,  __FILE__, __LINE__, __VA_ARGS__ );		\
		if ( ar == AR_BREAK )													\
		{																		\
			DAEDALUS_HALT;	/* User breakpoint */								\
		}																		\
		else if ( ar == AR_IGNORE )												\
		{																		\
			ignore = true;	/* Ignore throughout session */						\
		}																		\
	}																			\
}

//
// Use this to assert without specifying a message
//
#define DAEDALUS_ASSERT_Q( e )													\
{																				\
	static bool ignore = false;													\
	if ( !(e) && !ignore )														\
	{																			\
		EAssertResult ar;														\
		if (gAssertHook != NULL)												\
			ar = gAssertHook( #e,  __FILE__, __LINE__, "" );					\
		else																	\
			ar = DaedalusAssert( #e,  __FILE__, __LINE__, "" );					\
		if ( ar == AR_BREAK )													\
		{																		\
			DAEDALUS_HALT;	/* User breakpoint */								\
		}																		\
		else if ( ar == AR_IGNORE )												\
		{																		\
			ignore = true;	/* Ignore throughout session */						\
		}																		\
	}																			\
}

//
// Use this to assert unconditionally - e.g. for unhandled cases
//
#define DAEDALUS_ERROR( ... )													\
{																				\
	static bool ignore = false;													\
	if ( !ignore )																\
	{																			\
		EAssertResult ar;														\
		if (gAssertHook != NULL)												\
			ar = gAssertHook( "",  __FILE__, __LINE__, __VA_ARGS__ );			\
		else																	\
			ar = DaedalusAssert( "",  __FILE__, __LINE__, __VA_ARGS__ );		\
		if ( ar == AR_BREAK )													\
		{																		\
			DAEDALUS_HALT;	/* User breakpoint */								\
		}																		\
		else if ( ar == AR_IGNORE )												\
		{																		\
			ignore = true;	/* Ignore throughout session */						\
		}																		\
	}																			\
}

#else // DAEDALUS_ENABLE_ASSERTS

#define DAEDALUS_ASSERT( ... )		do { DAEDALUS_USE(__VA_ARGS__); } while(0)
#define DAEDALUS_ASSERT_Q( ... )	do { DAEDALUS_USE(__VA_ARGS__); } while(0)
#define DAEDALUS_ERROR( ... )		do { DAEDALUS_USE(__VA_ARGS__); } while(0)

#endif // DAEDALUS_ENABLE_ASSERTS

#endif // DEBUG_DAEDALUSASSERT_H_
