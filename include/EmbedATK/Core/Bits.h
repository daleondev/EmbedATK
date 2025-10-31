#pragma once

template<typename T>
constexpr size_t EATK_SIZE_BITS() 
{ 
    constexpr size_t BITS_PER_BYTE = 8;
    return BITS_PER_BYTE*sizeof(T);
}

template<size_t I, typename T>
requires std::integral<T> 
constexpr T EATK_BIT() 
{ 
    static_assert(I < EATK_SIZE_BITS<T>());
    return (T{1} << I);
}

template<size_t I, typename T>
constexpr void EATK_SET_BIT(T& data) 
{ 
    data |= EATK_BIT<I, T>();
}

template<size_t I, typename T>
constexpr void EATK_RESET_BIT(T& data) 
{ 
    data &= ~EATK_BIT<I, T>();
}

template<size_t I, typename T>
constexpr bool EATK_CHECK_BIT(T data) 
{ 
    return (data & EATK_BIT<I, T>()) != 0;
}

template<size_t I, typename T, size_t... Is>
constexpr void EATK_CHECK_BITS_IMPL(T data, bool* results, std::integer_sequence<size_t, Is...>)
{
    ( (results[Is] = EATK_CHECK_BIT<I + Is, T>(data)), ... );
}

template<size_t N, size_t I, typename T>
constexpr std::array<bool, N> EATK_CHECK_BITS(T data)
{
    std::array<bool, N> result;
    EATK_CHECK_BITS_IMPL<I, T>(data, result.data(), std::make_integer_sequence<size_t, N>{});
    return result;
}

template<size_t I, typename T, size_t... Is>
constexpr void EATK_CREATE_MASK_IMPL(T& mask, std::integer_sequence<size_t, Is...>)
{
    ( (EATK_SET_BIT<I + Is, T>(mask)), ... );
}

template<size_t I, size_t N, typename T>
constexpr T EATK_CREATE_MASK()
{
    static_assert(I + N <= (8 * sizeof(T)));
    T mask = 0;
    EATK_CREATE_MASK_IMPL<I, T>(mask, std::make_integer_sequence<size_t, N>{});
    return mask;
}

template<size_t I, size_t N, typename T>
constexpr void EATK_SET_MASKED(T& data, T val) 
{ 
    constexpr T mask = EATK_CREATE_MASK<I, N, T>();
    data &= ~mask;
    data |= ((val << I) & mask);
}

template<size_t I, size_t N, typename T>
constexpr T EATK_GET_MASKED(T data) 
{ 
    constexpr T mask = EATK_CREATE_MASK<I, N, T>();
    return (data & mask) >> I;
}