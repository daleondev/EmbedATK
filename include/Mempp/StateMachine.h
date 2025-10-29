/*
 * A modern, compile-time hierarchical state machine (HSM).
 *
 * This state machine supports nested states, allowing for more complex and organized
 * state logic. The hierarchy is defined at compile-time, ensuring type safety and
 * high performance.
 *
 * --- Hierarchical State Machine (HSM) Example ---
 *
 * The hierarchy is defined using `SubstateGroup` structs, which specify parent-child
 * relationships and default child states.
 *
 * 1. State Definitions (as an enum):
 *    enum class Defs {
 *        Stopped,
 *        Playing,
 *        Video,
 *        Audio,
 *        Buffering,
 *        ShowingUI
 *    };
 *
 * 2. Hierarchy Definition:
 *    using PlayerHierarchy = std::tuple<
 *        // `Playing` is a parent state. `Video` is its default child.
 *        SubstateGroup<Defs::Playing, Defs::Video, Defs::Audio, Defs::ShowingUI>,
 *
 *        // `Video` is also a parent state. `Buffering` is its default child.
 *        SubstateGroup<Defs::Video, Defs::Buffering>
 *    >;
 *
 * 3. Visual Representation:
 *    This hierarchy code models the following state tree:
 *
 *    (root)
 *      |
 *      ├─ [Stopped]
 *      |
 *      └─ [Playing]            (Parent)
 *         │
 *         ├─ [Video]           (Default Child of Playing, and also a Parent)
 *         │  │
 *         │  └─ [Buffering]    (Default Child of Video)
 *         │
 *         ├─ [Audio]
 *         │
 *         └─ [ShowingUI]
 *
 * --- Key Behaviors ---
 *
 * - Event Bubbling: If an event occurs in the `Buffering` state, the HSM first
 *   checks for transitions from `Buffering`. If none match, it checks for
 *   transitions from the parent `Video`. If still no match, it checks the next
 *   parent, `Playing`. This allows parent states to handle common events for all
 *   their children.
 *
 * - Default Entry: If a transition targets a parent state (e.g., `Playing`),
 *   the HSM will automatically enter its default child (`Video`), and continue
 *   drilling down to the deepest default child (`Buffering`). The full entry
 *   sequence would call onEntry() for `Playing`, then `Video`, then `Buffering`.
 *
 * - Transitions: The state machine correctly calculates the exit and entry paths
 *   for transitions between any two states in the hierarchy, ensuring that
 *   `onExit` and `onEntry` are called correctly for all affected states.
 */
#pragma once

#include "Concepts.h"
#include "Container.h"

#include <reflect>
#include <optional>
#include <deque>
#include <type_traits>
#include <magic_enum/magic_enum_switch.hpp>
#include <ranges>

// ------------------------------------------------------
//                        States
// ------------------------------------------------------

// Base class which concrete states must inherit from
template<IsEnumClass StateIds>
class IState
{
public:
    virtual ~IState() = default;
    
    virtual void onEntry() = 0;
    virtual void onActive(StateIds* substates = nullptr, size_t numSubstates = 0) = 0;
    virtual void onExit() = 0;
};

// State definition (consists of enum class value and implementation class with matching names)
template<auto StateId, IsBaseOf<IState<decltype(StateId)>> StateImpl>
requires IsEnumClass<decltype(StateId)>
struct State
{
    static constexpr auto ID = StateId;
    using Impl = StateImpl;
};

// concept to force a valid State definition
template<typename T>
concept IsState = requires {
    typename T;
    { T::ID };
    typename T::Impl;
} && 
IsEnumClass<decltype(T::ID)> && IsBaseOf<typename T::Impl, IState<std::remove_const_t<decltype(T::ID)>>> &&
magic_enum::enum_name(T::ID) == reflect::type_name<typename T::Impl>();

// For defining all States of a State Machine
template<IsState... S>
using States = std::tuple<S...>;

// ------------------------------------------------------
//                     State Transitions
// ------------------------------------------------------

// State Transition from one State to another on trigger with optional callback
template<IsState From, auto Trig, IsState To, auto Action = std::nullopt>
requires
    (!std::is_same_v<From, To>) && IsEnumClass<decltype(Trig)> &&
    IsOptionallyInvocable<decltype(Action), decltype(From::ID), decltype(Trig), decltype(To::ID)>
struct StateTransition
{
    static constexpr auto FROM      = From::ID;
    static constexpr auto TRIG      = Trig;
    static constexpr auto TO        = To::ID;
    static constexpr auto ACTION    = Action;
};

