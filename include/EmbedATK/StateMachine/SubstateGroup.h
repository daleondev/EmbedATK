#pragma once

#include "State.h"

// Substate-Group for defining hierarchical States
template<IsState Parent, IsState DefaultChild, IsState... Children>
struct SubstateGroup 
{
    static_assert(AreStates<DefaultChild, Children...>);
    static_assert(std::is_same_v<typename Parent::IdType, typename DefaultChild::IdType>);
    static_assert(!std::is_same_v<Parent, DefaultChild>);
    static_assert((!std::is_same_v<Parent, Children> && ...));
    
    using ParentState       = Parent;
    using DefaultChildState = DefaultChild;
    using ChildStates       = std::tuple<DefaultChild, Children...>;
};

// concept to force a valid substate group
template<typename T>
concept IsSubstateGroup = requires {
    typename T;
    typename T::ParentState;
    typename T::DefaultChildState;
    typename T::ChildStates;

    requires std::is_same_v<typename T::ParentState::IdType, typename  T::DefaultChildState::IdType>;
    requires !std::is_same_v<typename T::ParentState, typename T::DefaultChildState>;
    requires all_tuple_types_unique_v<typename T::ChildStates>;
};

// Concept for a valid collection of substate groups
template<class... Ts>
concept AreSubstateGroups = requires { 
    requires (IsSubstateGroup<Ts> && ...);
    requires all_tuple_types_unique_v<std::tuple<Ts...>>;
    requires all_tuple_types_unique_v<std::tuple<typename Ts::ParentState...>>;
    requires all_tuple_types_unique_v<decltype(std::tuple_cat(std::declval<typename Ts::ChildStates>()...))>;
};

// For defining hierarchy substate groups of a State Machine
template<IsSubstateGroup... Groups>
requires AreSubstateGroups<Groups...>
using StateHierarchy = std::tuple<Groups...>;

template<typename T>
inline constexpr bool is_valid_substate_groups_tuple_v = false;

template<typename... Ts>
inline constexpr bool is_valid_substate_groups_tuple_v<std::tuple<Ts...>> = AreSubstateGroups<Ts...>;

template<typename T>
concept IsSubstateGroupsTuple = is_tuple_v<T> && is_valid_substate_groups_tuple_v<T>;