# --- Fetch GoogleTest ---
include(FetchContent)
FetchContent_Declare(
    GTest
    GIT_REPOSITORY https://github.com/google/googletest.git
    GIT_TAG        v1.17.0
)
FetchContent_MakeAvailable(GTest)

# --- OSAL Tests ---
add_executable(osal_tests 
    ${CMAKE_CURRENT_SOURCE_DIR}/OSAL/osal_tests.cpp
)
target_link_libraries(osal_tests
    PRIVATE
        GTest::gtest_main
        GTest::gmock
        EmbedATK::EmbedATK
)

# --- Memory Tests ---
add_executable(memory_tests 
    ${CMAKE_CURRENT_SOURCE_DIR}/Memory/memory_tests.cpp
)
target_link_libraries(memory_tests
    PRIVATE
        GTest::gtest_main
        GTest::gmock
        EmbedATK::EmbedATK
)
target_compile_options(memory_tests
    PRIVATE
        -Wno-unused-result
        -Wno-nonnull
)

# --- Statemachine Tests ---
add_executable(statemachine_tests 
    ${CMAKE_CURRENT_SOURCE_DIR}/StateMachine/statemachine_tests.cpp
)
target_link_libraries(statemachine_tests
    PRIVATE
        GTest::gtest_main
        GTest::gmock
        EmbedATK::EmbedATK
)

# --- Register with CTest ---
enable_testing()
include(GoogleTest)
gtest_discover_tests(memory_tests)
gtest_discover_tests(statemachine_tests)