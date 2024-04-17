#pragma once

#include <type_traits>

class A {};
class B {};
template <typename T> class Array {};

template <typename T1, typename T2>
Array<A> operator+(Array<T1> const &, Array<T2> const &);

// overload + for concrete types:
Array<A> operator+(Array<A> const &arrayA, Array<B> const &arrayB);
void addAB(Array<A> const &arrayA, Array<B> const &arrayB) {
  auto sum = arrayA + arrayB; // ERROR?: depends on whether the compiler
                              // instantiates PlusResultT<A,B>
}

template <typename, typename, typename = std::void_t<>>
struct HasPlusT : std::false_type {};

template <typename T1, typename T2>
struct HasPlusT<T1, T2,
                std::void_t<decltype(std::declval<T1>() + std::declval<T2>())>>
    : std::true_type {};

template <typename T1, typename T2, bool = HasPlusT<T1, T2>::value>
struct PlusResultT {
  using Type = decltype(std::declval<T1>() + std::declval<T2>());
};

template <typename T1, typename T2> struct PlusResultT<T1, T2, false> {};

template <typename T1, typename T2>
using PlusResult = typename PlusResultT<T1, T2>::Type;

struct Foo1 {};
struct Foo2 {};

void execute() {
  PlusResult<int, float> res;

  addAB(Array<A>(), Array<B>());
}