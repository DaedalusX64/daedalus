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

// We need this directive for backwards compatibility with devices that do not have C++ 20 available which enables use of std::format or fmt::format (with external library)
#if __has_include(<format>)
    #include <format>
    #define FORMAT_NAMESPACE std
#elif __has_include(<fmt/core.h>)
    #include <fmt/core.h>
    #define FORMAT_NAMESPACE fmt
#else
    #error "No supported format library found!"
#endif

#endif // UTILITY_MACROS_H_
