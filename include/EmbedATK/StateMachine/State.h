#pragma once

#include "EmbedATK/Core/Concepts.h"

// Base class which concrete states must inherit from
template<auto StateId>
class IState
{
public:
    using IdType = decltype(StateId);
    inline static constexpr IdType ID = StateId;
    inline static constexpr std::string_view NAME = magic_enum::enum_name<StateId>();

    virtual ~IState() = default;
    
    virtual void onEntry() = 0;
    virtual void onActive(IdType* subStates, size_t numSubStates) = 0;
    virtual void onExit() = 0;

    static_assert(magic_enum::is_scoped_enum_v<IdType>);
};

// Concept for a valid state
template<class T>
concept IsState = requires { 
    requires is_templated_base_of_v<IState, T>;
    requires magic_enum::enum_name<T::ID>() == reflect::type_name<T>();
};

// Concept for a valid collection of states
template<class T, class... Ts>
concept AreStates = requires { 
    requires IsState<T>;
    requires (
        (
            IsState<Ts> && 
            std::is_same_v<typename T::IdType, typename Ts::IdType>
        )
        && ...
    );
    requires are_types_unique_v<T, Ts...>;
};

// For defining all States of a State Machine
template<IsState DefaultState, IsState... OtherStates>
requires AreStates<DefaultState, OtherStates...>
using States = std::tuple<DefaultState, OtherStates...>;

template<typename T>
inline constexpr bool is_valid_states_tuple_v = false;

template<typename T, typename... Ts>
inline constexpr bool is_valid_states_tuple_v<std::tuple<T, Ts...>> = AreStates<T, Ts...>;

template<typename T>
concept IsStatesTuple = is_tuple_v<T> && is_valid_states_tuple_v<T>;