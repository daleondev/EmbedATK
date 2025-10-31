#pragma once

#include "EmbedATK/Core/Concepts.h"
#include "EmbedATK/Memory/Container.h"


// ------------------------------------------------------
//                        States
// ------------------------------------------------------

// Base class which concrete states must inherit from
template<auto StateId>
class IState
{
public:
    using IdType = decltype(StateId);
    inline static constexpr IdType ID = StateId;

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

// ------------------------------------------------------
//                     State Transitions
// ------------------------------------------------------

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

// ------------------------------------------------------
//                     SubState Groups
// ------------------------------------------------------

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

// ------------------------------------------------------
//                     State Machine
// ------------------------------------------------------

template<typename T>
inline constexpr bool is_valid_are_states_tuple_v = false; // Default for non-tuples

template<typename T, typename... Ts>
inline constexpr bool is_valid_are_states_tuple_v<std::tuple<T, Ts...>> = AreStates<T, Ts...>;

template<typename T>
concept IsValidStatesTuple = is_tuple_v<T> && is_valid_are_states_tuple_v<T>;

template<typename T>
inline constexpr bool is_valid_are_state_transitions_tuple_v = false;

template<typename T, typename... Ts>
inline constexpr bool is_valid_are_state_transitions_tuple_v<std::tuple<T, Ts...>> = AreStateTransitions<T, Ts...>;

template<typename T>
concept IsValidTransitionsTuple = is_tuple_v<T> && is_valid_are_state_transitions_tuple_v<T>;

template<typename T>
inline constexpr bool is_valid_are_substate_groups_tuple_v = false;

template<typename... Ts>
inline constexpr bool is_valid_are_substate_groups_tuple_v<std::tuple<Ts...>> = AreSubstateGroups<Ts...>;

template<typename T>
concept IsValidHierarchyTuple = is_tuple_v<T> && is_valid_are_substate_groups_tuple_v<T>;

namespace detail {
    template<typename StatesTuple, typename EventsEnum, typename... Transitions>
    consteval bool validate_transitions_impl(std::tuple<Transitions...>) {
        return (
            (
                is_type_in_tuple_v<typename Transitions::OldState, StatesTuple> &&
                is_type_in_tuple_v<typename Transitions::NewState, StatesTuple> &&
                std::is_same_v<std::decay_t<decltype(Transitions::TRIG)>, EventsEnum>
            ) 
            && ...
        );
    }
}
template<typename TransitionsTuple, typename StatesTuple, typename EventsEnum>
inline constexpr bool all_transitions_are_valid_v = detail::validate_transitions_impl<StatesTuple, EventsEnum>(TransitionsTuple{});

namespace detail {
    template<typename StatesTuple, typename... Groups>
    consteval bool validate_hierarchy_impl(std::tuple<Groups...>) {
        auto check_children = []<typename... Children>(std::tuple<Children...>) {
            return (is_type_in_tuple_v<Children, StatesTuple> && ...);
        };

        return (
            (
                is_type_in_tuple_v<typename Groups::ParentState, StatesTuple> &&
                check_children(typename Groups::ChildStates{})
            ) 
            && ...
        ); 
    }
}
template<typename HierarchyTuple, typename StatesTuple>
inline constexpr bool all_hierarchy_states_are_valid_v = detail::validate_hierarchy_impl<StatesTuple>(HierarchyTuple{});

template <
    IsValidStatesTuple States,
    IsEnumClass Events,
    IsValidTransitionsTuple Transitions,
    IsValidHierarchyTuple Hierarchy = StateHierarchy<>,
    size_t MaxDepth = 8
>
class StateMachine
{
    inline static constexpr size_t  NUM_STATES      = std::tuple_size_v<States>;
    inline static constexpr size_t  NUM_EVENTS      = magic_enum::enum_values<Events>().size();
    inline static constexpr size_t  NUM_TRANSITIONS = std::tuple_size_v<Transitions>;
    inline static constexpr bool    IS_HIERARCHICAL = std::tuple_size_v<Hierarchy> > 0;

    static_assert(NUM_STATES > 0, "A state machine must have at least one state.");
    static_assert(NUM_EVENTS > 0, "A state machine must have at least one event.");
    static_assert(NUM_TRANSITIONS > 0, "A state machine must have at least one transition.");

    static_assert(all_transitions_are_valid_v<Transitions, States, Events>, "One or more 'StateTransition' definitions are invalid");
    static_assert(all_hierarchy_states_are_valid_v<Hierarchy, States>, "One or more 'SubstateGroup' definitions are invalid");

