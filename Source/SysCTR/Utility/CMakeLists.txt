add_library(Utility_CTR OBJECT
                CacheCTR.S
                MemoryCTR.c
            )

if(DAEDALUS_PROFILE_UTILITY_CTR)
     target_compile_options(Utility_CTR PRIVATE -pg)
endif(DAEDALUS_PROFILE_UTILITY_CTR)
                            
            
list(APPEND daed_libs "Utility_CTR")
set(daed_libs ${daed_libs} PARENT_SCOPE)


