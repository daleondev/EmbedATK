# --- Project Setup ---
cmake_minimum_required(VERSION 3.20)
project(EmbedATK
    VERSION 0.1.0
    DESCRIPTION "The Embedded Automation Toolkit"
    LANGUAGES ASM C CXX
)

# --- External Libraries ---
add_subdirectory(
    ${CMAKE_CURRENT_SOURCE_DIR}/external/magic_enum
)

# --- Library Target Definition ---
add_library(EmbedATK STATIC)
add_library(EmbedATK::EmbedATK ALIAS EmbedATK)

# --- Source Files ---
file(GLOB_RECURSE SOURCES
    CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp"
)
file(GLOB_RECURSE HEADERS
    CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/include/*.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/include/*.hpp"
)
target_sources(EmbedATK
    PRIVATE
        ${SOURCES}
        ${HEADERS}
)

# --- Include Directories ---
target_include_directories(EmbedATK
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/external/reflect>
    PRIVATE
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/src>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/platform>
)

# --- Precompiled Headers ---
target_precompile_headers(EmbedATK
    PRIVATE
        "src/pch.h"
)

# --- Link Libraries ---
target_link_libraries(EmbedATK
    PUBLIC
        magic_enum::magic_enum
)

# --- Compile Features ---
target_compile_features(EmbedATK
    PUBLIC
        cxx_std_23
)

# --- Compile Options ---
target_compile_options(EmbedATK
    PRIVATE
        $<$<CXX_COMPILER_ID:MSVC>:/W4>
        $<$<NOT:$<CXX_COMPILER_ID:MSVC>>:-Wall -Wextra>
        $<$<CXX_COMPILER_ID:MSVC>:/FI"${CMAKE_CURRENT_SOURCE_DIR}/src/pch.h">
)

# --- Compile Definitions ---
target_compile_definitions(EmbedATK 
    PUBLIC 
        EATK_ENABLE_ASSERTS
)

# --- Platform Specific Definitions ---
set(X86_ARCH_REGEX "^(x86_64|AMD64)$")
set(ARM_ARCH_REGEX "^(arm|aarch64|armv7l|arm64)$")

if(WIN32)
    target_compile_definitions(EmbedATK PUBLIC EATK_PLATFORM_WINDOWS)
elseif(APPLE)
    if(CMAKE_SYSTEM_PROCESSOR MATCHES "${X86_ARCH_REGEX}")
        target_compile_definitions(EmbedATK PUBLIC EATK_PLATFORM_MACOS)
        target_include_directories(EmbedATK PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/platform/macos/include/OSAL)
    else()
        message(FATAL_ERROR "Unsupported Apple processor: ${CMAKE_SYSTEM_PROCESSOR}")
    endif()
elseif(CMAKE_SYSTEM_NAME STREQUAL "Linux")
    if(CMAKE_SYSTEM_PROCESSOR MATCHES "${X86_ARCH_REGEX}")
        target_compile_definitions(EmbedATK PUBLIC EATK_PLATFORM_LINUX)
        target_include_directories(EmbedATK PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/platform/linux/include/OSAL)
    elseif(CMAKE_SYSTEM_PROCESSOR MATCHES "${ARM_ARCH_REGEX}")
        target_compile_definitions(EmbedATK PUBLIC EATK_PLATFORM_LINUX_ARM)
        target_include_directories(EmbedATK PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/platform/linux_arm/include/OSAL)
    else()
        message(FATAL_ERROR "Unsupported Linux processor: ${CMAKE_SYSTEM_PROCESSOR}")
    endif()
elseif(CMAKE_SYSTEM_NAME STREQUAL "Generic")
    message(STATUS "Configuring for a generic bare-metal target.")
    if(CMAKE_SYSTEM_PROCESSOR MATCHES "${ARM_ARCH_REGEX}")
        if(NOT TARGET_PLATFORM)
            message(FATAL_ERROR "TARGET_PLATFORM must be specified on the command line.")
        endif()
        
        target_compile_definitions(EmbedATK PUBLIC EATK_PLATFORM_ARM)
        include("${CMAKE_CURRENT_SOURCE_DIR}/platform/arm/${TARGET_PLATFORM}.cmake")
        target_include_directories(EmbedATK PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/platform/arm/include/OSAL)
        target_link_libraries(EmbedATK PUBLIC Platform)

        set(BUILD_SAMPLES OFF)
        set(BUILD_TESTS OFF)
    else()
        message(FATAL_ERROR "Unsupported processor for generic target.")
    endif()
else()
    message(FATAL_ERROR "Unsupported Operating System: ${CMAKE_SYSTEM_NAME}")
endif()

# --- Source Properties/Groups for IDE ---
foreach(h IN LISTS HEADERS)
    set_source_files_properties(${h} PROPERTIES HEADER_FILE_ONLY TRUE)
endforeach()
source_group(
    TREE ${CMAKE_CURRENT_SOURCE_DIR}/src
    PREFIX "Source Files"
    FILES ${SOURCES}
)
source_group(
    TREE ${CMAKE_CURRENT_SOURCE_DIR}/include
    PREFIX "Header Files"
    FILES ${HEADERS}
)

# --- Samples ---
option(BUILD_SAMPLES "Build the sample executables." ON)
if(BUILD_SAMPLES)
    add_subdirectory(samples)
endif()

# --- Tests ---
option(BUILD_TESTS "Build the tests." OFF)
if(BUILD_TESTS)
    add_subdirectory(tests)
endif()