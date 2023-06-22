#pragma once

#ifndef UTILITY_MACROS_H_
#define UTILITY_MACROS_H_

#ifdef _MSC_VER
#define DAEDALUS_FORCEINLINE __forceinline
#else
#define DAEDALUS_FORCEINLINE inline
#endif

#ifdef DAEDALUS_ENABLE_ASSERTS

#ifdef DAEDALUS_DEBUG_CONSOLE
#define NODEFAULT		DAEDALUS_ERROR( "No default - we shouldn't be here" )
#endif
#else

#ifdef _MSC_VER
#define NODEFAULT		__assume( 0 )
#else
#define NODEFAULT		//DAEDALUS_EXPECT_LIKELY(1)?
#endif

#endif

#ifndef ARRAYSIZE
#define ARRAYSIZE(arr)  std::size(arr)
#endif

#define DAEDALUS_USE(...)	do { (void)sizeof(__VA_ARGS__, 0); } while(0)

#endif // UTILITY_MACROS_H_
