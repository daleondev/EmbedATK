#pragma once

#include "State.h"
#include "StateTransition.h"
#include "SubstateGroup.h"

#include "EmbedATK/Memory/Container.h"

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

template<typename TransitionsTuple, typename StatesTuple, typename EventsEnum>
inline constexpr bool transitions_valid_v = detail::validate_transitions_impl<StatesTuple, EventsEnum>(TransitionsTuple{});

template<typename SubstateGroupsTuple, typename StatesTuple>
inline constexpr bool hierarchy_valid_v = detail::validate_hierarchy_impl<StatesTuple>(SubstateGroupsTuple{});

template <
    IsStatesTuple States,
    IsEnumClass Events,
    IsStateTransitionsTuple Transitions,
    IsSubstateGroupsTuple Hierarchy = StateHierarchy<>,
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

    static_assert(transitions_valid_v<Transitions, States, Events>, "One or more 'StateTransition' definitions are invalid");
    static_assert(hierarchy_valid_v<Hierarchy, States>, "One or more 'SubstateGroup' definitions are invalid");

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

        StaticVector<StateId, MaxDepth-1> subStates{};
        for (auto state : m_activeStatePath | std::views::reverse) {
            callOnActive(state, subStates.data(), subStates.size());
            subStates.push_back(state);
        }
    }

    constexpr auto currentState() const { return m_activeStatePath[m_activeStatePath.size() - 1]; }
    const auto& currentStatePath() const { return m_activeStatePath; }

private:
    // ------------------------------------------------------
    //                 Event processing
    // ------------------------------------------------------
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
            for (size_t i = 0; i < minPathSize; ++i) {
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
    //                      State access
    // ------------------------------------------------------
    template<auto Id, size_t I = 0>
    constexpr auto& stateImpl() {
        if constexpr (I == NUM_STATES) {
            static_assert(I != NUM_STATES, "State ID not found");
        } else if constexpr (std::tuple_element_t<I, States>::ID == Id) {
            return std::get<I>(m_states);
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

    constexpr void callOnActive(StateId state, StateId* subStates, size_t numSubStates) 
    {
        magic_enum::enum_switch([this, subStates, numSubStates](auto s) { stateImpl<s.value>().onActive(subStates, numSubStates); }, state);
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