add_library(Utility_CTR OBJECT
                CacheCTR.S
                MemoryCTR.c
                ThreadCTR.cpp
                # TimingCTR.cpp // We use the Timing based on the STL now. (Remove when tested okay)
            )

            
list(APPEND daed_libs "Utility_CTR")
set(daed_libs ${daed_libs} PARENT_SCOPE)


