#pragma once

#include <iostream>
#include <type_traits>
#include <utility>

template <typename... Types> class Tuple;

template <typename T, T Value> struct CTValue {
  static constexpr T value = Value;
};

// 递归情况
template <typename Head, typename... Tail> class Tuple<Head, Tail...> {
private:
  Head head;
  Tuple<Tail...> tail;

public:
  Tuple() {}
  // Tuple(Head const &head, Tuple<Tail...> const &tail)
  //     : head(head), tail(tail) {}

  // 用于转换，如 std::string <-> const char (&)
  Tuple(Head const &head, Tail const &...tail) : head(head), tail(tail...) {}

  template <typename VHead, typename... VTail,
            typename = std::enable_if_t<sizeof(VHead) == sizeof(head)>>
  Tuple(VHead &&head, VTail &&...tail)
      : head(std::forward<VHead>(head)), tail(std::forward<VTail>(tail)...) {}

  template <typename VHead, typename... VTail,
            typename = std::enable_if_t<sizeof...(VTail) == sizeof...(Tail)>>
  Tuple(Tuple<VHead, VTail...> const &other)
      : head(other.getHead()), tail(other.getTail()) {}

  Head &getHead() { return head; }
  Head const &getHead() const { return head; }
  Tuple<Tail...> &getTail() { return tail; }
  Tuple<Tail...> const &getTail() const { return tail; }

  template <typename T, T Index> auto operator[](CTValue<T, Index>) {
    return get<Index>(*this);
  }
};

template <> class Tuple<> {};

// 获取元素
template <unsigned N> struct TupleGet {
  template <typename Head, typename... Tail>
  static auto apply(Tuple<Head, Tail...> const &t) {
    return TupleGet<N - 1>::apply(t.getTail());
  }
};

template <> struct TupleGet<0> {
  template <typename Head, typename... Tail>
  static auto apply(Tuple<Head, Tail...> const &t) {
    return t.getHead();
  }
};

template <unsigned N, typename... Types> auto get(Tuple<Types...> const &t) {
  return TupleGet<N>::apply(t);
}

template <typename... Types> auto makeTuple(Types &&...elems) {
  return Tuple<std::decay_t<Types>...>(std::forward<Types>(elems)...);
}

// Compare Tuple
bool operator==(Tuple<>, Tuple<>) { return true; }
template <typename Head1, typename... Tail1, typename Head2, typename... Tail2>
bool operator==(Tuple<Head1, Tail1...> t1, Tuple<Head2, Tail2...> t2) {
  return t1.getHead() == t2.getHead() && t1.getTail() == t2.getTail();
}

// Output Tuple
#include <iostream>
void printTuple(std::ostream &strm, Tuple<> const &, bool isFirst = true) {
  strm << (isFirst ? '(' : ')');
}
template <typename Head, typename... Tail>
void printTuple(std::ostream &strm, Tuple<Head, Tail...> const &t,
                bool isFirst = true) {
  strm << (isFirst ? "(" : ", ");
  strm << t.getHead();
  printTuple(strm, t.getTail(), false);
}
template <typename... Types>
std::ostream &operator<<(std::ostream &strm, Tuple<Types...> const &t) {
  printTuple(strm, t);
  return strm;
}

// Algorithm

// 1. Like typelists

template <typename T> struct IsEmpty;
template <> struct IsEmpty<Tuple<>> { static constexpr bool value = true; };

// extract front element:
template <typename... T> class FrontT;
template <typename Head, typename... Tail> class FrontT<Tuple<Head, Tail...>> {
public:
  using Type = Head;
}; // remove front element:

template <typename... T> class PopFrontT;
template <typename Head, typename... Tail>
class PopFrontT<Tuple<Head, Tail...>> {
public:
  using Type = Tuple<Tail...>;
};
template <typename Tuple> using PopFront = typename PopFrontT<Tuple>::Type;

// add element to the front:
template <typename Tuple, typename Element> class PushFrontT;
template <typename... Types, typename Element>
class PushFrontT<Tuple<Types...>, Element> {
public:
  using Type = Tuple<Element, Types...>;
};

template <typename Tuple, typename Element>
using PushFront = typename PushFrontT<Tuple, Element>::Type;

// add element to the back:
template <typename Tuple, typename Element> class PushBackT;
template <typename... Types, typename Element>
class PushBackT<Tuple<Types...>, Element> {
public:
  using Type = Tuple<Types..., Element>;
};

template <typename... Types, typename V>
PushFront<Tuple<Types...>, V> pushFront(Tuple<Types...> const &tuple,
                                        V const &value) {

  return PushFront<Tuple<Types...>, V>(value, tuple);
}

// push back
template <typename V> Tuple<V> pushBack(Tuple<> const &, V const &value) {
  return Tuple<V>(value);
}

template <typename Head, typename... Tail, typename V>
Tuple<Head, Tail..., V> pushBack(Tuple<Head, Tail...> const &tuple,
                                 V const &value) {
  return Tuple<Head, Tail..., V>(tuple.getHead(),
                                 pushBack(tuple.getTail(), value));
}

// pop front
template <typename... Types>
PopFront<Tuple<Types...>> popFront(Tuple<Types...> const &tuple) {
  return tuple.getTail();
}

// reverse
Tuple<> reverse(Tuple<> const &t) { return t; }
template <typename Head, typename... Tail>
auto reverse(Tuple<Head, Tail...> const &t) {
  return pushBack(reverse(t.getTail()), t.getHead());
}

// void test() {
//   // 测试 PopFront 的类型是否正确
//   Tuple<int, double, std::string> t(1, 2.0, "3");
//   static_assert(
//       std::is_same_v<Tuple<int, double>, PopFront<Tuple<char, int,
//       double>>>);
// }
