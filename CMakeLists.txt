# CMakeLists.txt
cmake_minimum_required(VERSION 3.26 FATAL_ERROR)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

project(PopLib LANGUAGES C CXX ASM)

set(POPLIB_ROOT_DIR ${CMAKE_CURRENT_LIST_DIR})

set(BUILD_SHARED_LIBS OFF)

# temp workaround for me
set(_Python3_EXECUTABLE_DEBUG "" CACHE FILEPATH "Python debug executable")

set_property(GLOBAL PROPERTY USE_FOLDERS ON)

option(FEATURE_DISCORD_RPC "Discord RPC" OFF)
option(BUILD_EXAMPLES "Build Examples" ON)
option(CONSOLE "Show the console on Windows" ON)
option(BUILD_TOOLS "Build Tools" ON)

if (CMAKE_SIZEOF_VOID_P EQUAL 8)
    message(STATUS "Using x64")
	set(IS_64BIT true)
else ()
    message (STATUS "Using x86")
	set(IS_64BIT false)
endif()

if(FEATURE_DISCORD_RPC)
	add_compile_definitions(_FEATURE_DISCORD_RPC)
endif()

if(CONSOLE AND WIN32)
	add_compile_definitions(_CONSOLE_ON)
endif()

if (IS_64BIT)
    message (STATUS "Getting the x64 BASS path")
	set(BASS_LIB_PATH ${POPLIB_ROOT_DIR}/external/bass/x64)
else ()
    message (STATUS "Getting the x86 BASS path")
	set(BASS_LIB_PATH  ${POPLIB_ROOT_DIR}/external/bass/x86)
endif()

if (WIN32)
    set(BASS_PATH "${BASS_LIB_PATH}/bass.dll" CACHE FILEPATH "Path to BASS DLL" FORCE)
elseif (UNIX)
    set(BASS_PATH "${BASS_LIB_PATH}/libbass.so" CACHE FILEPATH "Path to BASS SO" FORCE)
endif()

# external libraries
set(BUILD_SHARED_LIBS OFF CACHE BOOL "Build as DLL" FORCE)
set(SDL_STATIC ON CACHE BOOL "" FORCE)
set(SDL_SHARED OFF CACHE BOOL "" FORCE)
set(LIBTYPE "STATIC" CACHE BOOL "" FORCE) # OpenAL
add_subdirectory(external/ogg EXCLUDE_FROM_ALL)
add_subdirectory(external/vorbis EXCLUDE_FROM_ALL)
add_subdirectory(external/SDL EXCLUDE_FROM_ALL)
add_subdirectory(external/SDL_ttf EXCLUDE_FROM_ALL)
add_subdirectory(external/openal EXCLUDE_FROM_ALL)
add_subdirectory(external/miniaudio EXCLUDE_FROM_ALL)
add_subdirectory(external/zlib EXCLUDE_FROM_ALL)
add_subdirectory(external/misc EXCLUDE_FROM_ALL)
add_subdirectory(external/discordrpc EXCLUDE_FROM_ALL)
add_subdirectory(external/tinyxml2 EXCLUDE_FROM_ALL)

# what the hell am i doing with my life
set(BUILD_CURL_EXE OFF CACHE BOOL "Build curl executable" FORCE)
# i am asking the same thing buddy.
set(CURL_ZLIB OFF CACHE BOOL "Disable curl's use of system zlib" FORCE)
set(HTTP_ONLY ON CACHE BOOL "" FORCE)
set(CURL_USE_LIBPSL OFF CACHE BOOL "Use libpsl" FORCE)
set(CURL_USE_OPENSSL OFF CACHE BOOL "" FORCE)
set(CURL_USE_MBEDTLS OFF CACHE BOOL "" FORCE)
add_subdirectory(external/curl EXCLUDE_FROM_ALL)

add_subdirectory(PopLib)

if(BUILD_EXAMPLES)
	add_subdirectory(examples)
endif()

# djugjsfgufdgujdfgiujgdijfgifjdgidfjgifdgjfdgufdguifdg electr0gunner told me to add this
if(BUILD_EXAMPLES OR BUILD_TOOLS)
    set(demo_deps PopLib)

    if(BUILD_EXAMPLES)
        list(APPEND demo_deps
            Demo1 Demo2 Demo3 Demo4 Demo5
            Hun-garr V12Demo V14Demo XMLDemo
			PhysicsDemo Barebones
        )
    endif()

    add_custom_target(alldemos ALL DEPENDS ${demo_deps})
endif()

if(NOT (DEFINED ENV{GITHUB_ACTIONS} AND "$ENV{GITHUB_ACTIONS}" STREQUAL "true"))
    add_custom_command(TARGET alldemos POST_BUILD
        COMMAND ${CMAKE_COMMAND} --install ${CMAKE_BINARY_DIR} --config $<CONFIG>
    )
endif()