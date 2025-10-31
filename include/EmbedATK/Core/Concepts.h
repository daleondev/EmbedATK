#pragma once

//------------------------------------------------------
//                      Base of
//------------------------------------------------------

template <typename Derived, typename Base>
concept IsBaseOf = std::is_base_of_v<Base, Derived>;

template <typename Base, typename T>
struct is_base_of_all : std::false_type {};

template <typename Base, typename... DerivedTypes>
struct is_base_of_all<Base, std::tuple<DerivedTypes...>>
{
    static constexpr bool value = (std::is_base_of_v<Base, DerivedTypes> && ...);
};

template <typename Base, typename DerivedTypes>
inline constexpr bool is_base_of_all_v = is_base_of_all<Base, DerivedTypes>::value;

template <typename DerivedTypes, typename Base>
concept IsBaseOfAll = is_base_of_all_v<Base, DerivedTypes>;

namespace detail {
template <template <auto...> class BaseTemplate, auto... Args>
void is_templated_base_of(const BaseTemplate<Args...>*);
}

template <template <auto...> class BaseTemplate, typename Derived>
concept is_templated_base_of = requires(Derived* p) {
    detail::is_templated_base_of<BaseTemplate>(p);
};

template <template <auto...> class BaseTemplate, typename Derived>
inline constexpr bool is_templated_base_of_v = is_templated_base_of<BaseTemplate, Derived>;

template <typename Derived, template <auto...> class BaseTemplate>
concept IsTemplatedBaseOf = is_templated_base_of_v<BaseTemplate, Derived>;

//------------------------------------------------------
//                      Tuple
//------------------------------------------------------

template <typename NotTuple>
struct is_tuple : std::false_type {};

template <typename... TupleTypes>
struct is_tuple<std::tuple<TupleTypes...>> : std::true_type {};

template <typename Tuple>
inline constexpr bool is_tuple_v = is_tuple<std::remove_cvref_t<Tuple>>::value;

template <typename Tuple>
concept IsTuple = is_tuple_v<Tuple>;

//------------------------------------------------------
//                 Tuple has type
//------------------------------------------------------

template<typename T, typename NotTuple>
struct has_type : std::false_type {};

template<typename T, typename... TupleTypes>
struct has_type<T, std::tuple<TupleTypes...>> : std::disjunction<std::is_same<T, TupleTypes>...> {};

template <typename Tuple, typename T>
inline constexpr bool has_type_v = has_type<T, Tuple>::value;

template <typename T, typename Tuple>
concept HasType = has_type_v<Tuple, T>;

//------------------------------------------------------
//                      Enum class
//------------------------------------------------------

template <typename T>
concept IsEnumClass = magic_enum::is_scoped_enum_v<T>;

//------------------------------------------------------
//                      Optional
//------------------------------------------------------

template <typename NotOptional>
struct is_optional : std::false_type {};

template <typename OptionalType>
struct is_optional<std::optional<OptionalType>> : std::true_type {};

template <typename Optional>
inline constexpr bool is_optional_v = is_optional<std::remove_cvref_t<Optional>>::value;

template <typename Optional>
concept IsOptional = is_optional_v<Optional>;

//------------------------------------------------------
//               Optionally Invocable
//------------------------------------------------------

template <typename T>
struct optional_inner_type { using type = T; };

template <typename OptionallyInvocableType>
struct optional_inner_type<std::optional<OptionallyInvocableType>> { using type = OptionallyInvocableType; };

template <typename OptionallyInvocable>
using optional_inner_type_t = typename optional_inner_type<std::remove_cvref_t<OptionallyInvocable>>::type;

template <typename OptionallyInvocable, typename... Args>
inline constexpr bool is_optionally_invocable_v = 
    std::invocable<OptionallyInvocable, Args...> ||
    std::same_as<std::remove_cvref_t<OptionallyInvocable>, std::nullopt_t> ||
    (is_optional_v<OptionallyInvocable> && std::invocable<optional_inner_type_t<OptionallyInvocable>, Args...>);

template <typename OptionallyInvocable, typename... Args>
concept IsOptionallyInvocable = is_optionally_invocable_v<OptionallyInvocable, Args...>;

//------------------------------------------------------
//                      Unique in tuple
//------------------------------------------------------

template <typename T, typename... Pack>
inline constexpr size_t count_type_v = (static_cast<size_t>(std::is_same_v<T, Pack>) + ...);

template <typename... Types>
inline constexpr bool are_types_unique_v = ((count_type_v<Types, Types...> == 1) && ...);

template <typename T>
struct all_tuple_types_unique;

template <typename... Types>
struct all_tuple_types_unique<std::tuple<Types...>> {
    static constexpr bool value = are_types_unique_v<Types...>;
};

template <typename T>
inline constexpr bool all_tuple_types_unique_v = all_tuple_types_unique<T>::value;

//------------------------------------------------------
//                      Type in tuple
//------------------------------------------------------

template<typename T, typename Tuple>
struct is_type_in_tuple;

template<typename T, typename... Ts>
struct is_type_in_tuple<T, std::tuple<Ts...>>
    : std::bool_constant<(std::is_same_v<T, Ts> || ...)> {};

template<typename T, typename Tuple>
inline constexpr bool is_type_in_tuple_v = is_type_in_tuple<T, Tuple>::value;