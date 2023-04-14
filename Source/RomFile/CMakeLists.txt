

add_library(RomFile 
                    RomFile.cpp 
                    RomFileCache.cpp 
                    RomFileCompressed.cpp 
                    RomFileMemory.cpp 
                    RomFileUncompressed.cpp
            )

            
list(APPEND daed_libs "RomFile")
set(daed_libs ${daed_libs} PARENT_SCOPE)
