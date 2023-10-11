add_library(DynaRec OBJECT 
                                BranchType.cpp 
                                DynaRecProfile.cpp
                                Fragment.cpp
                                FragmentCache.cpp 
                                IndirectExitMap.cpp 
                                StaticAnalysis.cpp 
                                TraceRecorder.cpp)

if(${CMAKE_SYSTEM_PROCESSOR} MATCHES "arm")
add_subdirectory(ARM)
endif()

list(APPEND daed_libs "DynaRec")
set(daed_libs ${daed_libs} PARENT_SCOPE)
