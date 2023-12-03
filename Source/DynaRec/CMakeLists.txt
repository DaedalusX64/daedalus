add_library(DynaRec OBJECT 
                                BranchType.cpp 
                                DynaRecProfile.cpp
                                Fragment.cpp
                                FragmentCache.cpp 
                                IndirectExitMap.cpp 
                                StaticAnalysis.cpp 
                                TraceRecorder.cpp)

if (${CMAKE_SYSTEM_PROCESSOR} MATCHES "armv")
    add_subdirectory(ARM)
elseif(${CMAKE_SYSTEM_PROCESSOR} STREQUAL "x86_64")
    add_subdirectory(x64)
elseif(${CMAKE_SYSTEM_PROCESSOR} STREQUAL "x86")
    add_subdirectory(x86)
elseif()
    add_subdirectory(dummy)
endif()

list(APPEND daed_libs "DynaRec")
set(daed_libs ${daed_libs} PARENT_SCOPE)
