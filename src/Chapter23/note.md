Metaprogramming often relies on the concepts of traits and type functions.

# The State of Modern C++ Metaprogramming

##  Value Metaprogramming

自从 C++14 引入 constexpr 以后，value metaprogramming 就变得简单了。

```c++
template<typename T>
template <typename T> constexpr T sqrt(T x) {
  // handle cases where x and its square root are equal as a special case to
  // simplify the iteration criterion for larger x:
  if (x <= 1) {
    return x;
  }
  // repeatedly determine in which half of a [lo, hi] interval the square root
  // of x is located, until the interval is reduced to just one value:
  T lo = 0, hi = x;
  for (;;) {
    auto mid = (hi + lo) / 2, midSquared = mid * mid;
    if (lo + 1 >= hi || midSquared == x) {
      // mid must be the square root:
      return mid;
    }
    // continue with the higher/lower half-interval:
    if (midSquared < x) {
      lo = mid;
    } else {
      hi = mid;
    }
  }
}
```

## Type Metaprogramming

采用类型作为输入并产生新的类型，比如如下的例子，递归调用本身并对偏特化匹配。

```c++
// Type Metaprogramming
template <typename T> struct RemoveAllExtentsT { using Type = T; };

template <typename T, std::size_t SZ> struct RemoveAllExtentsT<T[SZ]> {
  using Type = typename RemoveAllExtentsT<T>::Type;
};

template <typename T> struct RemoveAllExtentsT<T[]> {
  using Type = typename RemoveAllExtentsT<T>::Type;
};

template <typename T>
using RemoveAllextents = typename RemoveAllExtentsT<T>::Type;
```
添加类型容器可以大大增加编程的适用性，Chapter 24 开发了一个 TypeList<> 模板用于类型容器。

## Hybrid Metaprogramming

可以在编译期完成和运行时一样的效果。

举例：计算两个 std::array 中元素的点积。

```c++
// Hybrid Metaprogramming
#include <array>

template <typename T, std::size_t N>
auto dotProduct(std::array<T, N> const &x, std::array<T, N> const &y) {
  T result{};
  for (std::size_t k = 0; k < N; ++k) {
    result += x[k] * y[k];
  }
  return result;
}

// 但是运行时会有开销，编译器可以做到循环展开。
// 我们使用模板来避免循环：

template <typename T, std::size_t N> struct DotProduct {
  static inline T result(T *a, T *b) {
    return *a * *b + DotProduct<T, N - 1>::result(a + 1, b + 1);
  }
};

template <typename T> struct DotProduct<T, 0> {
  static inline T result(T *a, T *b) { return a * b; }
};

template <typename T, std::size_t N>
auto dotProduct_2(std::array<T, N> const &x, std::array<T, N> const &y) {
  return DotProduct<T, N>::result(x.begin(), y.begin());
}
```

这段代码的主要特点是，**混合了确定代码整体结构的编译时计算 (通过递归模板实例化实现) 和确定特定运行时效果的运行时计算 (调用 result())**。

对于异构类型，由于 std::tuple 和 std::variant 与结构类型一样，都是异构类型，因此使用此类类型的 hybrid Metaprogramming 有时称为 heterogeneous metaprogramming。

## Heterogeneous Metaprogramming for Unit Types

异构计算的另一个强大之处——能够计算不同单元类型的库，结果在运行时计算，结果元计算在编译时确定。

我们将根据主要单位的比率（分数）来跟踪单位。例如，如果时间的主要单位是秒，则用比率 1/1000 表示毫秒，用比率 60/1 表示分钟。那么关键是定义一个比率类型，其中每个值都有自己的类型：

首先定义分数：

```c++
template <unsigned N, unsigned D = 1> struct Ratio {
  static constexpr unsigned num = N; // numerator
  static constexpr unsigned den = D; // denominator
  using Type = Ratio<num, den>;
};
```

接着可以定义编译时计算 ratio 之和，返回 ratio：

```c++
template <typename R1, typename R2> struct RatioAddImpl {
private:
  static constexpr unsigned den = R1::den * R2::den;
  static constexpr unsigned num = R1::num * R2::den + R2::num * R1::den;

public:
  typedef Ratio<num, den> Type;
};
// using declaration for convenient usage:
template <typename R1, typename R2>
using RatioAdd = typename RatioAddImpl<R1, R2>::Type;
```

接着就可以实现不同单位之间相加：

```c++
// duration type for values of type T with unit type U:
template <typename T, typename U = Ratio<1>> class Duration {
public:
  using ValueType = T;
  using UnitType = typename U::Type;

private:
  ValueType val;

public:
  constexpr Duration(ValueType v = 0) : val(v) {}
  constexpr ValueType value() const { return val; }
};

template <typename T1, typename U1, typename T2, typename U2>
auto constexpr operator+(Duration<T1, U1> const &lhs,
                         Duration<T2, U2> const &rhs) {
  using VT = Ratio<1, RatioAdd<U1, U2>::den>;
  auto val = lhs.value() * VT::den / U1::den * U1::num +
             rhs.value() * VT::den / U2::den * U2::num;
  return Duration<decltype(val), VT>(val);
}
```

