本章将讲解 std::tuple 的简化实现。

# Basic Tuple Design

## Storage

Tuple 可以通过递归来存取，如下所示：

```c++
template <typename... Types> class Tuple;

// 递归情况
template <typename Head, typename... Tail> class Tuple<Head, Tail...> {
private:
  Head head;
  Tuple<Tail...> tail;

public:
  Tuple() {}
  Tuple(Head const &head, Tuple<Tail...> const &tail)
      : head(head), tail(tail) {}

  Head &getHead() { return head; }
  Head const &getHead() const { return head; }
  Tuple<Tail...> &getTail() { return tail; }
  Tuple<Tail...> const &getTail() const { return tail; }
};

template <> class Tuple<> {};
```

接下来实现获取元素：

```c++
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
```

## Construction

目前除了刚刚定义的构造函数以外，需要可以从一组独立的值开始构造：

```c++
Tuple(Head const &head, Tail const &...tail) : head(head), tail(tail...) {}
```

不过用户可能希望使用移动构造初始化部分参数，因此需要完美转发：

```c++
Tuple(VHead &&head, VTail &&...tail)
    : head(std::forward<VHead>(head)), tail(std::forward<VTail>(tail)...) {}
```

接着是从一个元组构造当前元组：

```c++
  Tuple(Tuple<VHead, VTail...> const &other)
      : head(other.getHead()), tail(other.getTail()) {}
```

但是会出现问题：

```c++
  Tuple<int, double, std::string> t(1, 2.0, "3");
  Tuple<long, long double, std::string> t2(t);
```

`error: no viable conversion from 'Tuple<int, double, std::basic_string<char>>' to 'long'`，这是因为模板通过匹配，会认为从一组独立值的构造会更优于从 tuple 构造。因此需要加入 `std::enable_if`：

```c++
  template <typename VHead, typename... VTail,
            typename = std::enable_if_t<sizeof(VHead) == sizeof(head)>>
  Tuple(VHead &&head, VTail &&...tail)
      : head(std::forward<VHead>(head)), tail(std::forward<VTail>(tail)...) {}

  template <typename VHead, typename... VTail,
            typename = std::enable_if_t<sizeof...(VTail) == sizeof...(Tail)>>
  Tuple(Tuple<VHead, VTail...> const &other)
      : head(other.getHead()), tail(other.getTail()) {}
```

可以使用模板函数更容易地创建 Tuple 类型：

```c++
template <typename... Types> auto makeTuple(Types &&...elems) {
  return Tuple<std::decay_t<Types>...>(std::forward<Types>(elems)...);
}
```

# Basic Tuple Operations

## Comparision

```c++
bool operator==(Tuple<>, Tuple<>) { return true; }
template <typename Head1, typename... Tail1, typename Head2, typename... Tail2>
bool operator==(Tuple<Head1, Tail1...> t1, Tuple<Head2, Tail2...> t2) {
  return t1.getHead() == t2.getHead() && t1.getTail() == t2.getTail();
}
```

## Output

```c++
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
```

# Tuple Algorithms

Tuple 算法既包含了编译时运行的代码（如 reverse type）和运行时代码（如 reverse 元素）。

## Tuple as a Typelists

如果忽略 Tuple 的运行时部分，可以发现和 Typelists 具有相同的结构。

```c++
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

// add element to the front:
template <typename Tuple, typename Element> class PushFrontT;
template <typename... Types, typename Element>
class PushFrontT<Tuple<Types...>, Element> {
public:
  using Type = Tuple<Element, Types...>;
};

// add element to the back:
template <typename Tuple, typename Element> class PushBackT;
template <typename... Types, typename Element>
class PushBackT<Tuple<Types...>, Element> {
public:
  using Type = Tuple<Types..., Element>;
};
```

## Adding to or Removing from a tuple

push front 还是很简单的，但是 push back 需要遍历类型列表以在最后增加一个新的类型：

```c++
// push front
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
```

现在可以发现，类型是在编译时计算的，而返回的结果是在运行时计算的。

## Reserving a Tuple

```c++
// reverse
Tuple<> reverse(Tuple<> const &t) { return t; }
template <typename Head, typename... Tail>
auto reverse(Tuple<Head, Tail...> const &t) {
  return pushBack(reverse(t.getTail()), t.getHead());
}
```

