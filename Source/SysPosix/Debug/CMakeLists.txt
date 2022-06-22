add_library(Debug_Plat OBJECT 
                            DaedalusAssertPosix.cpp 
                            DebugConsolePosix.cpp 
                            WebDebug.cpp 
                            WebDebugTemplate.cpp 
                            ${PROJECT_SOURCE_DIR}/Source/third_party/webby/webby.c)