编译器在编译时确定结果单位类型 Ratio<1,3000> 并生成代码以在运行时计算结果值，该值根据结果单位类型进行调整。

因为值类型是模板参数，所以我们可以将 Duration 类与 int 以外的值类型一起使用，甚至可以使用异构值类型

如果值是在编译器已知的话，甚至可以完成编译器计算，因为是 constexpr。

# The Dimensions of Reflective Metaprogramming (反射元)

上述描述了 value metaprogramming（基于 constexpr）和 type metaprogramming（基于递归模板实例化），两种在当前 C++ 中均有使用。

在 C++ 不支持 constexpr 函数之前，value metaprogramming 也是靠递归模板实例化来驱动的。比如求平方根：

```c++
// recursion value metaprogramming without constexpr for function
template <std::size_t N, std::size_t L = 1, std::size_t R = N> struct Sqrt {
  static constexpr std::size_t mid = (L + R + 1) / 2;

  static constexpr std::size_t value =
      (mid * mid > N) ? Sqrt<N, L, mid - 1>::value : Sqrt<N, mid, R>::value;
};

template <std::size_t N, std::size_t M> struct Sqrt<N, M, M> {
  static constexpr std::size_t value = M;
};
```

这和 constexpr with function 实现思路相同，However, the input to the metafunction is a nontype template argument instead of a function argument, and the “local variables” tracking the bounds to the interval are also recast as nontype template arguments. Clearly, this is a far less friendly approach than the constexpr function.

我们要从三个维度进行分析：Computation, Reflection and Generation.

Reflection 是以编程方式检查程序功能的能力。Generation 是指为程序生成附加代码的能力。

对于 Computation，现在已经有两种选项：递归实例化和 constexpr。

对于 Reflection，我们在 type traits 中找到了部分解决方案。尽管可用的特征支持相当多的高级模板技术，但它们还远远不能涵盖语言中反射工具所需的所有功能。例如，给定一个 class type，许多应用程序希望以编程方式探索该类的成员。

当前的 traits 基于模板实例化，C++ 可以提​​供额外的语言设施或内在库组件，以生成包含编译时反映的信息的类模板实例。这种方法非常适合基于递归模板实例化的计算。但是，类模板实例化会占用大量编译器存储空间并且直到编译结束才会被释放。

另一种选择是引入新的标准类型表示反射的信息，可以与 Computation 中的 constexpr 相配合。第 17.9 节还展示了实现强大代码生成的未来方法。在现有 C++ 语言中创建一个灵活的、通用的、开发者友好的代码生成机制，仍然是一个挑战。实例化模板是一种“代码生成”机制，编译器在扩展对内联小函数的调用方面已经足够可靠，这种机制可以用作代码生成的载体。这些观察结果正是上面 DotProductT 示例的基础，结合更强大的反射功能，现有技术已经可以完全实现元编程。

# The Cost of Recursive Instantiation

分析 Sqrt in value metaprogramming 的开销，代码如下：

```c++
// recursion value metaprogramming without constexpr for function
template <std::size_t N, std::size_t L = 1, std::size_t R = N> struct Sqrt {
  static constexpr std::size_t mid = (L + R + 1) / 2;

  static constexpr std::size_t value =
      (mid * mid > N) ? Sqrt<N, L, mid - 1>::value : Sqrt<N, mid, R>::value;
};

template <std::size_t N, std::size_t M> struct Sqrt<N, M, M> {
  static constexpr std::size_t value = M;
};
```

算法使用二分查找，当上下界相同时代码返回 value。

模板实例化 is not cheap：即使是相对适度的类模板也可以为每个实例分配超过 1 KB 的存储空间，并且在编译完成之前无法回收该存储空间。因此，让我们检查一下使用 Sqrt 模板的简单程序的详细信息，以 Sqrt<16> 为例。

## Tracking All Instantiations

注意，编译器对于 ?: 表达式，true 和 false 的分支都会实例化，并且代码又试图通过 :: 访问 value，那么 false 分支内的所有成员也会被实例化。这意味着 Sqrt<16,9,16> 的完全实例化会导致 Sqrt<16,9,12> 和 Sqrt<16,13,16> 的完全实例化。当详细检查整个过程时，会发现生成了几十个实例，总数是 N 值的两倍。

可以使用 19 章的 IFELSETHEN 模板防止过度实例化：

