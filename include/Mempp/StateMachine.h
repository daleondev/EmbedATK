#pragma once

#include "Polymorphic.h"

#include <magic_enum/magic_enum.hpp>
#include <reflect>
#include <optional>
#include <deque>
#include <mutex>

class IState
{
public:
    virtual ~IState() = default;
    
    virtual void onEntry() = 0;
    virtual void onActive() = 0;
    virtual void onExit() = 0;
};

template <typename T>
struct is_tuple : std::false_type {};

template <typename... T>
struct is_tuple<std::tuple<T...>> : std::true_type {};

template <typename T>
inline constexpr bool is_tuple_v = is_tuple<std::remove_cvref_t<T>>::value;

template <typename T>
struct is_optional : std::false_type {};

template <typename T>
struct is_optional<std::optional<T>> : std::true_type {};

template <typename T>
inline constexpr bool is_optional_v = is_optional<std::remove_cvref_t<T>>::value;

template <typename T>
struct optional_inner_type { using type = T; };

template <typename T>
struct optional_inner_type<std::optional<T>> { using type = T; };

template <typename T>
using optional_inner_type_t = typename optional_inner_type<std::remove_cvref_t<T>>::type;

template <typename T, typename... Args>
concept OptionallyInvocable = 
    std::invocable<T, Args...> ||
    std::same_as<std::remove_cvref_t<T>, std::nullopt_t> ||
    (is_optional_v<T> && std::invocable<optional_inner_type_t<T>, Args...>);

template <typename T>
concept EnumClass = magic_enum::is_scoped_enum_v<T>;

template <typename T>
concept StatesTuple = is_base_of_all_v<IState, T>;

template <typename T>
concept StateTransitionsTuple = is_tuple_v<T>;

template<auto From, auto Trig, auto To, auto Action = std::nullopt>
requires EnumClass<decltype(From)> && EnumClass<decltype(Trig)> && EnumClass<decltype(To)> && OptionallyInvocable<decltype(Action)>
struct StateTransition
{
    static constexpr auto from      = From;
    static constexpr auto trig      = Trig;
    static constexpr auto to        = To;
    static constexpr auto action    = Action;
};

template<typename... T>
using StateTransitions = std::tuple<T...>;

template<typename... T>
using States = std::tuple<T...>;

template <EnumClass StateDefs, StatesTuple StateImpls, EnumClass StateEvents, StateTransitionsTuple Transitions, StateDefs DefaultStateDef = static_cast<StateDefs>(0), bool IsStatic = true>
class StateMachine
{
    inline static constexpr size_t NumStatesDef = magic_enum::enum_values<StateDefs>().size();
    inline static constexpr size_t NumStatesImpl = std::tuple_size_v<StateImpls>;

public:
    StateMachine()
    {
        m_prevState = m_state = DefaultStateDef;
        m_stateImpl.template construct<DefToImpl<DefaultStateDef>>();
        m_stateImpl.get()->onEntry();
    }
    ~StateMachine()
    {
        if (m_stateImpl) {
            m_stateImpl.get()->onExit();
        }
    }

    void sendEvent(StateEvents event)
    {
        m_eventQueue.push_back(event);
    }

    void update()
    {
        while (!m_eventQueue.empty()) {
            auto event = m_eventQueue.front();
            m_eventQueue.pop_front();
            ProcessEvent(event);
        }

        m_stateImpl.get()->onActive();
    }

    constexpr auto prevState() const { return m_prevState; }
    constexpr auto currentState() const { return m_state; }

private:
    template <StateDefs Def>
    using DefToImpl = std::tuple_element_t<
        static_cast<std::size_t>(Def), 
        StateImpls
    >;

    template <typename Impl, typename... Tail>
    struct DefIndex 
    {
        static constexpr size_t value = 0;
    };

    template <typename Impl, typename Head, typename... Tail>
    struct DefIndex<Impl, std::tuple<Head, Tail...>> 
    {
        static constexpr size_t value = 1 + DefIndex<Impl, States<Tail...>>::value;
    };

    template <typename Impl>
    constexpr StateDefs ImplToDef() 
    {
        return static_cast<StateDefs>(DefIndex<Impl, StateImpls>::value);
    }

    template<size_t I = 0>
    void ProcessEvent(StateEvents event) {
        if constexpr (I < std::tuple_size_v<Transitions>) {
            using Transition = std::tuple_element_t<I, Transitions>;   
            
            if (m_state == Transition::from && event == Transition::trig) {
                m_stateImpl.get()->onExit();
                m_stateImpl.destroy();

                if constexpr (!std::is_same_v<std::decay_t<decltype(Transition::action)>, std::nullopt_t>) {
                    if constexpr (is_optional_v<decltype(Transition::action)>) {
                        if (Transition::action) {
                            (*Transition::action)();
                        }
                    } else {
                        Transition::action();
                    }
                }
                
                m_prevState = m_state;
                m_state = Transition::to;

                m_stateImpl.template construct<DefToImpl<Transition::to>>();
                m_stateImpl.get()->onEntry();
            } else {
                ProcessEvent<I + 1>(event); // compile time loop
            }
        }
    }

    StateDefs m_prevState;
    StateDefs m_state;
    std::conditional_t<
        IsStatic, 
        StaticPolymorphic<IState, StateImpls>, 
        DynamicPolymorphic<IState>
    > m_stateImpl;
    
    std::deque<StateEvents> m_eventQueue;

    //------------------------------------------------------
    //                      Static Checks
    //------------------------------------------------------
    static_assert(NumStatesDef > 0 && NumStatesDef == NumStatesImpl, "Invalid number of State Definitions and Implementations");

    template <size_t I>
    static constexpr bool CheckNameMatch() 
    {
        using Impl = std::tuple_element_t<I, StateImpls>;
        constexpr std::string_view implName = reflect::type_name<Impl>();

        constexpr StateDefs Def = static_cast<StateDefs>(I);
        constexpr std::string_view defName = magic_enum::enum_name(Def);

        return implName == defName;
    }

    template <size_t... I>
    static constexpr bool ValidateNames(std::index_sequence<I...>) 
    {
        return (CheckNameMatch<I>() && ...);
    }

    static_assert(ValidateNames(std::make_index_sequence<NumStatesDef>{}), "Invalid Names of State Definitions and Implementations");

    template<typename Transition>
    struct TransitionChecker;

    template<auto From, auto Trigger, auto To, auto Action>
    struct TransitionChecker<StateTransition<From, Trigger, To, Action>>
    {
        static constexpr bool valid =
            std::is_same_v<decltype(From), StateDefs> &&
            std::is_same_v<decltype(To), StateDefs> &&
            std::is_same_v<decltype(Trigger), StateEvents> &&
            magic_enum::enum_contains<StateDefs>(From) &&
            magic_enum::enum_contains<StateDefs>(To) &&
            magic_enum::enum_contains<StateEvents>(Trigger);
    };

    template <size_t... I>
    static constexpr bool ValidateTransitions(std::index_sequence<I...>)
    {
        if constexpr (sizeof...(I) > 0) {
            return (TransitionChecker<std::tuple_element_t<I, Transitions>>::valid && ...);
        }
        return true;
    }

    static_assert(ValidateTransitions(std::make_index_sequence<NumStatesDef>{}), "Invalid State Transition");
};
