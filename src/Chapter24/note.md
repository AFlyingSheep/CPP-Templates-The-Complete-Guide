对于类型元编程，中心数据结构是类型列表，顾名思义，它是一个包含类型的列表。模板元程序可以对这些类型列表进行操作，操纵它们最终生成可执行程序的一部分。在本章中，我们讨论使用 Type List 的技术。

# Anatomy of a Typelist

类型列表是表示列表的类型，可以由模板元程序操作。提供了与列表相关联的操作: 迭代列表中的元素 (类型)、添加元素或删除元素。

但是，类型列表与大多数运行时数据结构（例如 std::list）不同，因为它们不允许修改。将元素添加到现有类型列表会创建一个新的类型列表，而不修改原始类型列表。

类型列表通常实现为一个类模板特化，该特化在其模板参数中编码类型列表的内容 (即它所包
含的类型及其顺序)。类型列表的直接实现对参数包中的元素进行编码：

```c++
template<typename... Elements>
class Typelist {};
```

下面是一些常用的操作：

```c++
template <typename List> class FrontT {};
template <typename Head, typename... Tail>
class FrontT<Typelist<Head, Tail...>> {
public:
  using Type = Head;
};

template <typename List> using Front = typename FrontT<List>::Type;

template <typename List> class PopFrontT {};
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
```

# Typelist Algorithms

## Indexing

```c++
// Indexing
template <typename List, std::size_t N>
class NthElementT : public NthElementT<PopFront<List>, N - 1> {};
template <typename List> class NthElementT<List, 0> : public FrontT<List> {};

template <typename List, unsigned N>
using NthElement = typename NthElementT<List, N>::Type;
```

## Find the Best Match

```c++
// Find the largest type
template <typename List> class IsEmptyT {
public:
  constexpr static bool value = false;
};

template <> class IsEmptyT<Typelist<>> {
public:
  constexpr static bool value = true;
};

template <typename List, bool Empty = IsEmptyT<List>::value> class LargestTypeT;

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
```

但是我们不想显式的写出 typelist<>，因为在其他地方可能会用得上这个。因此使用 Isempty。

## Appending to a TypeList

使用 Front, PushFront, PopFront, and IsEmpty 来实现一个 PushBack：

```c++
template <typename List, typename NewElement,
          bool IsEmptyT = IsEmptyT<List>::value>
class PushBackT;

template <typename List, typename NewElement>
class PushBackT<List, NewElement, false> {
private:
  using Front = Front<List>;
  using RestList = PopFront<List>;
  using NewTail = typename PushBackT<RestList, NewElement>::Type;

public:
  using Type = PushFront<NewTail, Front>;
};

template <typename List, typename NewElement>
class PushBackT<List, NewElement, true> {
public:
  using Type = Typelist<NewElement>;
};

template <typename List, typename NewElement>
using PushBack = typename PushBackT<List, NewElement>::Type;
```

这种实现肯定是没有之前实现更快的，因为这个实例化的模板更多，需要（N+1）个 PushBack+PushFront 产生的 List，N 个 Front 和 N 个 RestList。

## Reversing a Typelist

```c++
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
```

## Transforming a Typelist

我们可能希望以某种方式“转换”类型列表中的所有类型，例如使用 AddConst 元函数将每种类型转换为其 const 限定变体：

```c++
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

template <typename List, template <typename T> typename MetaFun>
using Transform = typename TransformT<List, MetaFun>::Type;
```

第四部分提供了一个更加高效的 Transform。

## Accumulating Typelists

Transform 很有用，一般与 Accumulating 一起使用，将序列归约到一个 result。

$F (F (F (...F (I, T1), T2), ..., TN −1), TN )$

```c++
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
```

Accumulate 版的 largest type：

```c++
// Large Type accumulate
template <typename T, typename U> class LargerTypeT {
public:
  using Type = std::conditional_t<sizeof(T) >= sizeof(U), T, U>;
};

template <typename T, typename U>
using LargerType = typename LargerTypeT<T, U>::Type;
```

## Insertion Sort

```c++
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
      std::conditional_t<Compare<Element, Front<List>>::value, List,
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

// 插入排序的外层实现，调用 InsertSortedT 来将当前头元素插入到递归调用返回的 List 中，
// 并返回一个排好序的 List。
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
```
注意：下面的实现是低效的：

```c++
template <typename List, typename Element,
          template <typename T, typename U> class Compare>
class InsertSortedT<List, Element, Compare, false>
    : public IfThenElseT<
          Compare<Element, Front<List>>::value, PushFront<List, Element>,
          PushFront<InsertSorted<PopFront<List>, Element, Compare>,
                    Front<List>>> {};
```

两个分支都会被评估。只有为类模板实例定义类型别名不会导致 C++ 编译器实例化该实例的主体！

# Nontype Typelists

一种简单的方法：定义 CTValue 类模板，表示类型列表中特定类型的值：

```c++
template <typename T, T Value> struct CTValue {
  static constexpr T value = Value;
};
```

有了这个表示，便可以表示素数并计算乘积：

```c++
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
```

使用 Multiply，便可以结合 Accumulate 进行相乘：

```c++
  static_assert(
      std::is_same_v<AccumulateT<Primes, MultiplyT, CTValue<int, 1>>::Type,
                     CTValue<int, 2310>>);
```

