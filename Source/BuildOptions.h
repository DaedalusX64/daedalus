#ifndef BUILDOPTIONS_H_
#define BUILDOPTIONS_H_

// Platform specifc #includes, externs, #defines etc
#ifdef DAEDALUS_W32
#include "DaedalusW32.h"
#endif


//  Basic Platform Options are included here and are required per project.
#include "Platform.h"

// Below are required for now as Cmake doesn't support Function preprocessing
// Expect Likely / Unlikely are defined per platform anyway so probably can be removed

//	Branch prediction 
#ifndef DAEDALUS_EXPECT_LIKELY
#define DAEDALUS_EXPECT_LIKELY(c) (c)
#endif
#ifndef DAEDALUS_EXPECT_UNLIKELY
#define DAEDALUS_EXPECT_UNLIKELY(c) (c)
#endif

//PSP Uncached pointer. 
#ifndef MAKE_UNCACHED_PTR
#define MAKE_UNCACHED_PTR(x)	(x)
#endif

#endif // BUILDOPTIONS_H_
