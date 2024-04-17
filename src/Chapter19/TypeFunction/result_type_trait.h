#pragma once

#include <utility>
template <typename T1, typename T2> struct PlusResultT {
  // using Type = decltype(T1() + T2());
  using Type = decltype(std::declval<T1>() + std::declval<T2>());
};

template <typename T1, typename T2>
using PlusResult = typename PlusResultT<T1, T2>::Type;

class Foo {
public:
  static void bar(...);
};