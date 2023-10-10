add_library(Utility_CTR OBJECT
                CacheCTR.S
                # IOCTR.cpp
                ${PROJECT_SOURCE_DIR}/Source/SysPosix/Utility/IOPosix.cpp
                MemoryCTR.c
                ThreadCTR.cpp
                TimingCTR.cpp
            )

            
list(APPEND daed_libs "Utility_CTR")
set(daed_libs ${daed_libs} PARENT_SCOPE)