## Index Lists

用上述的方法会发现，运行时效率很低下，因为每个元素都会被复制很多次。原文中使用 copy 计数器进行测试。

最理想的情况是每个元素都复制一次，会像如下的实现一样：

```c++
auto reversed = makeTuple(get<4>(copies), get<3>(copies),
                          get<2>(copies), get<1>(copies),
                          get<0>(copies));
```

Index list 通过将元素的索引捕获到 pack 里面，允许通过 pack 展开的方式使用 get 获得元素。标准类型中的 std::integer_sequence 就会实现如上目的。

## Reversal with Index Lists

如何生成索引列表？一种方法是通过模板元编程生成从 0 ~ N-1 的索引列表并使用 Reserve 完成反转（因为生成的类型是一个 Valuelist，可以进行该操作）：

```c++
// recursive case
template <unsigned N, typename Result = Valuelist<unsigned>>
struct MakeIndexListT
    : MakeIndexListT<N - 1, PushFront<Result, CTValue<unsigned, N - 1>>> {};
// basis case
template <typename Result> struct MakeIndexListT<0, Result> {
  using Type = Result;
};
template <unsigned N> using MakeIndexList = typename MakeIndexListT<N>::Type;

using MyIndexList = Reverse<MakeIndexList<5>>;
```

注：C++14 provides a similar template make_index_sequence that yields a list of indices of type std::size_t.

有了索引列表便可以完成新版的 reverse，由于使用的是 get<> 来获取元素，因此仅会复制一次：

```c++
template <typename... Elements, unsigned... Indices>
auto reverseImpl(Tuple<Elements...> const &t, Valuelist<unsigned, Indices...>) {
  return makeTuple(get<Indices>(t)...);
}
template <typename... Elements> auto reverse(Tuple<Elements...> const &t) {
  return reverseImpl(t, Reverse<MakeIndexList<sizeof...(Elements)>>());
}
```

## Shuffle and Select

上面的算法仅仅我们指定了一个反向的 Value List，然而通过 select 和 不同的索引列表，我们可以实现各种算法：

```c++
template <typename... Elements, unsigned... Indices>
auto select(Tuple<Elements...> const &t, Valuelist<unsigned, Indices...>) {
  return makeTuple(get<Indices>(t)...);
}
```

比如，我们希望实现一个 splat 算法，生成指定元素重复 N 次的 Tuple，如：

```c++
Tuple<int, double, std::string> t1(42, 7.7, "hello");
auto a = splat<1, 4>(t);
std::cout << a << ’\n’;
```

结果为：(7.7, 7.7, 7.7, 7.7)

```c++
template <unsigned I, unsigned N, typename IndexList = Valuelist<unsigned>>
class ReplicatedIndexListT;
// 递归生成 I 重复 N 次的 Valuelist
template <unsigned I, unsigned N, unsigned... Indices>
class ReplicatedIndexListT<I, N, Valuelist<unsigned, Indices...>>
    : public ReplicatedIndexListT<I, N - 1,
                                  Valuelist<unsigned, Indices..., I>> {};
template <unsigned I, unsigned... Indices>
class ReplicatedIndexListT<I, 0, Valuelist<unsigned, Indices...>> {
public:
  using Type = Valuelist<unsigned, Indices...>;
};
template <unsigned I, unsigned N>
using ReplicatedIndexList = typename ReplicatedIndexListT<I, N>::Type;

// 通过 Select 选择元素并返回 Tuple
template <unsigned I, unsigned N, typename... Elements>
auto splat(Tuple<Elements...> const &t) {
  return select(t, ReplicatedIndexList<I, N>());
}
```

即使是复杂的元组算法也可以通过索引列表上的模板元程序并随后应用 select() 来实现。如使用插入排序来根据元素类型的大小对元组进行排序。

给定这样一个 sort() 函数，它接受比较元组元素类型的模板元函数作为比较操作，我们可以使用如下代码按大小对元组元素进行排序：

```c++
template <typename T, typename U> class SmallerThanT {
public:
  static constexpr bool value = sizeof(T) < sizeof(U);
};
void testTupleSort() {
  auto t1 = makeTuple(17LL, std::complex<double>(42, 77), 'c', 42, 7.7);
  std::cout << t1 << '\n';
  auto t2 = sort<SmallerThanT>(t1); // t2 is Tuple<int, long, std::string>
  std::cout << "sorted by size: " << t2 << ’\n’;
}
```
其中，sort 的实现如下，基本思想是首先生成一个 0 ~ N-1 的 index list，接着比较函数是获取索引对应的 type size 来进行比较。

