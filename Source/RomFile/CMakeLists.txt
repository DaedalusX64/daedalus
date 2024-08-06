

add_library(RomFile OBJECT
                    RomFile.cpp 
                    RomFileCache.cpp 
                    RomFileCompressed.cpp 
                    RomFileMemory.cpp 
                    RomFileUncompressed.cpp
                    ROMBuffer.cpp
                    RomSettings.cpp
            )

            
list(APPEND daed_libs "RomFile")
set(daed_libs ${daed_libs} PARENT_SCOPE)
