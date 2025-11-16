#pragma once

#include "Logger.h"

#if defined(EATK_ENABLE_ASSERTS)
    #if defined(EATK_PLATFORM_WINDOWS)
        #include <intrin.h>
        #define EATK_DEBUG_BREAK() __debugbreak()
    #elif defined(EATK_PLATFORM_LINUX)
        #include <csignal>
        #define EATK_DEBUG_BREAK() std::raise(SIGTRAP)
    #elif defined(EATK_PLATFORM_ARM)
        #if defined(__GNUC__) && !defined(__llvm__) // GCC
            #define EATK_DEBUG_BREAK() __asm volatile("bkpt #0")
        #elif defined(__llvm__) // Clang
            #define EATK_DEBUG_BREAK() __builtin_trap()
        #elif defined(__CC_ARM) // Keil MDK
            #define EATK_DEBUG_BREAK() __bkpt(0)
        #elif defined(__ICCARM__) // IAR
            #define EATK_DEBUG_BREAK() __DebugBreak()
        #else
            #define EATK_DEBUG_BREAK() std::abort()
        #endif
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