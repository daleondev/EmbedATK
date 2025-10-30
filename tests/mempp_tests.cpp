#include <gtest/gtest.h>
#include <gmock/gmock.h>

// #include "memory_tests.h"
// #include "polymorphic_tests.h"
// #include "containers_tests.h"
#include "statemachine_tests.h"

int main(int argc, char* argv[])
{
    ::testing::InitGoogleTest(&argc, argv);

    return RUN_ALL_TESTS();
}