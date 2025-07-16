
if(DAEDALUS_PSP)
set(Timing Timing/PSP/Timing.cpp)
elseif(DAEDALUS_CTR)
set(Timing Timing/CTR/Timing.cpp)
else()
set(Timing Timing/Posix/Timing.cpp)
endif()

add_library(System OBJECT 

            CompressedStream.cpp 
            DataSink.cpp
            Thread.cpp
            SystemInit.cpp
            ${Timing}    
            )

target_include_directories(System PUBLIC ${ZLIB_INCLUDE_DIRS})

if(DAEDALUS_PROFILE_SYSTEM)
        target_compile_options(System PRIVATE -pg)
endif(DAEDALUS_PROFILE_SYSTEM)
            

list(APPEND daed_libs "System")
set(daed_libs ${daed_libs} PARENT_SCOPE)
