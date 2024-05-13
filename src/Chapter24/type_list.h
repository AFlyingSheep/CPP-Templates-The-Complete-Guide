#pragma once
#include <cstddef>
#include <type_traits>
template <typename... Elements> class Typelist {};

template <typename List> class FrontT {
public:
  using Type = typename List::Head;
};
template <typename Head, typename... Tail>
class FrontT<Typelist<Head, Tail...>> {
public:
  using Type = Head;
};

template <typename List> using Front = typename FrontT<List>::Type;

template <typename List> class PopFrontT {
public:
  using Type = typename List::Tail;
};
template <typename Head, typename... Tail>
class PopFrontT<Typelist<Head, Tail...>> {
public:
  using Type = Typelist<Tail...>;
};

template <typename List> using PopFront = typename PopFrontT<List>::Type;

template <typename List, typename NewElement> class PushFrontT;
template <typename... Elements, typename NewElement>
class PushFrontT<Typelist<Elements...>, NewElement> {
public:
  using Type = Typelist<NewElement, Elements...>;
};
template <typename List, typename NewElement>
using PushFront = typename PushFrontT<List, NewElement>::Type;

// Indexing
template <typename List, std::size_t N>
class NthElementT : public NthElementT<PopFront<List>, N - 1> {};
template <typename List> class NthElementT<List, 0> : public FrontT<List> {};

template <typename List, unsigned N>
using NthElement = typename NthElementT<List, N>::Type;

// Find the largest type

template <typename List> class IsEmpty {
public:
  constexpr static bool value = false;
};

template <> class IsEmpty<Typelist<>> {
public:
  constexpr static bool value = true;
};

template <typename List, bool Empty = IsEmpty<List>::value> class LargestTypeT;

template <typename List> class LargestTypeT<List, false> {
public:
  using FirstType = Front<List>;
  using RestType = typename LargestTypeT<PopFront<List>>::Type;
  using Type = std::conditional_t<sizeof(FirstType) >= sizeof(RestType),
                                  FirstType, RestType>;
};

template <typename List> class LargestTypeT<List, true> {
public:
  using Type = char;
};

template <typename List> using LargestType = typename LargestTypeT<List>::Type;

// PushBack
// template <typename List, typename NewElement,
//           bool IsEmpty = IsEmpty<List>::value>
// class PushBackT;

// template <typename List, typename NewElement>
// class PushBackT<List, NewElement, false> {
// private:
//   using Front = Front<List>;
//   using RestList = PopFront<List>;
//   using NewTail = typename PushBackT<RestList, NewElement>::Type;

// public:
//   using Type = PushFront<NewTail, Front>;
// };

// template <typename List, typename NewElement>
// class PushBackT<List, NewElement, true> {
// public:
//   using Type = Typelist<NewElement>;
// };

// template <typename List, typename NewElement>
// using PushBack = typename PushBackT<List, NewElement>::Type;
template <typename List, typename NewElement> class PushBackT;
template <typename... Elements, typename NewElement>
class PushBackT<Typelist<Elements...>, NewElement> {
public:
  using Type = Typelist<Elements..., NewElement>;
};
template <typename List, typename NewElement>
using PushBack = typename PushBackT<List, NewElement>::Type;

// Reversing List
template <typename List, bool Empty = IsEmpty<List>::value> class ReverseT;
template <typename List> using Reverse = typename ReverseT<List>::Type;

template <typename List> class ReverseT<List, true> {
public:
  using Type = List;
};
template <typename List>
class ReverseT<List, false>
    : public PushBackT<Reverse<PopFront<List>>, Front<List>> {};

// Transforming List
template <typename T> struct AddConstT { using Type = T const; };
template <typename T> using AddConst = typename AddConstT<T>::Type;

template <typename List, template <typename T> typename MetaFun,
          bool Empty = IsEmpty<List>::value>
class TransformT;

template <typename List, template <typename T> typename MetaFun>
class TransformT<List, MetaFun, false>
    : public PushFrontT<typename TransformT<PopFront<List>, MetaFun>::Type,
                        MetaFun<Front<List>>> {};

template <typename List, template <typename T> typename MetaFun>
class TransformT<List, MetaFun, true> {
public:
  using Type = List;
};

// Use pack expansion
// template <typename... Element, template <typename T> typename MetaFun>
// class TransformT<Typelist<Element...>, MetaFun, false> {
// public:
//   using Type = Typelist<MetaFun<Element>...>;
// };

