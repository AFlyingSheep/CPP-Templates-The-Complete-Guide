#pragma once
#include "type_list.h"
#include <iostream>

class Nil {};
template <typename HeadT, typename TailT = Nil> class Cons {
public:
  using Head = HeadT;
  using Tail = TailT;
};

// FrontT
// template <typename List> class FrontT {
// public:
//   using Type = typename List::Head;
// };
// template <typename List> using Front = typename FrontT<List>::Type;

// Push front
template <typename List, typename Element> class PushFrontT {
public:
  using Type = Cons<Element, List>;
};
template <typename List, typename Element>
using PushFront = typename PushFrontT<List, Element>::Type;

// Pop front
// template <typename List> class PopFrontT {
// public:
//   using Type = typename List::Tail;
// };
// template <typename List> using PopFront = typename PopFrontT<List>::Type;

// IsEmpty
// template <typename List> struct IsEmpty {
//   static constexpr bool value = false;
// };
template <> struct IsEmpty<Nil> { static constexpr bool value = true; };

void conslisttest() {
  using ConsList = Cons<int, Cons<char, Cons<short, Cons<double>>>>;
  using SortedTypes = InsertionSort<ConsList, SmallerThanT>;
  using Expected = Cons<char, Cons<short, Cons<int, Cons<double>>>>;
  std::cout << std::is_same<SortedTypes, Expected>::value << '\n';
}
