if(NOT DAEDALUS_CTR)
set(Timing Timing.cpp)
endif()

add_library(System OBJECT 

            CompressedStream.cpp 
            SystemInit.cpp
            ${Timing}    
            )

target_include_directories(System PUBLIC ${ZLIB_INCLUDE_DIRS})

if(DAEDALUS_PROFILE_SYSTEM)
        target_compile_options(System PRIVATE -pg)
endif(DAEDALUS_PROFILE_SYSTEM)
            

list(APPEND daed_libs "System")
set(daed_libs ${daed_libs} PARENT_SCOPE)