但是这种写法太冗长了，特别是针对都是同一类型的值，因此我们使用如下版本：

```c++
template<typename T, T... Values>
using CTTypelist = Typelist<CTValue<T, Values>...>;
```
这种方法的唯一缺点是别名模板只是别名，因此错误消息可能最终会打印 CTValueTypes 的底层类型列表，导致它们比我们想要的更详细。为了解决这个问题，我们可以创建一个全新的类型列表类，Valuelist，它直接存储值:

```c++
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
```

通过提供 IsEmpty、FrontT、PopFrontT 和 PushFrontT，我们使 Valuelist 成为一个合适的类型列表，可以与本章中定义的算法一起使用。 PushBackT 作为一种算法专门化提供，以减少编译时此操作的成本。例如，Valuelist 可以与之前定义的 InsertionSort 算法一起使用：

```c++
template <typename T, typename U> struct GreaterThanT;
template <typename T, T First, T Second>
struct GreaterThanT<CTValue<T, First>, CTValue<T, Second>> {
  static constexpr bool value = First > Second;
};

void test_non_type() {
  using Integers = Valuelist<int, 6, 2, 4, 9, 5, 2, 1, 7>;
  using SortedIntegers = InsertionSort<Integers, GreaterThanT>;
  static_assert(
      std::is_same_v<SortedIntegers, Valuelist<int, 9, 7, 6, 5, 4, 2, 2, 1>>,
      "insertion sort failed");
}
```

## Deducible Nontype Parameters

在 C++17 中，可以通过使用单个可推导的非类型参数（拼写为 auto）来改进 CTValue：

```c++
template<auto Value>
struct CTValue { static constexpr auto value = Value; };
```

于是使用 CTValue 就变得更加简单了：

```c++
using Primes = Typelist<CTValue<2>, CTValue<3>, CTValue<5>, CTValue<7>, CTValue<11>>;
```

```c++
template <auto Value> struct CTValue2 { static constexpr auto value = Value; };
template <auto... Value> class Valuelist2 {};
using MyList = Valuelist2<true, 'a', &test>;
```

# Optimizing Algorithms with Pack Expansions Pack

Pack expansions 对于将 typelist iteration offload 到编译器是很有用的。 Transform 算法自然适合使用Pack expansions，因为它对列表中的每个元素应用相同的操作。这为类型列表的 Transform 提供了一个特化算法 (通过偏特化):

```c++
// 原来的 Transforming List
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

// 通过 Pack expansions 以后的 Transforming
template <typename... Element, template <typename T> typename MetaFun>
class TransformT<Typelist<Element...>, MetaFun, false> {
public:
  using Type = Typelist<MetaFun<Element>...>;
};
```

这样做免除了递归，使得语法更加简单。更重要的是减少了模板实例化的数量，仅仅需要一次实例化。其他算法也会间接受益于 pack expansions。

Pack expansion 也可以完成 select the elements in a given list of indices to produce a new typelist：





# Const-style Typelists

引入可变模板之前，typelists 一般是用 Lisp 的 const 单元递归数据结构表示的，如：

```c++
class Nil {};
template <typename HeadT, typename TailT = Nil> class Cons {
public:
  using Head = HeadT;
  using Tail = TailT;
};
```

我们也可以通过实现接口来应用插入排序：

```c++
class Nil {};
template <typename HeadT, typename TailT = Nil> class Cons {
public:
  using Head = HeadT;
  using Tail = TailT;
};

// FrontT
template <typename List> class FrontT {
public:
  using Type = typename List::Head;
};
template <typename List> using Front = typename FrontT<List>::Type;

// Push front
template <typename List, typename Element> class PushFrontT {
public:
  using Type = Cons<Element, List>;
};
template <typename List, typename Element>
using PushFront = typename PushFrontT<List, Element>::Type;

// Pop front
template <typename List> class PopFrontT {
public:
  using Type = typename List::Tail;
};
template <typename List> using PopFront = typename PopFrontT<List>::Type;

// IsEmpty
template <typename List> struct IsEmpty {
  static constexpr bool value = false;
};
template <> struct IsEmpty<Nil> { static constexpr bool value = true; };

void conslisttest() {
  using ConsList = Cons<int, Cons<char, Cons<short, Cons<double>>>>;
  using SortedTypes = InsertionSort<ConsList, SmallerThanT>;
  using Expected = Cons<char, Cons<short, Cons<int, Cons<double>>>>;
  std::cout << std::is_same<SortedTypes, Expected>::value << '\n';
}
```

cons 样式类型列表可以表达与本章中描述的可变参数类型列表相同的所有算法。

然而，它们有一些缺点，导致我们更喜欢可变参数版本：首先，嵌套使得长 cons 样式类型列表在源代码和编译器诊断中都很难编写和读取。其次，多种算法（包括 PushBack 和 Transform）可以专门用于可变参数类型列表，以提供更高效的实现（通过实例化数量来衡量）。最后，类型列表的可变参数模板的使用与异构容器的可变参数模板的使用非常适合，这将在第 25 章和第 26 章中讨论。

可以发现，对于通用的算法，每种类型的容器只需要实现所需的接口便可以使用。实现了容器-算法的分离。
