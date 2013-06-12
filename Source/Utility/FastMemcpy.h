
#ifndef UTILITY_FASTMEMCPY_H_
#define UTILITY_FASTMEMCPY_H_

#include <stdlib.h>

// Define to profile memcpys (PSP only!)
//#define PROFILE_MEMCPY

#ifdef PROFILE_MEMCPY
void memcpy_test( void * dst, const void * src, size_t size );
#endif

void memcpy_byteswap( void* dst, const void* src, size_t size );	// Little endian, platform independent, ALWAYS swaps.

// memcpy_swizzle is just a regular memcpy on big-endian targets.
#if (DAEDALUS_ENDIAN_MODE == DAEDALUS_ENDIAN_BIG)
#define memcpy_swizzle 		memcpy
#elif (DAEDALUS_ENDIAN_MODE == DAEDALUS_ENDIAN_LITTLE)
#define memcpy_swizzle 		memcpy_byteswap
#else
#error No DAEDALUS_ENDIAN_MODE specified
#endif


// The PSP build defines fast versions of memcpy/memcpy_swizzle.
#ifdef DAEDALUS_PSP

void memcpy_vfpu( void* dst, const void* src, size_t size );
void memcpy_vfpu_byteswap( void* dst, const void* src, size_t size );

#define fast_memcpy 		memcpy_vfpu
#define fast_memcpy_swizzle memcpy_vfpu_byteswap

#else // DAEDALUS_PSP

// Just use regular memcpy/memcpy_swizzle.
#define fast_memcpy 		memcpy
#define fast_memcpy_swizzle memcpy_swizzle

#endif // DAEDALUS_PSP


#endif // UTILITY_FASTMEMCPY_H_