```c++
// metafunction wrapper that compares the elements in a tuple:
template <typename List, template <typename T, typename U> class F>
class MetafunOfNthElementT {
public:
  template <typename T, typename U> class Apply;
  template <unsigned N, unsigned M>
  class Apply<CTValue<unsigned, M>, CTValue<unsigned, N>>
      : public F<NthElement<List, M>, NthElement<List, N>> {};
};
// sort a tuple based on comparing the element types:
template <template <typename T, typename U> class Compare, typename... Elements>

auto sort(Tuple<Elements...> const &t) {
  return select(
      t,
      InsertionSort<
          MakeIndexList<sizeof...(Elements)>,
          MetafunOfNthElementT<Tuple<Elements...>, Compare>::template Apply>());
}
```

该实现类似于运行时的如下实现：

```c++
std::vector<std::string> strings = {"banana", "apple", "cherry"};
std::vector<unsigned> indices = {0, 1, 2};
std::sort(indices.begin(), indices.end(), [&strings](unsigned i, unsigned j) {
  return strings[i] < strings[j];
});
```

需要注意的是，我们实现的 sort 是在编译时便可以完成排序的，因为 sizeof(type) 是可以在编译器确定。

# Expanding Tuples

我们想要实现类似 ... 的效果来 unpack tuple，因此我们实现了 apply 函数完成如上操作：

```c++
template <typename F, typename... Elements, unsigned... Indices>
auto applyImpl(F f, Tuple<Elements...> const &t,
               Valuelist<unsigned, Indices...>)
    -> decltype(f(get<Indices>(t)...)) {
  return f(get<Indices>(t)...);
}
template <typename F, typename... Elements, unsigned N = sizeof...(Elements)>
auto apply(F f, Tuple<Elements...> const &t)
    -> decltype(applyImpl(f, t, MakeIndexList<N>())) {
  return applyImpl(f, t, MakeIndexList<N>());
}

// main.cc
Tuple<std::string, char const*, int, char> t("Pi", "is roughly",3, '\n');

print(t...); // Error!
apply(print, t); // OK: prints Pi is roughly 3
```

# Optimization Tuple

## Tuples and EBCO

可以发现我们的 Tuple 超出了严格要求的存储，原因是每个 Tuple 的 tail 都会以一个 empty tuple 结尾，数据成员会多出 1 字节，不利于对齐。

首先的一个解决方案是，不把 Tail 作为数据成员了，将他作为基类，如下所示：

```c++
template <typename Head, typename... Tail>
class Tuple<Head, Tail...> : private Tuple<Tail...> {
private:
  Head head;

public:
  Head &getHead() { return head; }
  Head const &getHead() const { return head; }
  Tuple<Tail...> &getTail() { return *this; }
  Tuple<Tail...> const &getTail() const { return *this; }
};
```

然而会产生副作用，原来我们是 head 先初始化，然后是 Tail，可现在是 Tail 先初始化了。

这个问题可以通过将 Head 成员下沉到它自己的基类中来解决，该基类位于基类列表中尾部之前。直接实现此方法将引入一个 TupleElt 模板，用于包装每个元素类型，以便 Tuple 可以从中继承：

```c++
template <typename... Types> class Tuple;
template <typename T> class TupleElt {
  T value;

public:
  TupleElt() = default;
  template <typename U> TupleElt(U &&other) : value(std::forward<U>(other)) {}
  T &get() { return value; }
  T const &get() const { return value; }
};
// recursive case:
template <typename Head, typename... Tail>
class Tuple<Head, Tail...> : private TupleElt<Head>, private Tuple<Tail...> {
public:
  Head &getHead() {
    // potentially ambiguous
    return static_cast<TupleElt<Head> *>(this)->get();
  }
  Head const &getHead() const {
    // potentially ambiguous
    return static_cast<TupleElt<Head> const *>(this)->get();
  }
  Tuple<Tail...> &getTail() { return *this; }
  Tuple<Tail...> const &getTail() const { return *this; }
};
// basis case:
template <> class Tuple<> {
  // no storage required
};
```