// concept to force a valid State transition
template<typename T>
concept IsStateTransition = requires {
    typename T;
    { T::FROM };
    { T::TRIG };
    { T::TO };
    { T::ACTION };
} && T::FROM != T::TO &&
IsEnumClass<decltype(T::FROM)> && IsEnumClass<decltype(T::FROM)> && IsEnumClass<decltype(T::TRIG)> &&
IsOptionallyInvocable<decltype(T::ACTION), decltype(T::FROM), decltype(T::TRIG), decltype(T::FROM)>;

// For defining all Transitions of a State Machine
template<IsStateTransition... Transitions>
using StateTransitions = std::tuple<Transitions...>;

// ------------------------------------------------------
//                     State Groups
// ------------------------------------------------------

// Substate-Group for defining hierarchical States
template<IsState Parent, IsState DefaultChild, IsState... Children>
struct SubstateGroup 
{
    static constexpr auto PARENT_STATE = Parent::ID;
    static constexpr auto DEFAULT_CHILD_STATE = DefaultChild::ID;
    using ChildStates = std::tuple<std::integral_constant<decltype(DefaultChild::ID), DefaultChild::ID>, std::integral_constant<decltype(Children::ID), Children::ID>...>;
};

template <
    IsTuple States,
    IsEnumClass Events,
    IsTuple Transitions,
    IsState DefaultState = std::tuple_element_t<0, States>
>
class StateMachine
{
    inline static constexpr size_t  NUM_STATES      = std::tuple_size_v<States>;
    inline static constexpr size_t  NUM_EVENTS      = magic_enum::enum_values<Events>().size();
    inline static constexpr size_t  NUM_TRANSITIONS = std::tuple_size_v<Transitions>;
    // inline static constexpr bool    IS_HIERARCHICAL = std::tuple_size_v<Hierarchy> > 0;
};

// template<IsState DefaultState, IsState... OtherStates>
// struct States 
// {
//     static constexpr auto DEFAULT_ID = DefaultState::ID;

//     using Id = decltype(DefaultState::ID);
// };

// using States = std::tuple<DefaultState::Impl, OtherStates::Impl...>;

// template<auto Parent, auto DefaultChild, auto... Children>
// struct SubstateGroup 
// {
//     static constexpr auto parent = Parent;
//     static constexpr auto defaultChild = DefaultChild;
//     using children = std::tuple<std::integral_constant<decltype(DefaultChild), DefaultChild>, std::integral_constant<decltype(Children), Children>...>;
// };



// // template<typename... T>
// // using States = std::tuple<T...>;

// template <
//     IsEnumClass StateNames,
//     IsBaseOfAll<IState> States,
//     IsEnumClass StateEvents,
//     IsTuple Transitions,
//     IsTuple Hierarchy = std::tuple<>,
//     StateNames DefaultStateName = static_cast<StateNames>(0),
//     size_t MaxDepth = 8
// >
// class StateMachine
// {
//     inline static constexpr size_t  NUM_STATE_NAMES = magic_enum::enum_values<StateNames>().size();
//     inline static constexpr size_t  NUM_STATES      = std::tuple_size_v<States>;
//     inline static constexpr size_t  NUM_TRANSITIONS = std::tuple_size_v<Transitions>;
//     inline static constexpr bool    IS_HIERARCHICAL = std::tuple_size_v<Hierarchy> > 0;

// public:
//     StateMachine()
//     {
//         m_activeStatePath.emplace_back(DefaultStateName);

//         if constexpr (IS_HIERARCHICAL) {
//             auto current = DefaultStateName;
//             while(true) {
//                 constexpr auto child = detail::template get_default_child<current, Hierarchy>();
//                 if constexpr (child == StateNames{}) {
//                     break;
//                 } 
//                 else {
//                     m_activeStatePath.push_back(child);
//                     current = child;
//                 }
//             }
//         }
        
//         for(size_t i = 0; i < m_activeStatePath.size(); ++i) {
//             callOnEntry(m_activeStatePath[i]);
//         }
//     }

//     ~StateMachine()
//     {
//         for(size_t i = m_activeStatePath.size(); i > 0; --i) {
//             callOnExit(m_activeStatePath[i-1]);
//         }
//     }

//     void sendEvent(StateEvents event)
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

//         for(size_t i = 0; i < m_activeStatePath.size(); ++i) {
//             callOnActive(m_activeStatePath[i]);
//         }
//     }

