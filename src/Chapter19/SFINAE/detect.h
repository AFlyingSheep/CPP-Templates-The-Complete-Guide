#pragma once
#include <iostream>
#include <type_traits>

#define DETECT_TYPE(type_name)                                                 \
  template <typename, typename = std::void_t<>>                                \
  struct has_Type_##type_name : std::false_type {};                            \
  template <typename T>                                                        \
  struct has_Type_##type_name<                                                 \
      T, std::void_t<typename std::remove_reference_t<T>::type_name>>          \
      : std::true_type {};                                                     \
  template <typename T>                                                        \
  constexpr bool has_type_##type_name = has_Type_##type_name<T>::value;

struct det_foo {
  using size_type = int;
};

struct size_type {};
struct det_foo2 : size_type {};

DETECT_TYPE(size_type);

void test_nontype();
void test_lambda();
void test_detect() {
  det_foo f1;
  det_foo &rf1 = f1;

  std::cout
      << has_type_size_type<int> << has_type_size_type<det_foo &> << std::endl;
  std::cout << has_type_size_type<det_foo2> << std::endl;

  test_nontype();
  test_lambda();
}

// For detecting non-type member
#define DEFINE_HAS_MEMBER(Member)                                              \
  template <typename, typename = std::void_t<>>                                \
  struct HasMemberT_##Member : std::false_type {};                             \
  template <typename T>                                                        \
  struct HasMemberT_##Member<T, std::void_t<decltype(&T::Member)>>             \
      : std::true_type {} // ; intentionally skipped

DEFINE_HAS_MEMBER(execute);
DEFINE_HAS_MEMBER(y);

// check
class det_foo3 {
public:
  void execute(int x){};
  void execute(float x){};

  int y;
};

void test_nontype() {
  std::cout << "Non-type start." << std::endl;
  std::cout << "Overload funtion: " << HasMemberT_execute<det_foo3>::value
            << std::endl;
  std::cout << HasMemberT_y<det_foo3>::value << std::endl;
}

// 使用 lambda 函数完成 detect

#include "SFINAE.h"

constexpr auto hasFirst =
    isValid([](auto x) -> decltype((void)valueT(x).first) {});

constexpr auto hasSizeType =
    isValid([](auto x) -> typename decltype(valueT(x))::size_type {});

constexpr auto hasLess =
    isValid([](auto x, auto y) -> decltype(std::declval<decltype(valueT(x))>() <
                                           std::declval<decltype(valueT(y))>()) {});

void test_lambda() {
  std::cout << "Lambda Test:\n";
  std::cout << hasLess(type<int>, type<int>) << std::endl;
}