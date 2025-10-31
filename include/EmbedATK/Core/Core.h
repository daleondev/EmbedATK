#pragma once

#define EATK_UNUSED(x) (void)x

#if defined(_MSC_VER)
    #define EATK_PACK_BEGIN_X(x)    __pragma(pack(push, x))
    #define EATK_PACK_BEGIN         __pragma(pack(push, 1))
    #define EATK_PACK_END           __pragma(pack(pop))
#elif defined(__GNUC__) || defined(__clang__)
    #define EATK_PACK_BEGIN_X(x)    _Pragma("pack(push, x)")
    #define EATK_PACK_BEGIN         _Pragma("pack(push, 1)")
    #define EATK_PACK_END           _Pragma("pack(pop)")
#else
    #define EATK_PACK_BEGIN_X(x)
    #define EATK_PACK_BEGIN    
    #define EATK_PACK_END      
#endif