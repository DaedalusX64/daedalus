#ifndef BUILDOPTIONS_H_
#define BUILDOPTIONS_H_

#include "Base/Types.h"
#include <cstdint>
// All Custom Macro Functions live here until CMake supports them.

// Defines same for all 
// XXX TODO. Check for safer options to reduce reliance on these Macros

#define DAEDALUS_EXPECT_LIKELY(c) __builtin_expect((c),1)
#define DAEDALUS_EXPECT_UNLIKELY(c) __builtin_expect((c),0)
#define DAEDALUS_ATTRIBUTE_CONST   __attribute__((const))
#define DAEDALUS_ATTRIBUTE_PURE   __attribute__((pure))





// Handle Uncached Pointer as is everywhere in code
#ifdef DAEDALUS_PSP
    template <typename T>
    T make_uncached_ptr(T ptr) {
        return reinterpret_cast<T>(reinterpret_cast<unsigned int>(ptr) | 0x40000000);
    }
#else
#define make_uncached_ptr(x)	(x)
#endif


#endif // BUILDOPTIONS_H_