//     constexpr auto currentState() const { return m_activeStatePath[m_activeStatePath.size() - 1]; }
//     const auto& currentStatePath() const { return m_activeStatePath; }

// private:
//     // ------------------------------------------------------
//     //                 Metaprogramming Helpers
//     // ------------------------------------------------------
//     struct detail {
//         template<typename T, typename Tuple>
//         struct has_type;

//         template<typename T, typename... Us>
//         struct has_type<T, std::tuple<Us...>> : std::disjunction<std::is_same<T, Us>...> {};

//         template<auto S, typename T>
//         struct get_parent_impl;

//         template<auto S, typename... Gs>
//         struct get_parent_impl<S, std::tuple<Gs...>> {
//             static constexpr auto get() {
//                 StateNames parent = StateNames{};
//                 ([&]{
//                     using ChildrenTuple = typename Gs::children;
//                     if (has_type<std::integral_constant<StateNames, S>, ChildrenTuple>::value) {
//                         parent = Gs::parent;
//                     }
//                 }(), ...);
//                 return parent;
//             }
//             static constexpr StateNames value = get();
//         };

//         template<auto S, typename H>
//         static constexpr auto get_parent() {
//             if constexpr (std::tuple_size_v<H> == 0) return StateNames{};
//             else return get_parent_impl<S, H>::value;
//         }

//         template<auto S, typename T>
//         struct get_default_child_impl;

//         template<auto S, typename... Gs>
//         struct get_default_child_impl<S, std::tuple<Gs...>> {
//             static constexpr auto get() {
//                 StateNames child = StateNames{};
//                 ([&]{
//                     if (Gs::parent == S) {
//                         child = Gs::default_child;
//                     }
//                 }(), ...);
//                 return child;
//             }
//             static constexpr StateNames value = get();
//         };
        
//         template<auto S, typename H>
//         static constexpr auto get_default_child() {
//             if constexpr (std::tuple_size_v<H> == 0) return StateNames{};
//             else return get_default_child_impl<S, H>::value;
//         }

//         template<auto S1, auto S2, typename H>
//         static constexpr bool is_ancestor() {
//             constexpr auto parent = get_parent<S2, H>();
//             if constexpr (parent == StateNames{}) {
//                 return false;
//             } else if constexpr (parent == S1) {
//                 return true;
//             } else {
//                 return is_ancestor<S1, parent, H>();
//             }
//         }
//     };

//     // ------------------------------------------------------
//     //                 State Implementation Helpers
//     // ------------------------------------------------------
//     template<StateNames Name>
//     IState& nameToImpl() { return std::get<magic_enum::enum_index<Name>()>(m_stateInstances); }

//     void callOnEntry(StateNames name) 
//     {
//         magic_enum::enum_switch([this](auto n) { nameToImpl<n.value>().onEntry(); }, name);
//     }
//     void callOnExit(StateNames name) 
//     {
//         magic_enum::enum_switch([this](auto n) { nameToImpl<n.value>().onExit(); }, name);
//     }
//     void callOnActive(StateNames name) 
//     {
//         magic_enum::enum_switch([this](auto n) { nameToImpl<n.value>().onActive(); }, name);
//     }

//     // ------------------------------------------------------
//     //                 Event & Transition Logic
//     // ------------------------------------------------------
//     void processEvent(StateEvents event) {
//         if constexpr (IS_HIERARCHICAL) {
//             for (size_t i = m_activeStatePath.size(); i > 0; --i) {
//                 if (findAndExecuteTransition(m_activeStatePath[i-1], event)) {
//                     return;
//                 }
//             }
//         } else {
//             findAndExecuteTransition(m_activeStatePath[0], event);
//         }
//     }

//     template<size_t I = 0>
//     bool findAndExecuteTransition(StateNames state, StateEvents event) {
//         if constexpr (I < NUM_TRANSITIONS) {
//             using Transition = std::tuple_element_t<I, Transitions>;
//             if (Transition::from == state && Transition::trig == event) {
//                 executeTransition<Transition>();
//                 return true;
//             }
//             return findAndExecuteTransition<I + 1>(state, event);
//         }
//         return false;
//     }

//     template<typename Transition>
//     void executeTransition() 
//     {
//         if constexpr (IS_HIERARCHICAL) {
            
//             StaticVector<StateNames, MaxDepth> newPath(1, Transition::to);

