#ifndef BBTAPE_UNIT_HPP
#define BBTAPE_UNIT_HPP

#include <bbtape/json.hpp>

namespace bb
{
  template< typename T >
  concept unit_type = requires(T a, T b, nlohmann::json & json, const T & value)
  {
    T{};
    { T(a) } -> std::same_as< T >;
    { a = b } -> std::same_as< T & >;
    { T(std::move(a)) } -> std::same_as< T >;
    { a = std::move(b) } -> std::same_as< T & >;
    { a < b } -> std::convertible_to< bool >;
    { a > b } -> std::convertible_to< bool >;
    { a <= b } -> std::convertible_to< bool >;
    { a >= b } -> std::convertible_to< bool >;
    { a == b } -> std::convertible_to< bool >;
    { a != b } -> std::convertible_to< bool >;
    
    { json = value } -> std::same_as< nlohmann::json & >;
    { json.get< T >() } -> std::same_as< T >;
  };

  template< unit_type T >
  using unit = std::vector< T >;
}

#endif
