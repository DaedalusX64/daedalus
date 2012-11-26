#ifndef BUILDOPTIONS_H_
#define BUILDOPTIONS_H_

//
//	Platform options
//
#undef  DAEDALUS_COMPRESSED_ROM_SUPPORT			// Define this to enable support for compressed Roms(zip'ed). If you define this, you will need to add unzip.c and ioapi.c to the project too. (Located at Source/Utility/Zip/)
#undef  DAEDALUS_ENABLE_DYNAREC					// Define this is dynarec is supported on the platform
#undef  DAEDALUS_ENABLE_OS_HOOKS				// Define this to enable OS HLE
#undef  DAEDALUS_TRAP_PLUGIN_EXCEPTIONS			// Define this if exceptions are available and you want to trap exceptions from the plugin
#undef  DAEDALUS_BREAKPOINTS_ENABLED			// Define this to enable breakpoint support
#undef	DAEDALUS_ENDIAN_MODE					// Define this to specify whether the platform is big or little endian

// DAEDALUS_ENDIAN_MODE should be defined as one of:
//
#define DAEDALUS_ENDIAN_LITTLE 1
#define DAEDALUS_ENDIAN_BIG 2

#define DAEDALUS_ENDIAN_MODE DAEDALUS_ENDIAN_LITTLE
//
//	Set up your preprocessor flags to search Source/SysXYZ/Include first, where XYZ is your target platform
//	If certain options are not defined, defaults are provided below
//
#include "Platform.h"

// Calling convention for the R4300 instruction handlers. 
// This is only defined for W32, so provide a default if it's not set up
#ifndef R4300_CALL_TYPE
#define R4300_CALL_TYPE
#endif

// Calling convention for threads
#ifndef DAEDALUS_THREAD_CALL_TYPE
#define DAEDALUS_THREAD_CALL_TYPE
#endif

// Calling convention for vararg functions
#ifndef DAEDALUS_VARARG_CALL_TYPE
#define DAEDALUS_VARARG_CALL_TYPE
#endif

#ifndef DAEDALUS_ZLIB_CALL_TYPE
#define DAEDALUS_ZLIB_CALL_TYPE
#endif

//	Branch prediction
#ifndef DAEDALUS_EXPECT_LIKELY
#define DAEDALUS_EXPECT_LIKELY(c) (c)
#endif 
#ifndef DAEDALUS_EXPECT_UNLIKELY
#define DAEDALUS_EXPECT_UNLIKELY(c) (c)
#endif

#ifndef DAEDALUS_ATTRIBUTE_NOINLINE
#define DAEDALUS_ATTRIBUTE_NOINLINE
#endif

#ifndef MAKE_UNCACHED_PTR
#define MAKE_UNCACHED_PTR(x)	(x)
#endif

// Pure is a function attribute which says that a function does not modify any global memory.
// Const is a function attribute which says that a function does not read/modify any global memory.

// Given that information, the compiler can do some additional optimisations.

#ifndef DAEDALUS_ATTRIBUTE_PURE
#define DAEDALUS_ATTRIBUTE_PURE
#endif

#ifndef DAEDALUS_ATTRIBUTE_CONST
#define DAEDALUS_ATTRIBUTE_CONST
#endif

//
//	Configuration options. These are not really platform-specific, but control various features
//
#include "BuildConfig.h"

#endif // BUILDOPTIONS_H_
