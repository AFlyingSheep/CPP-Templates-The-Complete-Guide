# Overloading on Type Properties

看起来像使用 Traits 来完成函数重载。

## Algorithm Specialization

算法特化指的是使用模板完成一个通用的算法，并且针对某种类型进行更高效实现的偏特化。

```c++
template <typename T> void swap(T &x, T &y) {
  T tmp(x);
  x = y;
  y = tmp;
}

template <typename T> void swap(Array<T> &x, Array<T> &y) {
  swap(x.ptr, y.ptr);
  swap(x.len, y.len);
}
```

但是针对以下情况就不行，因为下面情况两种函数是一样的：

```c++
template <typename InputIterator, typename Distance>
void advanceIter(InputIterator &x, Distance n) {
  while (n > 0) { // linear time
    ++x;
    --n;
  }
}

template <typename RandomAccessIterator, typename Distance>
void advanceIter(RandomAccessIterator &x, Distance n) {
  x += n; // constant time
}
```

因此下面的章节将解决如何完成这个重载。

## Tag Dispatching

可以使用标准库的 Tag 完成分辨：

```c++
template <typename Iterator, typename Distance>
void advanceIterImpl(Iterator &iter, Distance distance,
                     std::random_access_iterator_tag) {
  iter += distance;
}

template <typename Iterator, typename Distance>
void advanceIterImpl(Iterator &iter, Distance distance,
                     std::bidirectional_iterator_tag) {
  while (distance > 0) {
    iter++;
    distance--;
  }
}

template <typename Iterator, typename Distance>
void advanceIter(Iterator &iter, Distance distance) {
  advanceIterImpl(iter, distance,
                  typename std::iterator_traits<Iterator>::iterator_category());
}

void test() {
  std::vector<int> vec(10);
  auto iter = vec.begin();
  advanceIter(iter, 2);
}
```

当算法使用的属性具有自然的层次结构以及提供这些标签值的现有特征集时，标签调度效果很好。当算法专门化依赖于临时类型属性（例如类型 T 是否具有简单的复制赋值运算符）时，它就不那么方便了。

所以需要如下的方法。

## Enabling/Disabling Function Templates 

使用 enable_if 来避免重载重名冲突：

```c++
template <typename Iterator>
constexpr bool IsRandomAccessIterator =
    IsConvertible<typename std::iterator_traits<Iterator>::iterator_category,
                  std::random_access_iterator_tag>;
template <typename Iterator, typename Distance>
std::enable_if<IsRandomAccessIterator<Iterator>> advanceIter(Iterator &x,
                                                       Distance n) {
  x += n; // constant time
}
```

可以实现一个简单的 enableIf：

```c++
template <bool, typename T = void> struct EnableIfT {};
template <typename T> struct EnableIfT<true, T> { using Type = T; };
template <bool Cond, typename T = void>
using EnableIf = typename EnableIfT<Cond, T>::Type;
```

当 Cond 为 false 的时候，EnableIf 将不存在 Type 类型，正常来说是一个 Error，但在 SFINAE 语境下没有问题。

我们同时还需要停用另一个模板，因为对于函数重载，如果两个都可以的话将会报出二义性错误（因为替换以后他们是一样的）。

```c++
template <typename Iterator, typename Distance>
EnableIf<!IsRandomAccessIterator<Iterator>> advanceIter(Iterator &x,
                                                        Distance n) {
  while (n > 0) { // linear time
    ++x;
    --n;
  }
}
```

注：为什么这里需要 disable，而之前的 SFINAE 就不需要呢？这里指的是函数重载，我们的目的是找到一个最佳的，但是可以看到对于函数的参数，他们都是一样的，此时 enable_if 仅仅做的是开启或关闭函数；然而之前的是发生在模板替换的语境中，如果 enable_if 为 false 的话就不会实例化。这两个是不一样的。

### Providing Multiple Specializations

但如果想要支持更多条件，我们便需要对每一种条件进行审视并编写 enable_if。比如对于 random access 是一种，双向访问 + no random 是一种，input access 是一种，便需要分别评估。

使用 EnableIf 进行 Algorithm Specialization 的缺点之一：每次引入算法的新变体时，都需要重新审视所有算法变体的条件，以确保所有算法变体都是互斥的。相比之下，使用标签分派引入双向迭代器变体只需要使用标签 std::bidirection_iterator_tag 添加新的 advanceIterImpl() 重载。

标签分派和 EnableIf 这两种技术在不同的上下文中都有用：一般来说，标签分派支持基于**分层标签的简单分派**，而 EnableIf 支持基于**由类型特征确定的任意属性集的更高级分派**。

### Where does the EnableIf go?

对于构造函数，没有返回值，没法使用 enable_if 作为返回值。于是在模板参数里面使用：

