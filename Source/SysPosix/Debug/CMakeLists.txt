add_library(Posix_Debug OBJECT 
                            DaedalusAssertPosix.cpp 
                            DebugConsolePosix.cpp 
                            WebDebug.cpp 
                            WebDebugTemplate.cpp 
                            ${PROJECT_SOURCE_DIR}/third_party/webby/webby.c)
