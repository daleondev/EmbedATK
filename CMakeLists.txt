# --- Project Setup ---
cmake_minimum_required(VERSION 3.20)
project(Mempp
    VERSION 0.1.0
    DESCRIPTION "C++ Memory Utilities"
    LANGUAGES CXX
)

# --- External Libraries ---
add_subdirectory(
    ${CMAKE_CURRENT_SOURCE_DIR}/external/magic_enum
)

# --- Library Target Definition ---
add_library(Mempp INTERFACE)
add_library(Mempp::Mempp ALIAS Mempp)

# --- Include Directories ---
target_include_directories(Mempp
    INTERFACE
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/external/reflect>
)

# --- Link Libraries ---
target_link_libraries(Mempp
    INTERFACE
        magic_enum::magic_enum
)

# --- Compile Features ---
target_compile_features(Mempp
    INTERFACE
        cxx_std_20
)

# --- Compile Options ---
target_compile_options(Mempp
    INTERFACE
        $<$<CXX_COMPILER_ID:MSVC>:/W4>
        $<$<NOT:$<CXX_COMPILER_ID:MSVC>>:-Wall -Wextra>
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