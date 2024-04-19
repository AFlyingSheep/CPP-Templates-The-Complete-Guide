#pragma once

#include <type_traits>

// 判断是否存在 operator <
template <typename T1, typename T2> struct HasLess {
  template <typename T> struct Identity;
  template <typename U1, typename U2>
  static std::true_type
  test(Identity<decltype(std::declval<U1>() < std::declval<U2>())> *);
  template <typename U1, typename U2> static std::false_type test(...);

  static constexpr bool value = decltype(test<T1, T2>(nullptr))::value;
};

template <typename T1, typename T2, bool = false> struct LessResultT {};
template <typename T1, typename T2> struct LessResultT<T1, T2, true> {
  using Type = decltype(std::declval<T1>() < std::declval<T2>());
};

template <typename T1, typename T2>
using LessResult = typename LessResultT<T1, T2, HasLess<T1, T2>::value>::Type;

template <typename T>
std::enable_if_t<std::is_convertible_v<LessResult<T const &, T const &>, bool>,
                 T const &>
min(T const &x, T const &y) {
  if (y < x) {
    return y;
  }
  return x;
}

struct X1 {};
bool operator<(X1 const &, X1 const &) { return true; }
struct X2 {};
bool operator<(X2, X2) { return true; }
struct X3 {};
bool operator<(X3 &, X3 &) { return true; }
struct X4 {};
struct BoolConvertible {
  operator bool() const { return true; } // implicit conversion to bool
};
struct X5 {};
BoolConvertible operator<(X5 const &, X5 const &) { return BoolConvertible(); }
struct NotBoolConvertible { // no conversion to bool
};
struct X6 {};
NotBoolConvertible operator<(X6 const &, X6 const &) {
  return NotBoolConvertible();
}
struct BoolLike {
  explicit operator bool() const { return true; } // explicit conversion to bool
};
struct X7 {};
BoolLike operator<(X7 const &, X7 const &) { return BoolLike(); }
int test_less() {
  min(X1(), X1()); // X1 can be passed to min()
  min(X2(), X2()); // X2 can be passed to min()
  // min(X3(), X3()); // ERROR: X3 并没有提供常引用的比较重载
  // min(X4(), X4()); // ERROR: X4 没有提供 < operator
  min(X5(), X5()); // X5 比较的返回值可以转换为 bool
  // min(X6(), X6()); // ERROR: X6 比较的返回值可以不可以转换为 bool
  // min(X7(), X7()); // UNEXPECTED ERROR: X7 返回值必须显式转换为 bool
}

template <typename T> struct IsContextualBoolT {
  template <typename U> struct Identity {};
  template <typename U>
  static std::true_type test(Identity<decltype(std::declval<U>() ? 1 : 0)> *);
  static std::false_type test(...);

  static constexpr bool value = decltype(test<T>(nullptr))::value;
};

template <typename T>
constexpr bool IsContextualBool = IsContextualBoolT<T>::value;
