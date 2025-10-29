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
template<IsEnumClass StateId>
class IState
{
public:
    virtual ~IState() = default;
    
    virtual void onEntry() = 0;
    virtual void onActive(StateId* substates = nullptr, size_t numSubstates = 0) = 0;
    virtual void onExit() = 0;
};

// State definition (consists of enum class value and implementation class with matching names)
template<auto StateId, IsBaseOf<IState<decltype(StateId)>> StateImpl>
requires 
    IsEnumClass<decltype(StateId)> &&
    (magic_enum::enum_name(StateId) == reflect::type_name<StateImpl>())
struct State
{
    using IdType                = decltype(StateId);
    static constexpr IdType ID  = StateId;
    StateImpl impl;
};

// concept to force a valid State definition
template<typename T>
concept IsState = requires {
    typename T;
    typename T::IdType;
    { T::ID };
    { T::impl };
} && 
IsEnumClass<typename T::IdType> && IsBaseOf<decltype(T::impl), IState<std::remove_const_t<typename T::IdType>>> &&
magic_enum::enum_name(T::ID) == reflect::type_name<decltype(T::impl)>();

// For defining all States of a State Machine
template<IsState DefaultState, IsState... OtherStates>
requires (std::is_same_v<decltype(DefaultState::ID), decltype(OtherStates::ID)> && ...)
using States = std::tuple<DefaultState, OtherStates...>;

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
    using OldState                  = From;
    using NewState                  = To;
    static constexpr auto TRIG      = Trig;
    static constexpr auto ACTION    = Action;
};

// concept to force a valid State transition
template<typename T>
concept IsStateTransition = requires {
    typename T;
    typename T::OldState;
    typename T::NewState;
    { T::TRIG };
    { T::ACTION };
} && 
(
    IsState<typename T::OldState> && IsEnumClass<decltype(T::TRIG)> && IsState<typename T::NewState> &&
    IsOptionallyInvocable<decltype(T::ACTION), decltype(T::OldState::ID), decltype(T::TRIG), decltype(T::NewState::ID)> &&
    !std::is_same_v<typename T::OldState, typename T::NewState> &&
    std::is_same_v<decltype(T::OldState::ID), decltype(T::NewState::ID)>
);

// For defining all Transitions of a State Machine
template<IsStateTransition FirstTransition, IsStateTransition... OtherTransitions>
requires (std::is_same_v<decltype(FirstTransition::OldState::ID), decltype(OtherTransitions::OldState::ID)> && ...)
using StateTransitions = std::tuple<FirstTransition, OtherTransitions...>;

// ------------------------------------------------------
//                     SubState Groups
// ------------------------------------------------------

// Substate-Group for defining hierarchical States
template<IsState Parent, IsState DefaultChild, IsState... Children>
requires 
    (!std::is_same_v<Parent, DefaultChild>) && 
    ((!std::is_same_v<Parent, Children> && !std::is_same_v<DefaultChild, Children>) && ...)
struct SubstateGroup 
{
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
} && 
(
    IsState<typename T::ParentState> && IsState<typename T::DefaultChildState> &&
    !std::is_same_v<typename T::ParentState, typename T::DefaultChildState> &&
    std::is_same_v<decltype(T::ParentState::ID), decltype(T::DefaultChildState::ID)>
) &&
(
    std::tuple_size_v<typename T::ChildStates>() == 0 ||
    (
        IsState<std::tuple_element_t<0, typename T::ChildStates>> &&
        !std::is_same_v<typename T::ParentState, std::tuple_element_t<0, typename T::ChildStates>> &&
        !std::is_same_v<typename T::DefaultChildState, std::tuple_element_t<0, typename T::ChildStates>> &&
        std::is_same_v<decltype(T::ParentState::ID), decltype(std::tuple_element_t<0, typename T::ChildStates>::ID)> &&
        std::is_same_v<decltype(T::DefaultChildState::ID), decltype(std::tuple_element_t<0, typename T::ChildStates>::ID)>
    )
);

// For defining hierarchy substate groups of a State Machine
template<IsSubstateGroup... Groups>
using StateHierarchy = std::tuple<Groups...>;

// ------------------------------------------------------
//                     State Machine
// ------------------------------------------------------

template <
    IsTuple States,
    IsEnumClass Events,
    IsTuple Transitions,
    IsTuple Hierarchy = StateHierarchy<>,
    size_t MaxDepth = 8
>
class StateMachine
{
    inline static constexpr size_t  NUM_STATES      = std::tuple_size_v<States>;
    inline static constexpr size_t  NUM_EVENTS      = magic_enum::enum_values<Events>().size();
    inline static constexpr size_t  NUM_TRANSITIONS = std::tuple_size_v<Transitions>;
    inline static constexpr bool    IS_HIERARCHICAL = std::tuple_size_v<Hierarchy> > 0;

    static_assert(NUM_STATES > 0);
    static_assert(NUM_EVENTS > 0);
    static_assert(NUM_TRANSITIONS > 0);

