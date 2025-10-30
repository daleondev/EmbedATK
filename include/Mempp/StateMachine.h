#pragma once

#include "Concepts.h"
#include "Container.h"

#include <reflect>
#include <optional>
#include <deque>
#include <type_traits>
#include <magic_enum/magic_enum_switch.hpp>
#include <ranges>
#include <algorithm>


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
template<IsState From, auto Trig, IsState To, auto Callback = std::nullopt>
struct StateTransition
{
    static_assert(!std::is_same_v<From, To>);
    static_assert(magic_enum::is_scoped_enum_v<decltype(Trig)>);
    static_assert(is_optionally_invocable_v<decltype(Callback), decltype(From::ID), decltype(Trig), decltype(To::ID)>);

    using OldState                  = From;
    using NewState                  = To;
    static constexpr auto TRIG      = Trig;
    static constexpr auto CALLBACK  = Callback;
};

// concept to force a valid State transition
template<typename T>
concept IsStateTransition = requires {
    typename T;
    typename T::OldState;
    typename T::NewState;
    { T::TRIG };
    { T::CALLBACK };
} && 
(
    IsState<typename T::OldState> && magic_enum::is_scoped_enum_v<decltype(T::TRIG)> && IsState<typename T::NewState> &&
    is_optionally_invocable_v<decltype(T::CALLBACK), decltype(T::OldState::ID), decltype(T::TRIG), decltype(T::NewState::ID)> &&
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
    requires (IsState<Children> && ...)
struct SubstateGroup 
{
    static_assert(!std::is_same_v<Parent, DefaultChild>);
    static_assert(((!std::is_same_v<Parent, Children> && !std::is_same_v<DefaultChild, Children>) && ...));
    static_assert((std::is_same_v<decltype(Parent::ID), decltype(Children::ID)> && ...));
    
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
        auto defaultStateId = std::tuple_element_t<0, States>::ID;
        
        if constexpr (IS_HIERARCHICAL) {
            m_activeStatePath = getPathToRoot(defaultStateId);
        } else {
            m_activeStatePath.push_back(defaultStateId);
        }
        
        for(size_t i = 0; i < m_activeStatePath.size(); ++i) {
            callOnEntry(m_activeStatePath[i]);
        }

        // Enter default child states until a leaf is reached
        enterDefaultChildren();
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
    void processEvent(Events event)
    {
        if constexpr (NUM_TRANSITIONS > 0)
        {
            std::optional<StateId> handlerState = m_activeStatePath[m_activeStatePath.size() - 1];
            while(handlerState)
            {
                bool transitionFound = false;
                findAndProcessTransition(*handlerState, event, transitionFound);
                if(transitionFound) {
                    return;
                }
                if constexpr (IS_HIERARCHICAL) {
                    handlerState = findParent(*handlerState);
                } else {
                    handlerState = std::nullopt;
                }
            }
        }
    }

    template<size_t I = 0>
    void findAndProcessTransition(StateId handlerState, Events event, bool& transitionFound)
    {
        if constexpr (I < NUM_TRANSITIONS)
        {
            using Transition = std::tuple_element_t<I, Transitions>;
            if (Transition::OldState::ID == handlerState && Transition::TRIG == event)
            {
                transitionFound = true;

                if constexpr (!std::is_same_v<std::decay_t<decltype(Transition::CALLBACK)>, std::nullopt_t>) {
                    if constexpr (is_optional_v<decltype(Transition::CALLBACK)>) {
                        if (Transition::CALLBACK) (*Transition::CALLBACK)(Transition::OldState::ID, Transition::TRIG, Transition::NewState::ID);
                    }
                    else {
                        Transition::CALLBACK(Transition::OldState::ID, Transition::TRIG, Transition::NewState::ID);
                    }
                }

                changeState<Transition>();
                return;
            }
            
            findAndProcessTransition<I + 1>(handlerState, event, transitionFound);
        }
    }

    template<IsStateTransition T>
    void changeState()
    {
        constexpr auto toStateId = T::NewState::ID;

        auto toPath = getPathToRoot(toStateId);

        std::optional<size_t> lcaIndex;
        if constexpr (IS_HIERARCHICAL) {
            size_t minPathSize = std::min(m_activeStatePath.size(), toPath.size());
            for(size_t i = 0; i < minPathSize; ++i) {
                if (m_activeStatePath[i] == toPath[i]) {
                    lcaIndex = i;
                } else {
                    break;
                }
            }
        }

        const size_t exitUntil = lcaIndex.has_value() ? lcaIndex.value() + 1 : 0;
        for(size_t i = m_activeStatePath.size(); i > exitUntil; --i) {
            callOnExit(m_activeStatePath[i-1]);
        }

        StaticVector<StateId, MaxDepth> newPath;
        const size_t pathCopyUntil = lcaIndex.has_value() ? lcaIndex.value() + 1 : 0;
        for(size_t i = 0; i < pathCopyUntil; ++i) {
            newPath.push_back(m_activeStatePath[i]);
        }

        const size_t entryFrom = lcaIndex.has_value() ? lcaIndex.value() + 1 : 0;
        for(size_t i = entryFrom; i < toPath.size(); ++i) {
            newPath.push_back(toPath[i]);
            callOnEntry(toPath[i]);
        }
        
        m_activeStatePath = newPath;

        enterDefaultChildren();
    }

    // ------------------------------------------------------
    //                 State Implementation Helpers
    // ------------------------------------------------------

    template<auto Id, size_t I = 0>
    constexpr auto& stateImpl() {
        if constexpr (I == NUM_STATES) {
            static_assert(I != NUM_STATES, "State ID not found");
        } else if constexpr (std::tuple_element_t<I, States>::ID == Id) {
            return std::get<I>(m_states).impl;
        } else {
            return stateImpl<Id, I + 1>();
        }
    }

    constexpr void callOnEntry(StateId state) 
    {
        magic_enum::enum_switch([this](auto s) { stateImpl<s.value>().onEntry(); }, state);
    }
    constexpr void callOnExit(StateId state) 
    {
        magic_enum::enum_switch([this](auto s) { stateImpl<s.value>().onExit(); }, state);
    }
    constexpr void callOnActive(StateId state) 
    {
        magic_enum::enum_switch([this](auto s) { stateImpl<s.value>().onActive(); }, state);
    }

    // ------------------------------------------------------
    //                  Hierarchy Helpers
    // ------------------------------------------------------

    void enterDefaultChildren()
    {
        if constexpr (IS_HIERARCHICAL) {
            auto lastState = m_activeStatePath[m_activeStatePath.size() - 1];
            auto defaultChild = findDefaultChild(lastState);
            while(defaultChild) {
                m_activeStatePath.push_back(*defaultChild);
                callOnEntry(*defaultChild);
                lastState = *defaultChild;
                defaultChild = findDefaultChild(lastState);
            }
        }
    }

    template<size_t G = 0, size_t C = 0>
    constexpr auto findParent(StateId childId) -> std::optional<StateId> {
        if constexpr (!IS_HIERARCHICAL) {
            return std::nullopt;
        } else if constexpr (G == std::tuple_size_v<Hierarchy>) {
            return std::nullopt;
        } else {
            using Group = std::tuple_element_t<G, Hierarchy>;
            using Children = typename Group::ChildStates;
            if constexpr (C < std::tuple_size_v<Children>) {
                if (std::tuple_element_t<C, Children>::ID == childId) {
                    return Group::ParentState::ID;
                }
                return findParent<G, C + 1>(childId);
            } else {
                return findParent<G + 1, 0>(childId);
            }
        }
    }

    template<size_t I = 0>
    constexpr auto findDefaultChild(StateId parentId) -> std::optional<StateId> {
        if constexpr (!IS_HIERARCHICAL) {
            return std::nullopt;
        } else if constexpr (I == std::tuple_size_v<Hierarchy>) {
            return std::nullopt;
        } else {
            using Group = std::tuple_element_t<I, Hierarchy>;
            if (Group::ParentState::ID == parentId) {
                return Group::DefaultChildState::ID;
            } else {
                return findDefaultChild<I + 1>(parentId);
            }
        }
    }

    constexpr auto getPathToRoot(StateId stateId) {
        StaticVector<StateId, MaxDepth> path;
        if constexpr (IS_HIERARCHICAL) {
            path.push_back(stateId);
            auto parent = findParent(stateId);
            while(parent) {
                path.push_back(*parent);
                parent = findParent(*parent);
            }
            std::reverse(path.begin(), path.end());
        } else {
            path.push_back(stateId);
        }
        return path;
    }

    // ------------------------------------------------------
    //                          Data
    // ------------------------------------------------------
    States m_states;
    StaticVector<StateId, MaxDepth> m_activeStatePath;
    std::deque<Events> m_eventQueue;
};