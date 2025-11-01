#pragma once

#include "State.h"

// State Transition from one State to another on trigger with optional callback
template<IsState From, auto Trig, IsState To, auto Callback = std::nullopt>
struct StateTransition
{
    using OldState                  = From;
    using NewState                  = To;
    static constexpr auto TRIG      = Trig;
    static constexpr auto CALLBACK  = Callback;

    static_assert(!std::is_same_v<From, To>);
    static_assert(magic_enum::is_scoped_enum_v<decltype(Trig)>);
    static_assert(is_optionally_invocable_v<decltype(Callback), decltype(From::ID), decltype(Trig), decltype(To::ID)>);
};

// concept to force a valid state transition
template<typename T>
concept IsStateTransition = requires {
    typename T;
    typename T::OldState;
    typename T::NewState;
    { T::TRIG };
    { T::CALLBACK };

    requires AreStates<typename T::OldState, typename T::NewState> && magic_enum::is_scoped_enum_v<decltype(T::TRIG)>;
    requires is_optionally_invocable_v<decltype(T::CALLBACK), decltype(T::OldState::ID), decltype(T::TRIG), decltype(T::NewState::ID)>;
};

namespace detail {
    template <size_t I, typename TransitionTuple, size_t... Js>
    consteval bool check_inner_loop(std::index_sequence<Js...>) {
        return ([&] {
            if constexpr (I >= Js) {
                return true;
            } else {
                using T1 = std::tuple_element_t<I, TransitionTuple>;
                using T2 = std::tuple_element_t<Js, TransitionTuple>;
                if (std::is_same_v<typename T1::OldState, typename T2::OldState> && T1::TRIG == T2::TRIG) {
                    return false;
                }
                return true;
            }
        }() && ...);
    }

    template <typename TransitionTuple, size_t... Is>
    consteval bool check_determinism(std::index_sequence<Is...>) {
        constexpr size_t N = std::tuple_size_v<TransitionTuple>;
        return (check_inner_loop<Is, TransitionTuple>(std::make_index_sequence<N>()) && ...);
    }
}

template <typename... Transitions>
inline constexpr bool are_transitions_deterministic_v = detail::check_determinism<std::tuple<Transitions...>>(
    std::make_index_sequence<sizeof...(Transitions)>()
);

// Concept for a valid collection of state transitions
template<class T, class... Ts>
concept AreStateTransitions = requires { 
    requires IsStateTransition<T>;
    requires (
        (
            IsStateTransition<Ts> && 
            std::is_same_v<decltype(T::TRIG), decltype(Ts::TRIG)>
        )
        && ...
    );
    requires are_types_unique_v<T, Ts...>;
    requires are_transitions_deterministic_v<T, Ts...>;
};

// For defining all Transitions of a State Machine
template<IsStateTransition FirstTransition, IsStateTransition... OtherTransitions>
requires AreStateTransitions<FirstTransition, OtherTransitions...>
using StateTransitions = std::tuple<FirstTransition, OtherTransitions...>;

template<typename T>
inline constexpr bool is_valid_state_transitions_tuple_v = false;

template<typename T, typename... Ts>
inline constexpr bool is_valid_state_transitions_tuple_v<std::tuple<T, Ts...>> = AreStateTransitions<T, Ts...>;

template<typename T>
concept IsStateTransitionsTuple = is_tuple_v<T> && is_valid_state_transitions_tuple_v<T>;