    using StateId = std::tuple_element_t<0, States>::IdType;

public:
    StateMachine()
    {
        auto defaultState = std::tuple_element_t<0, States>::ID;
        m_activeStatePath.push_back(defaultState);

        // if constexpr (IS_HIERARCHICAL) {
        //     auto current = defaultState;
        //     while(true) {
        //         constexpr auto child = detail::template get_default_child<current, Hierarchy>();
        //         if constexpr (child == StateId{}) {
        //             break;
        //         } 
        //         else {
        //             m_activeStatePath.push_back(child);
        //             current = child;
        //         }
        //     }
        // }
        
        for(size_t i = 0; i < m_activeStatePath.size(); ++i) {
            callOnEntry(m_activeStatePath[i]);
        }
    }

    ~StateMachine()
    {
        for(size_t i = m_activeStatePath.size(); i > 0; --i) {
            callOnExit(m_activeStatePath[i-1]);
        }
    }

    void sendEvent(Events event)
    {
        m_eventQueue.push_back(event);
    }

    void update()
    {
        while (!m_eventQueue.empty()) {
            auto event = m_eventQueue.front();
            m_eventQueue.pop_front();
            processEvent(event);
        }

        for(size_t i = 0; i < m_activeStatePath.size(); ++i) {
            callOnActive(m_activeStatePath[i]);
        }
    }

    constexpr auto currentState() const { return m_activeStatePath[m_activeStatePath.size() - 1]; }
    const auto& currentStatePath() const { return m_activeStatePath; }

private:
    // ------------------------------------------------------
    //                 Metaprogramming Helpers
    // ------------------------------------------------------
    struct detail {
        template<typename T, typename Tuple>
        struct has_type;

        template<typename T, typename... Us>
        struct has_type<T, std::tuple<Us...>> : std::disjunction<std::is_same<T, Us>...> {};

        template<auto S, typename T>
        struct get_parent_impl;

        template<auto S, typename... Gs>
        struct get_parent_impl<S, std::tuple<Gs...>> {
            static constexpr auto get() {
                StateId parent = StateId{};
                ([&]{
                    using ChildrenTuple = typename Gs::children;
                    if (has_type<std::integral_constant<StateId, S>, ChildrenTuple>::value) {
                        parent = Gs::parent;
                    }
                }(), ...);
                return parent;
            }
            static constexpr StateId value = get();
        };

        template<auto S, typename H>
        static constexpr auto get_parent() {
            if constexpr (std::tuple_size_v<H> == 0) return StateId{};
            else return get_parent_impl<S, H>::value;
        }

        template<auto S, typename T>
        struct get_default_child_impl;

        template<auto S, typename... Gs>
        struct get_default_child_impl<S, std::tuple<Gs...>> {
            static constexpr auto get() {
                StateId child = StateId{};
                ([&]{
                    if (Gs::parent == S) {
                        child = Gs::default_child;
                    }
                }(), ...);
                return child;
            }
            static constexpr StateId value = get();
        };
        
        template<auto S, typename H>
        static constexpr auto get_default_child() {
            if constexpr (std::tuple_size_v<H> == 0) return StateId{};
            else return get_default_child_impl<S, H>::value;
        }

        template<auto S1, auto S2, typename H>
        static constexpr bool is_ancestor() {
            constexpr auto parent = get_parent<S2, H>();
            if constexpr (parent == StateId{}) {
                return false;
            } else if constexpr (parent == S1) {
                return true;
            } else {
                return is_ancestor<S1, parent, H>();
            }
        }
    };

    // ------------------------------------------------------
    //                 State Implementation Helpers
    // ------------------------------------------------------

    template<auto Id, size_t I = 0>
    auto& stateImpl() {
        if constexpr (I == NUM_STATES) {
            static_assert(I != NUM_STATES, "State ID not found");
        } else if constexpr (std::tuple_element_t<I, States>::ID == Id) {
            return std::get<I>(m_states).impl;
        } else {
            return stateImpl<Id, I + 1>();
        }
    }

    void callOnEntry(StateId state) 
    {
        magic_enum::enum_switch([this](auto s) { stateImpl<s.value>().onEntry(); }, state);
    }
    void callOnExit(StateId state) 
    {
        magic_enum::enum_switch([this](auto s) { stateImpl<s.value>().onExit(); }, state);
    }
    void callOnActive(StateId state) 
    {
        magic_enum::enum_switch([this](auto s) { stateImpl<s.value>().onActive(); }, state);
    }

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
//     bool findAndExecuteTransition(StateId state, StateEvents event) {
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
            
//             StaticVector<StateId, MaxDepth> newPath(1, Transition::to);

//             auto current = Transition::to;
//             while(true) {
//                 constexpr auto child = detail::template get_default_child<current, Hierarchy>();
//                 if constexpr (child == StateId{}) {
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
//             StateId& activeState = m_activeStatePath[0];
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

    // ------------------------------------------------------
    //                          Data
    // ------------------------------------------------------
    States m_states;
    StaticVector<StateId, MaxDepth> m_activeStatePath;
    std::deque<Events> m_eventQueue;
};