    using StateId = std::tuple_element_t<0, States>::IdType;

// public:
//     StateMachine()
//     {
//         auto defaultStateId = std::tuple_element_t<0, States>::ID;
        
//         if constexpr (IS_HIERARCHICAL) {
//             m_activeStatePath = getPathToRoot(defaultStateId);
//         } else {
//             m_activeStatePath.push_back(defaultStateId);
//         }
        
//         for(size_t i = 0; i < m_activeStatePath.size(); ++i) {
//             callOnEntry(m_activeStatePath[i]);
//         }

//         enterDefaultChildren();
//     }

//     ~StateMachine()
//     {
//         for(size_t i = m_activeStatePath.size(); i > 0; --i) {
//             callOnExit(m_activeStatePath[i-1]);
//         }
//     }

//     void sendEvent(Events event)
//     {
//         m_eventQueue.push_back(event);
//     }

//     void update()
//     {
//         while (!m_eventQueue.empty()) {
//             auto event = m_eventQueue.front();
//             m_eventQueue.pop_front();
//             processEvent(event);
//         }

//         StaticVector<StateId, MaxDepth-1> subStates{};
//         for (auto state : m_activeStatePath | std::views::reverse) {
//             callOnActive(state, subStates.data(), subStates.size());
//             subStates.push_back(state);
//         }
//     }

//     constexpr auto currentState() const { return m_activeStatePath[m_activeStatePath.size() - 1]; }
//     const auto& currentStatePath() const { return m_activeStatePath; }

// private:
//     // ------------------------------------------------------
//     //                 Event processing
//     // ------------------------------------------------------
//     void processEvent(Events event)
//     {
//         if constexpr (NUM_TRANSITIONS > 0)
//         {
//             std::optional<StateId> handlerState = m_activeStatePath[m_activeStatePath.size() - 1];
//             while(handlerState)
//             {
//                 bool transitionFound = false;
//                 findAndProcessTransition(*handlerState, event, transitionFound);
//                 if(transitionFound) {
//                     return;
//                 }
//                 if constexpr (IS_HIERARCHICAL) {
//                     handlerState = findParent(*handlerState);
//                 } else {
//                     handlerState = std::nullopt;
//                 }
//             }
//         }
//     }

//     template<size_t I = 0>
//     void findAndProcessTransition(StateId handlerState, Events event, bool& transitionFound)
//     {
//         if constexpr (I < NUM_TRANSITIONS)
//         {
//             using Transition = std::tuple_element_t<I, Transitions>;
//             if (Transition::OldState::ID == handlerState && Transition::TRIG == event)
//             {
//                 transitionFound = true;

//                 if constexpr (!std::is_same_v<std::decay_t<decltype(Transition::CALLBACK)>, std::nullopt_t>) {
//                     if constexpr (is_optional_v<decltype(Transition::CALLBACK)>) {
//                         if (Transition::CALLBACK) (*Transition::CALLBACK)(Transition::OldState::ID, Transition::TRIG, Transition::NewState::ID);
//                     }
//                     else {
//                         Transition::CALLBACK(Transition::OldState::ID, Transition::TRIG, Transition::NewState::ID);
//                     }
//                 }

//                 changeState<Transition>();
//                 return;
//             }
            
//             findAndProcessTransition<I + 1>(handlerState, event, transitionFound);
//         }
//     }

//     template<IsStateTransition T>
//     void changeState()
//     {
//         constexpr auto toStateId = T::NewState::ID;

//         auto toPath = getPathToRoot(toStateId);

//         std::optional<size_t> lcaIndex;
//         if constexpr (IS_HIERARCHICAL) {
//             size_t minPathSize = std::min(m_activeStatePath.size(), toPath.size());
//             for (size_t i = 0; i < minPathSize; ++i) {
//                 if (m_activeStatePath[i] == toPath[i]) {
//                     lcaIndex = i;
//                 } else {
//                     break;
//                 }
//             }
//         }

//         const size_t exitUntil = lcaIndex.has_value() ? lcaIndex.value() + 1 : 0;
//         for(size_t i = m_activeStatePath.size(); i > exitUntil; --i) {
//             callOnExit(m_activeStatePath[i-1]);
//         }

//         StaticVector<StateId, MaxDepth> newPath;
//         const size_t pathCopyUntil = lcaIndex.has_value() ? lcaIndex.value() + 1 : 0;
//         for(size_t i = 0; i < pathCopyUntil; ++i) {
//             newPath.push_back(m_activeStatePath[i]);
//         }

//         const size_t entryFrom = lcaIndex.has_value() ? lcaIndex.value() + 1 : 0;
//         for(size_t i = entryFrom; i < toPath.size(); ++i) {
//             newPath.push_back(toPath[i]);
//             callOnEntry(toPath[i]);
//         }
        
//         m_activeStatePath = newPath;

//         enterDefaultChildren();
//     }

//     // ------------------------------------------------------
//     //                 State Implementation Helpers
//     // ------------------------------------------------------
//     template<auto Id, size_t I = 0>
//     constexpr auto& stateImpl() {
//         if constexpr (I == NUM_STATES) {
//             static_assert(I != NUM_STATES, "State ID not found");
//         } else if constexpr (std::tuple_element_t<I, States>::ID == Id) {
//             return std::get<I>(m_states).impl;
//         } else {
//             return stateImpl<Id, I + 1>();
//         }
//     }

//     constexpr void callOnEntry(StateId state) 
//     {
//         magic_enum::enum_switch([this](auto s) { stateImpl<s.value>().onEntry(); }, state);
//     }
//     constexpr void callOnExit(StateId state) 
//     {
//         magic_enum::enum_switch([this](auto s) { stateImpl<s.value>().onExit(); }, state);
//     }
//     constexpr void callOnActive(StateId state, StateId* subStates, size_t numSubStates) 
//     {
//         magic_enum::enum_switch([this, subStates, numSubStates](auto s) { stateImpl<s.value>().onActive(subStates, numSubStates); }, state);
//     }

//     // ------------------------------------------------------
//     //                  Hierarchy Helpers
//     // ------------------------------------------------------
//     void enterDefaultChildren()
//     {
//         if constexpr (IS_HIERARCHICAL) {
//             auto lastState = m_activeStatePath[m_activeStatePath.size() - 1];
//             auto defaultChild = findDefaultChild(lastState);
//             while(defaultChild) {
//                 m_activeStatePath.push_back(*defaultChild);
//                 callOnEntry(*defaultChild);
//                 lastState = *defaultChild;
//                 defaultChild = findDefaultChild(lastState);
//             }
//         }
//     }

//     template<size_t G = 0, size_t C = 0>
//     constexpr auto findParent(StateId childId) -> std::optional<StateId> {
//         if constexpr (!IS_HIERARCHICAL) {
//             return std::nullopt;
//         } else if constexpr (G == std::tuple_size_v<Hierarchy>) {
//             return std::nullopt;
//         } else {
//             using Group = std::tuple_element_t<G, Hierarchy>;
//             using Children = typename Group::ChildStates;
//             if constexpr (C < std::tuple_size_v<Children>) {
//                 if (std::tuple_element_t<C, Children>::ID == childId) {
//                     return Group::ParentState::ID;
//                 }
//                 return findParent<G, C + 1>(childId);
//             } else {
//                 return findParent<G + 1, 0>(childId);
//             }
//         }
//     }

//     template<size_t I = 0>
//     constexpr auto findDefaultChild(StateId parentId) -> std::optional<StateId> {
//         if constexpr (!IS_HIERARCHICAL) {
//             return std::nullopt;
//         } else if constexpr (I == std::tuple_size_v<Hierarchy>) {
//             return std::nullopt;
//         } else {
//             using Group = std::tuple_element_t<I, Hierarchy>;
//             if (Group::ParentState::ID == parentId) {
//                 return Group::DefaultChildState::ID;
//             } else {
//                 return findDefaultChild<I + 1>(parentId);
//             }
//         }
//     }

//     constexpr auto getPathToRoot(StateId stateId) {
//         StaticVector<StateId, MaxDepth> path;
//         if constexpr (IS_HIERARCHICAL) {
//             path.push_back(stateId);
//             auto parent = findParent(stateId);
//             while(parent) {
//                 path.push_back(*parent);
//                 parent = findParent(*parent);
//             }
//             std::reverse(path.begin(), path.end());
//         } else {
//             path.push_back(stateId);
//         }
//         return path;
//     }

//     // ------------------------------------------------------
//     //                          Data
//     // ------------------------------------------------------
//     States m_states;
//     StaticVector<StateId, MaxDepth> m_activeStatePath;
//     std::deque<Events> m_eventQueue;
};