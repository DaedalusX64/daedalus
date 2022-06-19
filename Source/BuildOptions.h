#ifndef BUILDOPTIONS_H_
#define BUILDOPTIONS_H_

#include "Base/Types.h"
// All Custom Macro Functions live here until CMake supports them.

// Defines same for all 
// XXX TODO. Check for safer options to reduce reliance on these Macros

#define DAEDALUS_EXPECT_LIKELY(c) __builtin_expect((c),1)
#define DAEDALUS_EXPECT_UNLIKELY(c) __builtin_expect((c),0)
#define DAEDALUS_ATTRIBUTE_CONST   __attribute__((const))
#define DAEDALUS_ATTRIBUTE_PURE   __attribute__((pure))



#ifdef DAEDALUS_PSP
    #define DAEDALUS_HALT			__asm__ __volatile__ ( "break" )
#elif DAEDALUS_POSIX
    #define DAEDALUS_HALT			__builtin_trap()
    //#define DAEDALUS_HALT			__builtin_debugger()
#elif DAEDALUS_CTR
    #define DAEDALUS_HALT			__asm__ __volatile__ ( "bkpt" )

#elif DAEDALUS_W32 // Ugh this needs simplifying
    #include "Sys32/Include/DaedalusW32.h" // Windows is special
    #define __PRETTY_FUNCTION__ __FUNCTION__
    #define _CRT_SECURE_NO_DEPRECATE
    #define _DO_NOT_DECLARE_INTERLOCKED_INTRINSICS_IN_MEMORY

    #define R4300_CALL_TYPE						__fastcall
    #define DAEDALUS_THREAD_CALL_TYPE			__stdcall // Thread functions need to be __stdcall to work with the W32 api
    #define DAEDALUS_VARARG_CALL_TYPE			__cdecl // Vararg functions need to be __cdecl
    #define	DAEDALUS_ZLIB_CALL_TYPE				__cdecl // Zlib is compiled as __cdecl
    #define DAEDALUS_HALT					__asm { int 3 }

#endif


// Handle Uncached Pointer as is everywhere in code
#ifdef DAEDALUS_PSP
    template <typename T>
    T make_uncached_ptr(T ptr) {
        return reinterpret_cast<T>(reinterpret_cast<u32>(ptr) | 0x40000000);
    }
#else
#define MAKE_UNCACHED_PTR(x)	(x)
#endif


#endif // BUILDOPTIONS_H_
