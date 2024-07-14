#ifndef UTILITY_MACROS_H_
#define UTILITY_MACROS_H_

#include <cstdint>
#include <bit>

#define DAEDALUS_USE(...)	do { (void)sizeof(__VA_ARGS__, 0); } while(0)


#ifdef DAEDALUS_PSP
    template <typename T>
   constexpr T make_uncached_ptr(T ptr) 
   {
        std::uintptr_t ptrInt = std::bit_cast<std::uintptr_t>(ptr);
        ptrInt |= 0x40000000;
        return std::bit_cast<T>(ptrInt);
    }
#else
    template <typename T>
    constexpr T make_uncached_ptr(T ptr) { return ptr; }
#endif

#endif // UTILITY_MACROS_H_