```c++
#include "ifthenelse.hpp"
// primary template for main recursive step
template <int N, int LO = 1, int HI = N> struct Sqrt {
  // compute the midpoint, rounded up
  static constexpr auto mid = (LO + HI + 1) / 2;
  // search a not too large value in a halved interval
  using SubT =
      IfThenElse<(N < mid * mid), Sqrt<N, LO, mid - 1>, Sqrt<N, mid, HI>>;
  static constexpr auto value = SubT::value;
};
// partial specialization for end of recursion criterion
template <int N, int S> struct Sqrt<N, S, S> {
  static constexpr auto value = S;
};
```

即，通过 IFThenElse 拿到类型，然后再通过 :: 来实例化拿取 value。

**重要：IfThenElse 模板是一种根据给定布尔常量在两种类型之间进行选择的工具。如果常量为 true，则第一个类型的类型别名为 Type；否则，Type 代表第二种类型。为类模板实例定义类型别名不会导致 C++ 编译器实例化该实例的主体。**

当使用改进后的时候，只有当调用 value = SubT::value 时，类型的 body 才会被实例化。

# Computational Completeness

Sqrt<> 说明模板元编程可以包含：

- 状态变量: 模板参数

- 循环构造: 通过递归

- 执行路径选择: 通过使用条件表达式或特化

- 整数运算

如果递归实例化的数量和允许的状态变量的数量没有限制，则可以证明这足以计算任何可计算的东西。但是大量递归可能会耗尽资源，因此 C++ 标准建议至少允许 1024 级递归任务。

因此，在实践中，应谨慎使用模板元程序。然而，在某些情况下，它们作为实现便捷模板的工具是不可替代的。特别是，它们有时可以隐藏在更传统模板的内部，以从关键算法实现中挤出更多性能。

# Recursive Instantiation versus Recursive Template Arguments

考虑下面离谱的实例化：

```c++
template <typename T, typename U> struct Doublify {};
template <int N> struct Trouble {
  using LongType = Doublify<typename Trouble<N - 1>::LongType,
                            typename Trouble<N - 1>::LongType>;
};
template <> struct Trouble<0> { using LongType = double; };
Trouble<10>::LongType ouch;
```

表达式`Trouble<N>::LongType`的类型描述的复杂性随着N呈指数增长。早期的 C++ 实现使用的编码大致与模板标识的长度成比例。这些编译器为 Trouble<10>::LongType 使用了超过 10,000 个字符的内存。

较新的 C++ 实现考虑到，嵌套模板标识在现代 C++ 程序中相当常见，并使用巧妙的压缩技术来大幅减少名称编码的增长 (例如，Trouble<10>::LongType 需要几百个字符)。因为没有为模板实例生成低层代码，这些编译器还会避免在不需要的情况下生成混乱的名称。尽管如此，在其他条件都相同的情况下，以模板参数不需要递归嵌套的方式组织递归实例化可能是更好的方法。

# Enumeration Values versus Static Constants

在 C++ 的早期，枚举值是创建“真正的常量”（称为常量表达式）作为类声明中的命名成员的唯一机制。如计算 3 的幂：

```c++
// primary template to compute 3 to the Nth
template <int N> struct Pow3 {
  enum { value = 3 * Pow3<N - 1>::value };
};
// full specialization to end the recursion
template <> struct Pow3<0> {
  enum { value = 1 };
};
```

C++98 的标准化引入了类内静态常量初始化器的概念，因此 Pow3 元程序可以这样写:

```c++
// primary template to compute 3 to the Nth
template <int N> struct Pow3 {
  static int const value = 3 * Pow3<N - 1>::value;
};
// full specialization to end the recursion
template <> struct Pow3<0> { static int const value = 1; };
```

但是也会有缺点，static constant member 是一个左值。因此，当我们声明：

`void foo(int const&);` 

并且我们将元程序的结果传递给它：

 `foo(Pow3<7>::value);`。

 这时，编译器必须传递 value 的地址，这就强制编译器实例化并且为 value 分配静态成员。这时计算就并不是纯粹的编译时效果。

 但是 enum 类型就不是左值了，当通过引用传递时，不会使用静态内存。

 然而，C++11 引入了 constexpr 静态数据成员，并且这些成员不限于整型。它们没有解决上面提出的地址问题，但尽管有这个缺点，它们现在是产生元程序结果的常用方法。它们的优点是具有正确的类型（与人工枚举类型相反），并且当使用 auto 类型说明符声明静态成员时可以推导出该类型。 **C++17添加了内联静态数据成员**，确实解决了上面提出的地址问题，并且可以与constexpr结合使用。


```c++
// link error: undefined reference to `Foo<int>::value'
template <typename T> struct Foo { static const std::size_t value = 1; };
void func(size_t const &v) { std::cout << v << std::endl; }

void test2() { func(Foo<int>::value); }

// OK
template <typename T> struct Foo { inline static const std::size_t value = 1; };
```