```c++
template <typename Iterator>
constexpr bool IsInputIterator =
    IsConvertible<typename std::iterator_traits<Iterator>::iterator_category,
                  std::input_iterator_tag>;
template <typename T> class Container {
public:
  // construct from an input iterator sequence:
  template <typename Iterator, typename = EnableIf<IsInputIterator<Iterator>>>
  Container(Iterator first, Iterator last);
  // convert to a container so long as the value types are convertible:
  template <typename U, typename = EnableIf<IsConvertible<T, U>>>
  operator Container<U>() const;
};
```

但是，一旦再添加一个重载，就会产生错误，因为确定两个模板是否等效不考虑默认参数：

```c++
template <typename T> class Foo {
public:
  template <typename U = T, typename = std::enable_if_t<std::is_array_v<U>>>
  Foo() {
    printf("Array.\n");
  }
  template <typename U = T, typename = std::enable_if_t<!std::is_array_v<U>>>
  Foo() { // Error！
    printf("Not Array.\n");
  }
};
```

解决方法可以为第二种加一个 typename = int。

### Compile-Time if

从 C++17 开始，可以使用 compile time if 来代替 enable-if。

```c++
template <typename Iterator, typename Distance>
void advanceIter(Iterator &x, Distance n) {
  if constexpr (IsRandomAccessIterator<Iterator>) {
    // implementation for random access iterators:
    x += n; // constant time
  } else if constexpr (IsBidirectionalIterator<Iterator>) {
    // implementation for bidirectional iterators:
    if (n > 0) {
      for (; n > 0; ++x, --n) { // linear time for positive n
      }
    } else {
      for (; n < 0; --x, ++n) { // linear time for negative n
      }
    }
  } else {
    // implementation for all other iterators that are at least input iterators:
    if (n < 0) {
      throw "advanceIter(): invalid iterator category for negative n";
    }
    while (n > 0) { // linear time for positive n only
      ++x;
      --n;
    }
  }
}
```

然而，compile time if 在某些条件下是不好用的,仅当通用组件中的差异可以完全在函数模板主体内表达时，才可以以这种方式使用 constexpr if 。在以下情况下，我们仍然需要 EnableIf：

- 涉及不同的接口

- 需要不同的类定义

- 某些模板参数列表不应存在有效的实例化

### Concepts

目前的实现很笨拙，于是提出 concepts 的概念：

```c++
template <typename T> class Container {
public:
  // construct from an input iterator sequence:
  template <typename Iterator>
  requires IsInputIterator<Iterator> Container(Iterator first, Iterator last);
  // construct from a random access iterator sequence:
  template <typename Iterator>
  requires IsRandomAccessIterator<Iterator> Container(Iterator first,
                                                      Iterator last);
  // convert to a container so long as the value types are convertible:
  template <typename U>
  requires IsConvertible<T, U>
  operator Container<U>() const;
};
```

requires 子句描述了模板的需求，如果不满足则不考虑在内。这是 enable_if 的一种更直接表达。

此外，requires 子句可以附加到非模板。例如，仅当类型 T 与 < 进行比较时才提供 sort() 成员函数：

```c++
template <typename T> class Container {
public:
  // ...
  requires HasLess<T>
  void sort() { ... }
};
```

注：Appendix E 提供了完整的 concept 介绍。可以参照一下。

## Class Specialization

Like overloaded function templates, it can make sense to differentiate those partial specializations based on properties of the template arguments.

首先以 Dictionary 为例：

```c++
template <typename Key, typename Value> class Dictionary {
private:
  vector<pair<Key const, Value>> data;

public:
  // subscripted access to the data:
  value &operator[](Key const &key) {
    // search for the element with this key:
    for (auto &element : data) {
      if (element.first == key) {
        return element.second;
      }
    }
    // there is no element with this key; add one
    data.push_back(pair<Key const, Value>(key, Value()));
    return data.back().second;
  }
  ...
};
```
如何 key 支持 < 运算符，那么可以使用 map 存储；进一步，如果支持 hash，则可以使用 unorder_map。

```c++
template <typename Key, typename Value, typename = void> class Dictionary {
private:
  std::vector<std::pair<Key const, Value>> data;

public:
  // subscripted access to the data:
  Value &operator[](Key const &key) {
    // search for the element with this key:
    for (auto &element : data) {
      if (element.first == key) {
        return element.second;
      }
    }
    // there is no element with this key; add one
    data.push_back(pair<Key const, Value>(key, Value()));
    return data.back().second;
  }
  // ...
  Dictionary() { printf("Default implement.\n"); }
};

template <typename Key, typename Value>
class Dictionary<Key, Value, std::enable_if_t<hasLess(type<Key>, type<Key>)>> {
private:
  std::map<Key const, Value> data;

public:
  // subscripted access to the data:
  Value &operator[](Key const &key) { return data[key]; }
  // ...
  Dictionary() { printf("Map implement.\n"); }
};
```

