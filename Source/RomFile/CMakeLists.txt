

add_library(RomFile OBJECT
                    RomFile.cpp 
                    RomFileCache.cpp 
                    RomFileCompressed.cpp 
                    RomFileMemory.cpp 
                    RomFileUncompressed.cpp
                    ROMBuffer.cpp
                    RomSettings.cpp
            )

if(DAEDALUS_PROFILE_ROMFILE)
        target_compile_options(RomFile PRIVATE -pg)
endif(DAEDALUS_PROFILE_ROMFILE)
            


list(APPEND daed_libs "RomFile")
set(daed_libs ${daed_libs} PARENT_SCOPE)
