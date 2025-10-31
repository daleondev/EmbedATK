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