与重载函数模板不同，我们不需要禁用主模板上的任何条件，因为任何部分特化都优先于主模板。然而，当我们为带有哈希操作的键添加另一个实现时，我们需要确保部分特化的条件是互斥的，否则会出现模板特化 ambiguous。

```c++
template <typename Key, typename Value>
class Dictionary<Key, Value, std::enable_if_t<hasHash(type<Key>)>> {
private:
  std::unordered_map<Key, Value> data;

public:
  // subscripted access to the data:
  Value &operator[](Key const &key) { return data[key]; }
  // ...
  Dictionary() { printf("Unorder Map implement.\n"); }
};
```

### Tag Dispatching for Class Templates

Tag dispatching 也可用于在类模板部分特化之间进行选择。为了说明这一点，我们定义了一个函数对象类型 Advance<Iterator> ，类似于前面部分中使用的 advanceIter() 算法，该算法使迭代器前进一定数量的步骤。

依赖于 BestMatchInSet 提供默认的模板参数，并进行偏特化。

```c++
template <typename... T> struct FunctionOverload;

// 递归终点，提供一个最后被识别的 func
template <> struct FunctionOverload<> { static void func(...); };

// 提供一个 T func(T) 的重载，并且继承后面的其他重载
template <typename T, typename... Args>
struct FunctionOverload<T, Args...> : public FunctionOverload<Args...> {
  static T func(T);
  using FunctionOverload<Args...>::func;
};

template <typename T, typename... Types> struct BestMatchInSetT {
  using Type = decltype(FunctionOverload<Types...>::func(std::declval<T>()));
};

template <typename T, typename... Types>
using BestMatchInSet = typename BestMatchInSetT<T, Types...>::Type;

// primary template (intentionally undefined):
template <typename Iterator,
          typename Tag = BestMatchInSet<
              typename std::iterator_traits<Iterator>::iterator_category,
              std::input_iterator_tag, std::bidirectional_iterator_tag,
              std::random_access_iterator_tag>>
class Advance;
```

不过最好在 SFINAE 失败提供 no result，保证是 SFINAE-friendly。

## Instantiation-Safe Templates

EnableIf 技术的本质是仅当模板参数满足某些特定条件时才启用特定模板或部分特化。例如，advanceIter() 算法的最有效形式会检查迭代器参数的类别是否可转换为 std::random_access_iterator_tag，这意味着各种随机访问迭代器操作将可供该算法使用。

如果我们将这个概念发挥到极致，并将模板对其**模板参数执行的每个操作编码**为 EnableIf 条件的一部分，会怎么样？这样的模板的实例化永远不会失败，因为不提供所需操作的模板参数将导致推导失败（通过 EnableIf），而不是允许实例化继续进行。我们将此类模板称为“实例化安全”模板，并在此处概述此类模板的实现。

以最简单基础的 min 函数开始：

```c++
template <typename T> T const &min(T const &x, T const &y) {
  if (y < x) {
    return y;
  }
  return x;
}
```

针对是否存在 < operator 以及获取比较后的 type 类型，我们引入如下的 trait：

```c++
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
```

接下来便可以使 min 函数 instantiation-safe:

```c++
template <typename T>
std::enable_if_t<std::is_convertible_v<LessResult<T const &, T const &>, bool>,
                 T const &>
min(T const &x, T const &y) {
  if (y < x) {
    return y;
  }
  return x;
}
```

下面进行了一些测试：

```c++
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
```

针对于 X7，他说明了在 Instantiation-Safe 的实现中并不会成立，而在原来的实现中是成立的。很微妙的是，即使是显式转换，也可以在某些上下文中隐式使用，包括控制流语句的布尔条件（if、while、for 和 do）、内置 !、&& 和 || 运算符和三元运算符 ?:。在这些上下文中，该值被认为会根据上下文转换为 bool。

但是我们希望显式的也可以用（我们对其过度限制了），同时也希望不能放过不能转换的（缺乏限制），于是我们加入 traits 来限制，在微妙的情况下同样判断是否成立。

```c++
template <typename T> struct IsContextualBoolT {
  template <typename U> struct Identity {};
  template <typename U>
  static std::true_type test(Identity<decltype(std::declval<U>() ? 1 : 0)> *);
  static std::false_type test(...);

  static constexpr bool value = decltype(test<T>(nullptr))::value;
};

template <typename T>
constexpr bool IsContextualBool = IsContextualBoolT<T>::value;
```

于是 min 函数便可以写为：

```c++
template <typename T>
std::enable_if_t<IsContextualBool<LessResult<T const &, T const &>, bool>,
                 T const &>
min(T const &x, T const &y) {
  if (y < x) {
    return y;
  }
  return x;
}
```

## C++ Standard Library

- C++ 标准库许多地方也使用了算法特化。大多数标准库倾向于基于标签的调度，但是最近也有使用 enable_if 的了。

- C++ 标准库也有许多算法特化的内在使用，如 std::copy 当迭代器引用连续内存，且具有简单的赋值运算符时，直接调用 memcpy。