然而这种实现虽然解决了初始化的问题，但又引入了新问题：对基类继承的时候会出现歧义，如 `Tuple<int, int>` 继承的时候都是 TupleElt<Int>，导致冲突。需要保证每个继承的 TupleElt 是唯一的，因此我们加入额外的参数——剩余尾部的长度进行编码：

```c++
template <unsigned Height, typename T> class TupleElt {
  T value;

public:
  TupleElt() = default;
  template <typename U> TupleElt(U &&other) : value(std::forward<U>(other)) {}
  T &get() { return value; }
  T const &get() const { return value; }
};
```

于是我们便可以实现既可以保证初始化顺序又可以保证正确性的 EBCO 优化实现：

```c++
template <typename... Types> class Tuple;
// recursive case:
template <typename Head, typename... Tail>
class Tuple<Head, Tail...> : private TupleElt<sizeof...(Tail), Head>,
                             private Tuple<Tail...> {
  using HeadElt = TupleElt<sizeof...(Tail), Head>;

public:
  Head &getHead() { return static_cast<HeadElt *>(this)->get(); }
  Head const &getHead() const {
    return static_cast<HeadElt const *>(this)->get();
  }
  Tuple<Tail...> &getTail() { return *this; }
  Tuple<Tail...> const &getTail() const { return *this; }
};
// basis case:
template <> class Tuple<> {
  // no storage required
};
```

但是还存在一个问题：当 Type 里面是空基类的时候，我们依然还是为他分配了空间（在 TupleElt 里面），因此我们使用 enable_if 来排除这种情况：

```c++
template <unsigned Height, typename T,
          bool = std::is_class<T>::value && !std::is_final<T>::value>
class TupleElt;
template <unsigned Height, typename T> class TupleElt<Height, T, false> {
  T value;

public:
  TupleElt() = default;
  template <typename U> TupleElt(U &&other) : value(std::forward<U>(other)) {}
  T &get() { return value; }
  T const &get() const { return value; }
};
```

应用如上优化以后，对于空基类 A，`Tuple<char, A, char>` 将仅占用 2 字节。

## Constant-time get()

由于 get<> 是在线性时间内完成的，因此还是会影响编译时间的。在我们进行 EBCO 优化以后，可以写一个常数时间内完成的 get<> 操作。

关键操作是，当将基类类型的参数与派生类类型参数匹配时，模板参数推导会推导基类的模板参数。因此，如果我们计算出提取元素的高度 H，便可以依靠从 `Tuple` 特化到 `TupleElt<H, T>`（其中 T 是被推导出来的）的转换来提取该元素。

```c++
template <unsigned H, typename T> T &getHeight(TupleElt<H, T> &te) {
  return te.get();
}
template <typename... Types> class Tuple;
template <unsigned I, typename... Elements>
auto get(Tuple<Elements...> &t)
    -> decltype(getHeight<sizeof...(Elements) - I - 1>(t)) {
  return getHeight<sizeof...(Elements) - I - 1>(t);
}
```

注意，getHeight 要声明为 Tuple 的友元函数以确保所有的私有继承均可以被访问到。

# Tuple Subscript

我们想实现一个类似于 vector[] 的下标运算符。但由于 Tuple 每个元素可能是不同类型的，因此函数必须是模板函数。所以，这也要求了索引必须具有不同的类型，否则返回是一个确定的类型。

CTValue 可以完成如上操作：

```c++
template<typename T, T Index>
auto& operator[](CTValue<T, Index>) {
  return get<Index>(*this);
}
```

因此我们便可以使用下标运算符进行访问：

```c++
auto t = makeTuple(0, ’1’, 2.2f, std::string{"hello"});
auto a = t[CTValue<unsigned, 2>{}];
auto b = t[CTValue<unsigned, 3>{}];
```

为了更方便的使用 []，我们使用 _c 后缀来将字符转换为整型变量：

```c++
template<char... cs>
constexpr auto operator"" _c() {
  return CTValue<int, parseInt<sizeof...(cs)>({cs...})>{};
}
```

因此，便可以进行如下推断：

- 42_c yields CTValue<int,42>
- 0x815_c yields CTValue<int,2069>
- 0b1111’1111_c yields CTValue<int,255>