template <typename List, template <typename T> typename MetaFun>
using Transform = typename TransformT<List, MetaFun>::Type;

// Acumulating List
template <typename List, template <typename X, typename Y> class F, typename I,
          bool = IsEmpty<List>::value>
class AccumulateT;
// recursive case:
template <typename List, template <typename X, typename Y> class F, typename I>
class AccumulateT<List, F, I, false>
    : public AccumulateT<PopFront<List>, F, typename F<I, Front<List>>::Type> {
};
template <typename List, template <typename X, typename Y> class F, typename I>
class AccumulateT<List, F, I, true> {
public:
  using Type = I;
};

template <typename List, template <typename X, typename Y> class F, typename I>
using Accumulate = typename AccumulateT<List, F, I>::Type;

// Large Type accumulate
template <typename T, typename U> class LargerTypeT {
public:
  using Type = std::conditional_t<sizeof(T) >= sizeof(U), T, U>;
};

template <typename T, typename U>
using LargerType = typename LargerTypeT<T, U>::Type;

// Insert Sort

template <typename T> struct IdentityT { using Type = T; };

// 插入排序的核心是 InsertSortedT 元函数，
// InsertSortedT 会将元素 Element 插入到已排序的 List 中。
// 输入的 List 认为已经是排好序的
template <typename List, typename Element,
          template <typename T, typename U> class Compare,
          bool = IsEmpty<List>::value>
class InsertSortedT;

template <typename List, typename Element,
          template <typename T, typename U> class Compare>
using InsertSorted = typename InsertSortedT<List, Element, Compare>::Type;

// recursive case:
template <typename List, typename Element,
          template <typename T, typename U> class Compare>
class InsertSortedT<List, Element, Compare, false> {
  // compute the tail of the resulting list:
  using NewTail =
      std::conditional_t<Compare<Element, Front<List>>::value,
                         typename IdentityT<List>::Type,
                         InsertSorted<PopFront<List>, Element, Compare>>;
  // compute the head of the resulting list:
  using NewHead = std::conditional_t<Compare<Element, Front<List>>::value,
                                     Element, Front<List>>;

public:
  using Type = PushFront<NewTail, NewHead>;
};

// basis case:
template <typename List, typename Element,
          template <typename T, typename U> class Compare>
class InsertSortedT<List, Element, Compare, true>
    : public PushFrontT<List, Element> {};
template <typename List, typename Element,
          template <typename T, typename U> class Compare>
using InsertSorted = typename InsertSortedT<List, Element, Compare>::Type;

// 插入排序的外层实现，返回一个排好序的 List
template <typename List, template <typename T, typename U> class Compare,
          bool = IsEmpty<List>::value>
class InsertionSortT;

template <typename List, template <typename T, typename U> class Compare>
using InsertionSort = typename InsertionSortT<List, Compare>::Type;

// recursive case (insert first element into sorted list):
template <typename List, template <typename T, typename U> class Compare>
class InsertionSortT<List, Compare, false>
    : public InsertSortedT<InsertionSort<PopFront<List>, Compare>, Front<List>,
                           Compare> {};

// basis case (an empty list is sorted):
template <typename List, template <typename T, typename U> class Compare>
class InsertionSortT<List, Compare, true> {
public:
  using Type = List;
};

template <typename T, typename U> struct SmallerThanT {
  constexpr static bool value = sizeof(T) < sizeof(U);
};

void test() {
  using List = Typelist<int, char, double>;
  static_assert(std::is_same_v<Front<List>, int>);
  static_assert(std::is_same_v<PopFront<List>, Typelist<char, double>>);
  static_assert(std::is_same_v<PushFront<List, float>,
                               Typelist<float, int, char, double>>);
  static_assert(std::is_same_v<NthElement<List, 1>, char>);
  static_assert(std::is_same_v<LargestType<List>, double>);
  static_assert(std::is_same_v<PushBack<List, float>,
                               Typelist<int, char, double, float>>);
  static_assert(std::is_same_v<Reverse<List>, Typelist<double, char, int>>);
  static_assert(std::is_same_v<Transform<List, AddConst>,
                               Typelist<const int, const char, const double>>);
  static_assert(std::is_same_v<Accumulate<List, PushFrontT, Typelist<double>>,
                               Typelist<double, char, int, double>>);

  static_assert(std::is_same_v<Accumulate<List, LargerTypeT, char>, double>);
  static_assert(
      std::is_same_v<InsertionSort<Typelist<int, char, double>, SmallerThanT>,
                     Typelist<char, int, double>>);
}
