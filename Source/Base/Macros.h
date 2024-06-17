#ifndef UTILITY_MACROS_H_
#define UTILITY_MACROS_H_

#include <cstdint>


#define DAEDALUS_USE(...)	do { (void)sizeof(__VA_ARGS__, 0); } while(0)


#ifdef DAEDALUS_PSP
    template <typename T>
    T make_uncached_ptr(T ptr) {
        return reinterpret_cast<T>(reinterpret_cast<uint32_t>(ptr) | 0x40000000);
    }
#else
#define make_uncached_ptr(x)	(x)
#endif

#endif // UTILITY_MACROS_H_
