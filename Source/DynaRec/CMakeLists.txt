add_library(DynaRec OBJECT 
                                BranchType.cpp 
                                DynaRecProfile.cpp
                                Fragment.cpp
                                FragmentCache.cpp 
                                IndirectExitMap.cpp 
                                StaticAnalysis.cpp 
                                TraceRecorder.cpp)

list(APPEND daed_libs "DynaRec")
set(daed_libs ${daed_libs} PARENT_SCOPE)
