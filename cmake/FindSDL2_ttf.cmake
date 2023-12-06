# sdl2_ttf cmake project-config input for ./configure scripts

include(FeatureSummary)
set_package_properties(SDL2_ttf PROPERTIES
    URL "https://www.libsdl.org/projects/SDL_ttf/"
    DESCRIPTION "Support for TrueType (.ttf) font files with Simple Directmedia Layer"
)

set(SDL2_ttf_FOUND TRUE)

set(SDL2TTF_HARFBUZZ 1)
set(SDL2TTF_FREETYPE TRUE)

set(SDL2TTF_VENDORED 0)

set(SDL2TTF_SDL2_REQUIRED_VERSION 2.0.10)

get_filename_component(prefix "${CMAKE_CURRENT_LIST_DIR}/../../../.." ABSOLUTE)
set(exec_prefix "${prefix}")
set(bindir "${exec_prefix}/bin")
set(includedir "${prefix}/include")
set(libdir "${prefix}/lib/x86_64-linux-gnu")
set(_sdl2ttf_extra_static_libraries " -lfreetype -lharfbuzz ")
string(STRIP "${_sdl2ttf_extra_static_libraries}" _sdl2ttf_extra_static_libraries)

set(_sdl2ttf_bindir   "${bindir}")
set(_sdl2ttf_libdir   "${libdir}")
set(_sdl2ttf_incdir   "${includedir}/SDL2")

# Convert _sdl2ttf_extra_static_libraries to list and keep only libraries
string(REGEX MATCHALL "(-[lm]([-a-zA-Z0-9._]+))|(-Wl,[^ ]*framework[^ ]*)" _sdl2ttf_extra_static_libraries "${_sdl2ttf_extra_static_libraries}")
string(REGEX REPLACE "^-l" "" _sdl2ttf_extra_static_libraries "${_sdl2ttf_extra_static_libraries}")
string(REGEX REPLACE ";-l" ";" _sdl2ttf_extra_static_libraries "${_sdl2ttf_extra_static_libraries}")

unset(prefix)
unset(exec_prefix)
unset(bindir)
unset(includedir)
unset(libdir)

include(CMakeFindDependencyMacro)

if(NOT TARGET SDL2_ttf::SDL2_ttf)
    if(WIN32)
        set(_sdl2ttf_dll "${_sdl2ttf_bindir}/SDL2_ttf.dll")
        set(_sdl2ttf_imp "${_sdl2ttf_libdir}/${CMAKE_STATIC_LIBRARY_PREFIX}SDL2_ttf.dll${CMAKE_STATIC_LIBRARY_SUFFIX}")
        if(EXISTS "${_sdl2ttf_dll}" AND EXISTS "${_sdl2ttf_imp}")
            add_library(SDL2_ttf::SDL2_ttf SHARED IMPORTED)
            set_target_properties(SDL2_ttf::SDL2_ttf
                PROPERTIES
                    IMPORTED_LOCATION "${_sdl2ttf_dll}"
                    IMPORTED_IMPLIB "${_sdl2ttf_imp}"
            )
        endif()
        unset(_sdl2ttf_dll)
        unset(_sdl2ttf_imp)
    else()
        set(_sdl2ttf_shl "${_sdl2ttf_libdir}/${CMAKE_SHARED_LIBRARY_PREFIX}SDL2_ttf${CMAKE_SHARED_LIBRARY_SUFFIX}")
        if(EXISTS "${_sdl2ttf_shl}")
            add_library(SDL2_ttf::SDL2_ttf SHARED IMPORTED)
            set_target_properties(SDL2_ttf::SDL2_ttf
                PROPERTIES
                    IMPORTED_LOCATION "${_sdl2ttf_shl}"
            )
        endif()
    endif()
    if(TARGET SDL2_ttf::SDL2_ttf)
        set_target_properties(SDL2_ttf::SDL2_ttf
            PROPERTIES
                INTERFACE_INCLUDE_DIRECTORIES "${_sdl2ttf_incdir}"
                COMPATIBLE_INTERFACE_BOOL "SDL2_SHARED"
                INTERFACE_SDL2_SHARED "ON"
        )
    endif()
endif()

if(NOT TARGET SDL2_ttf::SDL2_ttf-static)
    set(_sdl2ttf_stl "${_sdl2ttf_libdir}/${CMAKE_STATIC_LIBRARY_PREFIX}SDL2_ttf${CMAKE_STATIC_LIBRARY_SUFFIX}")
    if(EXISTS "${_sdl2ttf_stl}")
        add_library(SDL2_ttf::SDL2_ttf-static STATIC IMPORTED)
        set_target_properties(SDL2_ttf::SDL2_ttf-static
            PROPERTIES
                INTERFACE_INCLUDE_DIRECTORIES "${_sdl2ttf_incdir}"
                IMPORTED_LOCATION "${_sdl2ttf_stl}"
                INTERFACE_LINK_LIBRARIES "${_sdl2ttf_extra_static_libraries}"
        )
    endif()
    unset(_sdl2ttf_stl)
endif()

unset(_sdl2ttf_extra_static_libraries)
unset(_sdl2ttf_bindir)
unset(_sdl2ttf_libdir)
unset(_sdl2ttf_incdir)