//             auto current = Transition::to;
//             while(true) {
//                 constexpr auto child = detail::template get_default_child<current, Hierarchy>();
//                 if constexpr (child == StateNames{}) {
//                     break;
//                 }
//                 else {
//                     newPath.push_back(child);
//                     current = child;
//                 }
//             }

//             size_t diverge_idx = 0;
//             for(size_t i = 0; i < m_activeStatePath.size() && i < newPath.size(); ++i) {
//                 if(m_activeStatePath[i] != newPath[i]) break;
//                 diverge_idx = i + 1;
//             }

//             // Exit states
//             for(size_t i = m_activeStatePath.size(); i > diverge_idx; --i) {
//                 callOnExit(m_activeStatePath[i-1]);
//             }

//             // Action
//             if constexpr (!std::is_same_v<std::decay_t<decltype(Transition::action)>, std::nullopt_t>) {
//                 if constexpr (is_optional_v<decltype(Transition::action)>) {
//                     if (Transition::action) (*Transition::action)(Transition::from, Transition::trig, Transition::to);
//                 }
//                 else {
//                     Transition::action(Transition::from, Transition::trig, Transition::to);
//                 }
//             }

//             // Rebuild active path
//             m_activeStatePath.resize(diverge_idx);
//             for(size_t i = diverge_idx; i < newPath.size(); ++i) {
//                 m_activeStatePath.push_back(newPath[i]);
//             }

//             // Enter states
//             for(size_t i = diverge_idx; i < m_activeStatePath.size(); ++i) {
//                 callOnEntry(m_activeStatePath[i]);
//             }

//         } else {
//             // exit old state
//             StateNames& activeState = m_activeStatePath[0];
//             callOnExit(activeState);

//             // do transition
//             if constexpr (!std::is_same_v<std::decay_t<decltype(Transition::action)>, std::nullopt_t>) {
//                  if constexpr (is_optional_v<decltype(Transition::action)>) {
//                     if (Transition::action) (*Transition::action)(Transition::from, Transition::trig, Transition::to);
//                 } else {
//                     Transition::action(Transition::from, Transition::trig, Transition::to);
//                 }
//             }

//             // enter new state
//             activeState = Transition::to;
//             callOnEntry(activeState);
//         }
//     }

//     // ------------------------------------------------------
//     //                          Data
//     // ------------------------------------------------------
//     States m_stateInstances;
//     StaticVector<StateNames, MaxDepth> m_activeStatePath;
//     std::deque<StateEvents> m_eventQueue;

//     // ------------------------------------------------------
//     //                    Compile Time Checks
//     // ------------------------------------------------------
//     static_assert(NUM_STATE_NAMES > 0 && NUM_STATE_NAMES == NUM_STATES, "Invalid number of States");

//     template <size_t I>
//     static constexpr bool CheckNameMatch() 
//     {
//         using State = std::tuple_element_t<I, States>;
//         constexpr std::string_view state = reflect::type_name<State>();

//         constexpr StateNames stateName = static_cast<StateNames>(I);
//         constexpr std::string_view name = magic_enum::enum_name(stateName);

//         return state == name;
//     }

//     template <size_t... I>
//     static constexpr bool ValidateNames(std::index_sequence<I...>) 
//     {
//         return (CheckNameMatch<I>() && ...);
//     }

//     static_assert(ValidateNames(std::make_index_sequence<NUM_STATE_NAMES>{}), "Invalid Name of State");

//     template<typename T>
//     struct TransitionChecker;

//     template<auto From, auto Trig, auto To, auto Action>
//     struct TransitionChecker<StateTransition<From, Trig, To, Action>>
//     {
//         static constexpr bool valid =
//             From != To &&
//             std::is_same_v<decltype(From), StateNames> &&
//             std::is_same_v<decltype(Trig), StateEvents> &&
//             std::is_same_v<decltype(To), StateNames> &&
//             magic_enum::enum_contains<StateNames>(From) &&
//             magic_enum::enum_contains<StateEvents>(Trig) &&
//             magic_enum::enum_contains<StateNames>(To);
//     };

//     template <size_t... I>
//     static constexpr bool ValidateTransitions(std::index_sequence<I...>)
//     {
//         if constexpr (sizeof...(I) > 0) {
//             return (TransitionChecker<std::tuple_element_t<I, Transitions>>::valid && ...);
//         }
//         return true;
//     }

//     static_assert(ValidateTransitions(std::make_index_sequence<NUM_TRANSITIONS>{}), "Invalid State Transition");
// };
