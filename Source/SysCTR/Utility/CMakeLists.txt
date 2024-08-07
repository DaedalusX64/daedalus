add_library(Utility_CTR OBJECT
                CacheCTR.S
                MemoryCTR.c
                ThreadCTR.cpp
                TimingCTR.cpp 
            )

            
list(APPEND daed_libs "Utility_CTR")
set(daed_libs ${daed_libs} PARENT_SCOPE)


