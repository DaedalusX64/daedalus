add_library(Utility OBJECT
                            BatchTest.cpp
                            CRC.cpp 
                            FastMemcpy.cpp 
                            FramerateLimiter.cpp
                            Hash.cpp 
                            IniFile.cpp 
                            MemoryHeap.cpp  
                            Paths.cpp
                            Profiler.cpp  
                            Stream.cpp 
                            StringUtil.cpp
                            Timer.cpp
                            Translate.cpp

            )


            # Utility/FastMemcpy_test.cpp Utility/MemoryPool.cpp These are testing


            
list(APPEND daed_libs "Utility")
set(daed_libs ${daed_libs} PARENT_SCOPE)
