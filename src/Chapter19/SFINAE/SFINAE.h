#pragma once

#include <iostream>
#include <type_traits>

template <typename T> struct IsDefaultConstructT {
private:
  // 此处，如果直接用 T 是不对的，因为他会在 IsDefaultConstructT<Foo>
  // 的时候就进行替换，此时的语境不是重载决议，因此会发生硬错误。
  // 因此此处使用 U 来推后替换的时间。
  template <typename U, typename = decltype(U())> char static test(void *);
  template <typename U> long static test(...);

public:
  static bool constexpr value =
      std::is_same_v<decltype(test<T>(nullptr)), char>;
};

class Foo {
public:
  Foo() = delete;
};
// void test() { bool t = IsDefaultConstructT<Foo>::value; }

template <typename...> using void_t = void;

template <typename, typename = void_t<>>
struct IsDefaultConstruct2T : std::false_type {};

template <typename T>
struct IsDefaultConstruct2T<T, void_t<decltype(T())>> : std::true_type {};

// Using Generic Lambdas for SFINAE

template <typename T> struct TypeT { using Type = T; };
template <typename T> constexpr auto type = TypeT<T>{};
template <typename T> T valueT(TypeT<T>);

// 本质上就是 lambda 表达式，看看将 args 放进去 lambda 表达式是否成立
template <typename F, typename... Args,
          typename = decltype(std::declval<F>()(std::declval<Args &&>()...))>
std::true_type isValidImpl(void *);

template <typename F, typename... Args> std::false_type isValidImpl(...);

inline constexpr auto isValid = [](auto f) {
  return [](auto &&...args) {
    return decltype(isValidImpl<decltype(f), decltype(args) &&...>(nullptr)){};
  };
};

constexpr auto IsDefaultConstruct3 =
    isValid([](auto x) -> decltype((void)decltype(valueT(x))()) {});

constexpr auto HasFirst =
    isValid([](auto x) -> decltype((void)valueT(x).fisrt) {});

constexpr auto IsDefaultConstructParamOne =
    isValid([](auto x, auto y) -> decltype(decltype(valueT(x))(valueT(y))) {});

template <typename T>
using IsDefaultConstruct = decltype(IsDefaultConstruct3(type<T>));
template <typename T> using HasFirst1 = decltype(HasFirst(type<T>));
template <typename T1, typename T2>
using IsDefaultConstructParamOne1 =
    decltype(IsDefaultConstructParamOne(type<T1>, type<T2>));

class Foo2 {
public:
  Foo2(int y){};

private:
  int fisrt;
};

void test2() {
  bool b1 = IsDefaultConstruct3(type<int>);
  bool b2 = HasFirst(type<Foo2>);
  bool b3 = IsDefaultConstructParamOne(type<Foo2>, type<Foo>);

  bool b4 = IsDefaultConstruct<int>::value;
  bool b5 = HasFirst1<Foo2>::value;
  bool b6 = IsDefaultConstructParamOne1<Foo2, Foo>::value;

  std::cout << b1 << b2 << b3 << b4 << b5 << b6 << std::endl;
}
