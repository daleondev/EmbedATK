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

//------------------------------------------------------
//                   Tuple append
//------------------------------------------------------

template <typename T, typename U>
struct tuple_append;

template <typename... Ts, typename U>
struct tuple_append<std::tuple<Ts...>, U>
{
    using type = std::tuple<Ts..., U>;
};

template <typename T, typename U>
using tuple_append_t = typename tuple_append<T, U>::type;

//------------------------------------------------------
//                   Tuple prepend
//------------------------------------------------------

template <typename T, typename U>
struct tuple_prepend;

template <typename... Ts, typename U>
struct tuple_prepend<std::tuple<Ts...>, U>
{
    using type = std::tuple<U, Ts...>;
};

template <typename T, typename U>
using tuple_prepend_t = typename tuple_prepend<T, U>::type;