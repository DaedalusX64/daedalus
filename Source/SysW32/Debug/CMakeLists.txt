add_library(Debug_Win DaedalusAssertW32.cpp DebugConsoleW32.cpp)

list(APPEND daed_libs Debug_Win)
set(daed_libs ${daed_libs} PARENT_SCOPE)

