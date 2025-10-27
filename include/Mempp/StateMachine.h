#pragma once

#include "Polymorphic.h"

#include <magic_enum/magic_enum.hpp>
#include <reflect>

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

template <typename F>
struct is_optional<std::optional<F>> : std::true_type {};

template <typename T>
inline constexpr bool is_optional_v = is_optional<std::remove_cvref_t<T>>::value;

template <typename T>
struct optional_inner_type { using type = T; };

template <typename F>
struct optional_inner_type<std::optional<F>> { using type = F; };

template <typename T>
using optional_inner_type_t = typename optional_inner_type<std::remove_cvref_t<T>>::type;

template <typename T, typename... Args>
concept OptionallyInvocable = 
    std::invocable<T, Args...> ||
    std::same_as<std::remove_cvref_t<T>, std::nullopt_t> ||
    (is_optional_v<T> && std::invocable<optional_inner_type_t<T>, Args...>);

template <typename T>
concept EnumClass = std::is_enum_v<T> && !std::is_convertible_v<T, int>;

template <typename T>
concept StatesTuple = is_base_of_all_v<IState, T>;

template <typename T>
concept StateTransitionsTuple = is_tuple_v<T>;

template<auto From, auto Trigger, auto To>
requires EnumClass<decltype(From)> && EnumClass<decltype(Trigger)> && EnumClass<decltype(To)>
struct StateTransition;

template<typename... T>
using StateTransitions = std::tuple<T...>;

template<typename... T>
using States = std::tuple<T...>;

template <EnumClass StateDefs, StatesTuple StateImpls, EnumClass StateEvents, StateTransitionsTuple Transitions, StateDefs DefaultStateDef = static_cast<StateDefs>(0), bool IsStatic = true>
class StateMachine
{
    inline static constexpr size_t NumStatesDef = magic_enum::enum_values<StateDefs>().size();
    inline static constexpr size_t NumStatesImpl = std::tuple_size_v<StateImpls>;
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

    // template<typename Transition>
    // struct TransitionChecker;

    // template<auto From, auto Trigger, auto To>
    // struct TransitionChecker<StateTransition<From, Trigger, To>>
    // {
    //     static constexpr bool valid =
    //         std::is_same_v<decltype(From), StateDefs> &&
    //         std::is_same_v<decltype(To), StateDefs> &&
    //         std::is_same_v<decltype(Trigger), StateEvents> &&
    //         magic_enum::enum_contains(From) &&
    //         magic_enum::enum_contains(To) &&
    //         magic_enum::enum_contains(Trigger);
    // };

    // template <size_t... I>
    // static constexpr bool ValidateTransitions(std::index_sequence<I...>)
    // {
    //     return (TransitionChecker<std::tuple_element_t<I, Transitions>>::valid && ...);
    // }

    // static_assert(ValidateTransitions(std::make_index_sequence<NumStatesDef>{}), "Invalid State Transition");

public:
    constexpr StateMachine()
    {
        m_prevState = m_state = DefaultStateDef;
        m_stateImpl.template construct<DefToImpl<DefaultStateDef>>();
    }
    ~StateMachine() = default;

    void update()
    {

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

    template <typename Impl, typename... Tail>
    struct DefIndex<Impl, std::tuple<Impl, Tail...>> 
    {
        static constexpr size_t value = 0;
    };

    template <typename Impl>
    constexpr StateDefs ImplToDef() 
    {
        return static_cast<StateDefs>(DefIndex<Impl, StateImpls>::value);
    }

    StateDefs m_prevState;
    StateDefs m_state;
    std::conditional_t<
        IsStatic, 
        StaticPolymorphic<IState, StateImpls>, 
        DynamicPolymorphic<IState>
    > m_stateImpl;
};
