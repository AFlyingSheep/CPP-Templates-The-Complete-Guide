#include "type_list.h"
#include <cstddef>
#include <iostream>

template <typename T, T Value> struct CTValue {
  static constexpr T value = Value;
};

using Primes = Typelist<CTValue<int, 2>, CTValue<int, 3>, CTValue<int, 5>,
                        CTValue<int, 7>, CTValue<int, 11>>;

template <typename T, typename U> struct MultiplyT;
template <typename T, T Value1, T Value2>
struct MultiplyT<CTValue<T, Value1>, CTValue<T, Value2>> {
public:
  using Type = CTValue<T, Value1 * Value2>;
};

template <typename T, typename U>
using Multiply = typename MultiplyT<T, U>::Type;

// New impl
template <typename T, T... Values>
using CTTypelist = Typelist<CTValue<T, Values>...>;

using Primes2 = CTTypelist<int, 2, 3, 5, 7, 11>;

// Value List
template <typename T, T... Values> struct Valuelist {};
template <typename T, T... Values> struct IsEmpty<Valuelist<T, Values...>> {
  static constexpr bool value = sizeof...(Values) == 0;
};
template <typename T, T Head, T... Tail>
struct FrontT<Valuelist<T, Head, Tail...>> {
  using Type = CTValue<T, Head>;
  static constexpr T value = Head;
};
template <typename T, T Head, T... Tail>
struct PopFrontT<Valuelist<T, Head, Tail...>> {
  using Type = Valuelist<T, Tail...>;
};
template <typename T, T... Values, T New>
class PushFrontT<Valuelist<T, Values...>, CTValue<T, New>> {
public:
  using Type = Valuelist<T, New, Values...>;
};
template <typename T, T... Values, T New>
class PushBackT<Valuelist<T, Values...>, CTValue<T, New>> {
public:
  using Type = Valuelist<T, Values..., New>;
};

template <typename T, typename U> struct GreaterThanT;
template <typename T, T First, T Second>
struct GreaterThanT<CTValue<T, First>, CTValue<T, Second>> {
  static constexpr bool value = First > Second;
};

// Deduce param
template <auto Value> struct CTValue2 { static constexpr auto value = Value; };
template <auto... Value> class Valuelist2 {};
using MyList = Valuelist2<true, 'a', &test>;

// Select element
template <typename Types, typename Indices> class SelectT;
template <typename Types, std::size_t... Indices>
class SelectT<Types, Valuelist<std::size_t, Indices...>> {
public:
  using Type = Typelist<NthElement<Types, Indices>...>;
};
template <typename Types, typename Indices>
using Select = typename SelectT<Types, Indices>::Type;

void test_non_type() {
  static_assert(
      std::is_same_v<AccumulateT<Primes, MultiplyT, CTValue<int, 1>>::Type,
                     CTValue<int, 2310>>);

  using Integers = Valuelist<int, 6, 2, 4, 9, 5, 2, 1, 7>;
  using SortedIntegers = InsertionSort<Integers, GreaterThanT>;
  static_assert(
      std::is_same_v<SortedIntegers, Valuelist<int, 9, 7, 6, 5, 4, 2, 2, 1>>,
      "insertion sort failed");

  // 测试 selectT
  using SignedIntegralTypes =
      Typelist<signed char, short, int, long, long long>;
  using ReversedSignedIntegralTypes =
      Select<SignedIntegralTypes, Valuelist<std::size_t, 4, 3, 2, 1, 0>>;
  static_assert(
      std::is_same_v<ReversedSignedIntegralTypes,
                     Typelist<long long, long, int, short, signed char>>);
}
