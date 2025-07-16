
if(DAEDALUS_PSP)
set(Timing Timing/PSP/Timing.cpp)
set(Thread Thread/PSP/Thread.cpp)
elseif(DAEDALUS_CTR)
set(Timing Timing/CTR/Timing.cpp)
set(Thread Thread/CTR/Thread.cpp)
else()
set(Timing Timing/Posix/Timing.cpp)
set(Thread Thread/Posix/Thread.cpp)
endif()

add_library(System OBJECT 

            CompressedStream.cpp 
            DataSink.cpp
            ${Thread}
            SystemInit.cpp
            ${Timing}    
            )

target_include_directories(System PUBLIC ${ZLIB_INCLUDE_DIRS})

if(DAEDALUS_PROFILE_SYSTEM)
        target_compile_options(System PRIVATE -pg)
endif(DAEDALUS_PROFILE_SYSTEM)
            

list(APPEND daed_libs "System")
set(daed_libs ${daed_libs} PARENT_SCOPE)
