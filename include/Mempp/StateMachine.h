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
    virtual void onActive() = 0;
    virtual void onExit() = 0;
};

// State definition (consists of enum class value and implementation class with matching names)
template<auto StateId, IsBaseOf<IState<decltype(StateId)>> StateImpl>
struct State
{
    static_assert(magic_enum::is_scoped_enum_v<decltype(StateId)>);
    static_assert(magic_enum::enum_name(StateId) == reflect::type_name<StateImpl>());

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
magic_enum::is_scoped_enum_v<typename T::IdType> && 
std::is_base_of_v<IState<std::remove_const_t<typename T::IdType>>, decltype(T::impl)> &&
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
struct StateTransition
{
    static_assert(!std::is_same_v<From, To>);
    static_assert(magic_enum::is_scoped_enum_v<decltype(Trig)>);
    static_assert(is_optionally_invocable_v<decltype(Action), decltype(From::ID), decltype(Trig), decltype(To::ID)>);

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
    IsState<typename T::OldState> && magic_enum::is_scoped_enum_v<decltype(T::TRIG)> && IsState<typename T::NewState> &&
    is_optionally_invocable_v<decltype(T::ACTION), decltype(T::OldState::ID), decltype(T::TRIG), decltype(T::NewState::ID)> &&
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
struct SubstateGroup 
{
    static_assert(!std::is_same_v<Parent, DefaultChild>);
    static_assert(((!std::is_same_v<Parent, Children> && !std::is_same_v<DefaultChild, Children>) && ...));
    
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

        if constexpr (IS_HIERARCHICAL) {
            auto current = defaultState;
            while(true) {
                StateId child = StateId{};
                magic_enum::enum_switch([&](auto state_val) {
                    constexpr auto S = state_val.value;
                    child = detail::template get_default_child<S, Hierarchy>();
                }, current);

                if (child == StateId{}) {
                    break;
                } 
                else {
                    m_activeStatePath.push_back(child);
                    current = child;
                }
            }
        }
        
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
    //                 State Implementation Helpers
    // ------------------------------------------------------

    template<auto Id, size_t I = 0>
    constexpr auto& get_state_impl() {
        if constexpr (I == NUM_STATES) {
            static_assert(I != NUM_STATES, "State ID not found");
        } else if constexpr (std::tuple_element_t<I, States>::ID == Id) {
            return std::get<I>(m_states).impl;
        } else {
            return get_state_impl<Id, I + 1>();
        }
    }

    constexpr void callOnEntry(StateId state) 
    {
        magic_enum::enum_switch([this](auto s) { get_state_impl<s.value>().onEntry(); }, state);
    }
    constexpr void callOnExit(StateId state) 
    {
        magic_enum::enum_switch([this](auto s) { get_state_impl<s.value>().onExit(); }, state);
    }
    constexpr void callOnActive(StateId state) 
    {
        magic_enum::enum_switch([this](auto s) { get_state_impl<s.value>().onActive(); }, state);
    }

    // ------------------------------------------------------
    //                 Event & Transition Logic
    // ------------------------------------------------------
    void processEvent(Events event) {
        if constexpr (IS_HIERARCHICAL) {
            for (size_t i = m_activeStatePath.size(); i > 0; --i) {
                if (findAndExecuteTransition(m_activeStatePath[i-1], event)) {
                    return;
                }
            }
        } else {
            findAndExecuteTransition(m_activeStatePath[0], event);
        }
    }

    template<size_t I = 0>
    bool findAndExecuteTransition(StateId state, Events event) {
        if constexpr (I < NUM_TRANSITIONS) {
            using Transition = std::tuple_element_t<I, Transitions>;
            
            if (Transition::OldState::ID == state && Transition::TRIG == event) {
                executeTransition<Transition>();
                return true;
            }
            return findAndExecuteTransition<I + 1>(state, event);
        }
        return false;
    }

    template<typename Transition>
    void executeTransition() 
    {
        if constexpr (IS_HIERARCHICAL) {
            StaticVector<StateId, MaxDepth> newPath(1, Transition::NewState::ID);

            auto current = Transition::NewState::ID;
            while(true) {
                StateId child = StateId{};
                magic_enum::enum_switch([&](auto state_val) {
                    constexpr auto S = state_val.value;
                    child = detail::template get_default_child<S, Hierarchy>();
                }, current);

                if (child == StateId{}) {
                    break;
                }
                else {
                    newPath.push_back(child);
                    current = child;
                }
            }

            size_t diverge_idx = 0;
            for(size_t i = 0; i < m_activeStatePath.size() && i < newPath.size(); ++i) {
                if(m_activeStatePath[i] != newPath[i]) break;
                diverge_idx = i + 1;
            }

            // Exit states
            for(size_t i = m_activeStatePath.size(); i > diverge_idx; --i) {
                callOnExit(m_activeStatePath[i-1]);
            }

            // Action
            if constexpr (!std::is_same_v<std::decay_t<decltype(Transition::ACTION)>, std::nullopt_t>) {
                if constexpr (is_optional_v<decltype(Transition::ACTION)>) {
                    if (Transition::ACTION) (*Transition::ACTION)(Transition::OldState::ID, Transition::TRIG, Transition::NewState::ID);
                }
                else {
                    Transition::ACTION(Transition::OldState::ID, Transition::TRIG, Transition::NewState::ID);
                }
            }

            // Rebuild active path
            m_activeStatePath.resize(diverge_idx);
            for(size_t i = diverge_idx; i < newPath.size(); ++i) {
                m_activeStatePath.push_back(newPath[i]);
            }

            // Enter states
            for(size_t i = diverge_idx; i < m_activeStatePath.size(); ++i) {
                callOnEntry(m_activeStatePath[i]);
            }

        } else {
            // exit old state
            StateId& activeState = m_activeStatePath[0];
            callOnExit(activeState);

            // do transition
            if constexpr (!std::is_same_v<std::decay_t<decltype(Transition::ACTION)>, std::nullopt_t>) {
                if constexpr (is_optional_v<decltype(Transition::ACTION)>) {
                    if (Transition::ACTION) (*Transition::ACTION)(Transition::OldState::ID, Transition::TRIG, Transition::NewState::ID);
                }
                else {
                    Transition::ACTION(Transition::OldState::ID, Transition::TRIG, Transition::NewState::ID);
                }
            }

            // enter new state
            activeState = Transition::NewState::ID;
            callOnEntry(activeState);
        }
    }

    // ------------------------------------------------------
    //                 Metaprogramming Helpers
    // ------------------------------------------------------
    struct detail {
        template<auto S, typename T>
        struct get_parent_impl;

        template<auto S, typename... Gs>
        struct get_parent_impl<S, std::tuple<Gs...>> {
            static constexpr auto get() {
                StateId parent = StateId{};
                ([&]{
                    using ChildrenTuple = typename Gs::children;
                    if constexpr (has_type_v<ChildrenTuple, std::integral_constant<StateId, S>>) {
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
                    if (Gs::ParentState::ID == S) {
                        child = Gs::DefaultChildState::ID;
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
    //                          Data
    // ------------------------------------------------------
    States m_states;
    StaticVector<StateId, MaxDepth> m_activeStatePath;
    std::deque<Events> m_eventQueue;
};
