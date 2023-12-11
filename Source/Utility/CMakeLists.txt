add_library(Utility OBJECT
                            CRC.cpp 
                            FastMemcpy.cpp 
                            Hash.cpp 
                            IniFile.cpp 
                            MemoryHeap.cpp  
                            Profiler.cpp  
                            Stream.cpp 
                            StringUtil.cpp
                            Translate.cpp
                            Timer.cpp
            )


            # Utility/FastMemcpy_test.cpp Utility/MemoryPool.cpp These are testing


            
list(APPEND daed_libs "Utility")
set(daed_libs ${daed_libs} PARENT_SCOPE)
