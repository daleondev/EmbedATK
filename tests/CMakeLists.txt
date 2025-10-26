# --- Fetch GoogleTest ---
include(FetchContent)
FetchContent_Declare(
    GTest
    GIT_REPOSITORY https://github.com/google/googletest.git
    GIT_TAG        v1.17.0
)
FetchContent_MakeAvailable(GTest)

# --- Define Tests ---
add_executable(mempp_tests 
    ${CMAKE_CURRENT_SOURCE_DIR}/mempp_tests.cpp
)
target_link_libraries(mempp_tests
    PRIVATE
        GTest::gtest_main
        GTest::gmock
        Mempp::Mempp
)
target_compile_options(mempp_tests
    PRIVATE
        -Wno-unused-result
        -Wno-nonnull
)

# --- Register with CTest ---
enable_testing()
include(GoogleTest)
gtest_discover_tests(mempp_tests)