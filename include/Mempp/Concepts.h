#pragma once

#include <magic_enum/magic_enum.hpp>

//------------------------------------------------------
//                      Base of
//------------------------------------------------------

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

template <typename Derived, typename Base>
concept IsBaseOf = std::is_base_of_v<Base, Derived>;

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
concept IsOptionallyInvocable = 
    std::invocable<OptionallyInvocable, Args...> ||
    std::same_as<std::remove_cvref_t<OptionallyInvocable>, std::nullopt_t> ||
    (is_optional_v<OptionallyInvocable> && std::invocable<optional_inner_type_t<OptionallyInvocable>, Args...>);