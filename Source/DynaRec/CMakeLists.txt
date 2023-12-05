if (${CMAKE_SYSTEM_PROCESSOR} MATCHES "armv")
    file(GLOB SRC_FILES ARM/*.cpp ARM/*.S)
elseif(${CMAKE_SYSTEM_PROCESSOR} STREQUAL "x86_64")
    file(GLOB SRC_FILES x64/*.cpp x64/*.S)
elseif(${CMAKE_SYSTEM_PROCESSOR} STREQUAL "AMD64")
    file(GLOB SRC_FILES x64/*.cpp x64/*.S)
elseif(${CMAKE_SYSTEM_PROCESSOR} STREQUAL "x86")
    file(GLOB SRC_FILES x86/*.cpp x86/*.S)
else()
    file(GLOB SRC_FILES x86/*.cpp x86/*.S)
endif()

add_library(DynaRec OBJECT
                ${SRC_FILES}
                BranchType.cpp
                DynaRecProfile.cpp
                Fragment.cpp
                FragmentCache.cpp 
                IndirectExitMap.cpp
                StaticAnalysis.cpp
                TraceRecorder.cpp
)

list(APPEND daed_libs "DynaRec")
set(daed_libs ${daed_libs} PARENT_SCOPE)
