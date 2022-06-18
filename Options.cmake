## Options File Ugh, I wish tehre were a better way for this

if(DAEDALUS_BATCH_TEST_ENABLED)
    add_compile_definitions(DAEDALUS_BATCH_TEST_ENABLED)
endif(DAEDALUS_BATCH_TEST_ENABLED)

if(DAEDALUS_DEBUG_CONSOLE)
    add_compile_definitions(DAEDALUS_DEBUG_CONSOLE)
endif()

if(DAEDALUS_DEBUG_DISPLAYLIST)
    add_compile_definitions(DAEDALUS_DEBUG_DISPLAYLIST)
endif()

if(DAEDALUS_DEBUG_MEMORY)
    add_compile_definitions(DAEDALUS_DEBUG_MEMORY)
endif()

if(DAEDALUS_DEBUG_PIF)
    add_compile_definitions(DAEDALUS_DEBUG_PIF)
endif()

if(DAEDALUS_DEBUG_DYNAREC)
    add_compile_definitions(DAEDALUS_DEBUG_DYNAREC)
endif()

if(DAEDALUS_ENABLE_ASSERTS)
    add_compile_definitions(DAEDALUS_ENABLE_ASSERTS)
endif()

if(DAEDALUS_ENABLE_SYNCHRONISATION)
    add_compile_definitions(DAEDALUS_ENABLE_SYNCHRONISATION)
endif()

if(DAEDALUS_ENABLE_PROFILING)
    add_compile_definitions(DAEDALUS_ENABLE_PROFILING)
endif()

if(DAEDALUS_ENABLE_DYNAREC)
    add_compile_definitions(DAEDALUS_ENABLE_DYNAREC)
endif()

if(DAEDALUS_ENABLE_OS_HOOKS)
    add_compile_definitions(DAEDALUS_ENABLE_OS_HOOKS)
endif()

if(DAEDALUS_LOG)
    add_compile_definitions(DAEDALUS_LOG)
endif()

if(DAEDALUS_PROFILE_EXECUTION)
    add_compile_definitions(DAEDALUS_PROFILE_EXECUTION)
endif(DAEDALUS_PROFILE_EXECUTION)

if(DAEDALUS_SILENT)
    add_compile_definitions(DAEDALUS_SILENT)
endif(DAEDALUS_SILENT)

if(DAEDALUS_PSP)
    add_compile_definitions(DAEDALUS_PSP)
endif(DAEDALUS_PSP)

if(DAEDALUS_PSP_USE_ME)
    add_compile_definitions(DAEDALUS_PSP_USE_ME)
endif(DAEDALUS_PSP_USE_ME)

if(DAEDALUS_PSP_USE_VFPU)
    add_compile_definitions(DAEDALUS_PSP_USE_VFPU)
endif()

if(DAEDALUS_DIALOGS)
    add_compile_definitions(DAEDALUS_DIALOGS)
endif(DAEDALUS_DIALOGS)


if(DAEDALUS_POSIX)
    add_compile_definitions(DAEDALUS_POSIX)
endif()

if(DAEDALUS_COMPRESSED_ROM_SUPPORT)
    add_compile_definitions(DAEDALUS_COMPRESSED_ROM_SUPPORT)
endif()

if(DAEDALUS_ACCURATE_TMEM)
    add_compile_definitions(DAEDALUS_ACCURATE_TMEM)
endif()
if(DAEDALUS_PSP_GPROF)
    add_compile_definitions(DAEDALUS_PSP_GPROF)
endif()
if(DAEDALUS_SIM_DOUBLES)
    add_compile_definitions(DAEDALUS_SIM_DOUBLES)
endif()
if(DAEDALUS_128BIT_MULT)
    add_compile_definitions(DAEDALUS_128BIT_MULT)
endif()
if(DAEDALUS_ACCURATE_CVT)
    add_compile_definitions(DAEDALUS_ACCURATE_CVT)
endif()
if(DAEDALUS_SDL)
    add_compile_definitions(DAEDALUS_SDL)
endif()