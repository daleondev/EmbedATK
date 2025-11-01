#pragma once

#include "Logger.h"

#if defined(EATK_ENABLE_ASSERTS)
    #if defined(EATK_PLATFORM_WINDOWS)
        #include <intrin.h>
        #define EATK_DEBUG_BREAK() __debugbreak()
    #elif defined(EATK_PLATFORM_LINUX) || defined(EATK_PLATFORM_ARM)
        #include <csignal>
        #define EATK_DEBUG_BREAK() std::raise(SIGTRAP)
    #else
        #define EATK_DEBUG_BREAK() std::abort()
    #endif

    #define EATK_ASSERT(x, msg, ...) \
        do { \
            if (!(x)) { \
                EATK_FATAL_NOW("Assertion failed: " msg, ##__VA_ARGS__); \
                EATK_DEBUG_BREAK(); \
            } \
        } while (false)
#else
    #define EATK_ASSERT(x, msg, ...) ((void)0)
#endif