在本章中，我们开发一个类模板 Variant，它动态存储给定的一组可能值类型之一的值，类似于 C++17 标准库的 std::variant<>。 

Variant 是一个可区分的联合，这意味着变体知道其可能的值类型当前处于活动状态，从而提供比等效的 C++ 联合更好的类型安全性。 Variant 本身是一个可变参数模板，它接受活动值可能具有的类型列表。

我们可以使用 `is<T>()` 判断是否为 T 类型并使用 `get<T>()` 提取相关的值。

# Storage

Variant 第一个要实现的是如何管理存储的值。一个简单粗暴的实现是为每个类型都使用一个来存储。不过这样占用的空间太多，并且需要每一种类型都具有一个构造函数才可以。

```c++
template<typename... Types>
class Variant {
public:
  Tuple<Types...> storage;
  unsigned char discriminator;
};
```

第二种实现是底层使用 union 来管理，重叠存储每种可能的类型。我们可以通过递归地将变体展开到其头部和尾部来实现这一点：

```c++
template<typename... Types>
union VariantStorage;

template<typename Head, typename... Tail>
union VariantStorage<Head, Tail...> {
  Head head;
  VariantStorage<Tail...> tail;
};
template<> union VariantStorage<> {};
```

然而我们所需要的算法可能需要继承，可是 union 不允许继承。

相反，我们选择变体存储的低级表示：一个足够大的字符数组，可以容纳任何类型，并对任何类型进行适当的对齐，我们将其用作存储活动值的缓冲区。 VariantStorage 类模板实现此缓冲区以及鉴别器。

```c++
template <typename... Types> class VariantStorage {
  using LargestT = LargestType<Typelist<Types...>>;
  alignas(Types...) unsigned char buffer[sizeof(LargestT)];
  unsigned char discriminator = 0;

public:
  unsigned char getDiscriminator() const { return discriminator; }
  void setDiscriminator(unsigned char d) { discriminator = d; }
  void *getRawBuffer() { return buffer; }
  const void *getRawBuffer() const { return buffer; }
  template <typename T> T *getBufferAs() {
    return std::launder(reinterpret_cast<T *>(buffer));
  }
  template <typename T> T const *getBufferAs() const {
    return std::launder(reinterpret_cast<T const *>(buffer));
  }
};
```

LargestType 元程序来计算缓冲区的大小，确保它对于任何值类型都足够大。类似地，alignas 包扩展确保缓冲区具有适合任何值类型的对齐方式。

我们计算的缓冲区本质上是上面显示的联合的机器表示。我们可以使用 getBuffer() 访问指向缓冲区的指针，并通过使用显式强制转换、放置 new（创建新值）和显式销毁（销毁我们创建的值）来操作存储。

[std::launder 的作用](https://www.zhihu.com/question/576066914)

## Design

现在我们已经解决了变体的存储问题，我们设计了 Variant 类型本身。与元组类型一样，我们使用继承来为类型列表中的每种类型提供行为。然而，与元组不同的是，这些基类没有存储空间。相反，每个基类都使用 CRTP，通过最派生类型访问共享变体存储。

```c++
template <typename T, typename... Types> class VariantChoice {
  using Derived = Variant<Types...>;
  Derived &getDerived() { return *static_cast<Derived *>(this); }
  Derived const &getDerived() const {
    return *static_cast<Derived const *>(this);
  }

protected:
  // compute the discriminator to be used for this type
  constexpr static unsigned Discriminator =
      FindIndexOfT<Typelist<Types...>, T>::value + 1;

public:
  VariantChoice() {}
  VariantChoice(T const &value);      // see variantchoiceinit.hpp
  VariantChoice(T &&value);           // see variantchoiceinit.hpp
  bool destroy();                     // see variantchoicedestroy.hpp
  Derived &operator=(T const &value); // see variantchoiceassign.hpp
  Derived &operator=(T &&value);      // see variantchoiceassign.hpp
};
```

Types 包含 Variant 里面的所有类型。其中 FindIndexOf 用来寻找类型 T 的位置。

```c++
// FindIndexof
template <typename List, typename T, unsigned N = 0,
          bool Empty = IsEmpty<List>::value>
struct FindIndexOfT;
template <typename List, typename T, unsigned N>
struct FindIndexOfT<List, T, N, false>
    : public std::conditional_t<std::is_same_v<Front<List>, T>,
                                std::integral_constant<unsigned, N>,
                                FindIndexOfT<PopFront<List>, T, N + 1>> {};

template <typename List, typename T, unsigned N>
struct FindIndexOfT<List, T, N, true> {};
```

Variant的框架，Variant、VariantStorage和VariantChoice之间的关系如下：

```c++
template <typename... Types>
class Variant : private VariantStorage<Types...>,
                private VariantChoice<Types, Types...>... {
  template <typename T, typename... OtherTypes>
  friend class VariantChoice; // enable CRTP
  ...
};
```

如前所述，每个 Variant 都有一个共享的 VariantStorage 基类，同时有若干的 VariantChoice 基类，是类似于 `VariantChoice<Type, Types...>...` 的形式。因此对于 `Variant<int, double, std::string>`，将会生成 
`VariantChoice<int, int, double, std::string>`
`VariantChoice<double, int, double, std::string>`
`VariantChoice<std::string, int, double, std::string>`
三种 Choice 基类。

这三个基类的鉴别器值分别为 1、2 和 3。当变体存储的鉴别器成员与特定 VariantChoice 基类的鉴别器匹配时，该基类负责管理活动值。

鉴别器值 0 是为变体不包含值的情况保留的，这是一种奇怪的状态，只有在赋值期间抛出异常时才能观察到。在 Variant 的整个讨论中，我们将小心处理判别器值为 0（并在适当的时候设置它），将在第三节中加以讨论。

于是完整的 Variant 如下所示，后文将对剩余的成员函数进行实现。

```c++
template <typename... Types>
class Variant : private VariantStorage<Types...>,
                private VariantChoice<Types, Types...>... {
  template <typename T, typename... OtherTypes> friend class VariantChoice;

public:
  template <typename T> bool is() const;        // see variantis.hpp
  template <typename T> T &get() &;             // see variantget.hpp
  template <typename T> T const &get() const &; // see variantget.hpp
  template <typename T> T &&get() &&;           // see variantget.hpp
  // see variantvisit.hpp:
  template <typename R = ComputedResultType, typename Visitor>
  VisitResult<R, Visitor, Types &...> visit(Visitor &&vis) &;
  template <typename R = ComputedResultType, typename Visitor>
  VisitResult<R, Visitor, Types const &...> visit(Visitor &&vis) const &;
  template <typename R = ComputedResultType, typename Visitor>
  VisitResult<R, Visitor, Types &&...> visit(Visitor &&vis) &&;
  using VariantChoice<Types, Types...>::VariantChoice...;
  Variant();                      // see variantdefaultctor.hpp
  Variant(Variant const &source); // see variantcopyctor.hpp
  Variant(Variant &&source);      // see variantmovector.hpp
  template <typename... SourceTypes>
  Variant(Variant<SourceTypes...> const &source); // variantcopyctortmpl.hpp
  template <typename... SourceTypes> Variant(Variant<SourceTypes...> &&source);
  using VariantChoice<Types, Types...>::operator=...;
  Variant &operator=(Variant const &source); // see variantcopyassign.hpp
  Variant &operator=(Variant &&source);
  template <typename... SourceTypes>
  Variant &operator=(Variant<SourceTypes...> const &source);
  template <typename... SourceTypes>
  Variant &operator=(Variant<SourceTypes...> &&source);
  bool empty() const;
  ~Variant() { destroy(); }
  void destroy(); // see variantdestroy.hpp
};
```

# Value Query and Extraction

对 Variant 类型最基本的查询是询问它的活动值是否属于特定类型 T，并在已知其类型时访问活动值。 is() 成员函数（定义如下）确定变量当前是否存储 T 类型的值（如果找不到类型将会编译错误）：

```c++
template<typename... Types>
template<typename T>
bool Variant<Types...>::is() const {
  return this->getDiscriminator() ==
          VariantChoice<T, Types...>::Discriminator;
}
```

get() 成员函数提取对存储值的引用。它必须提供要提取的类型（例如，v.get<int>()），并且仅当变体的活动值属于该类型时才有效

```c++
template <typename T> T &get() & {
  if (empty()) {
    // ... throw exception
  }
  static_assert(is<T>());
  return *this->template getBufferAs<T>();
}
```

# Element Initialization, Assignment and Destruction

## Initialization

当活动值的类型为 T 时，每个 VariantChoice 基类负责处理初始化、赋值和销毁。本节通过填写 VariantChoice 类模板的详细信息来开发这些核心操作。

```c++
template <typename T, typename... Types>
VariantChoice<T, Types...>::VariantChoice(T const &value) {
  // place value in buffer and set type discriminator:
  new (getDerived().getRawBuffer()) T(value);
  getDerived().setDiscriminator(Discriminator);
}

template <typename T, typename... Types>
VariantChoice<T, Types...>::VariantChoice(T &&value) {
  // place moved value in buffer and set type discriminator:
  new (getDerived().getRawBuffer()) T(std::move(value));
  getDerived().setDiscriminator(Discriminator);
}
```

在每种情况下，构造函数都使用 CRTP 操作 getDerived() 来访问共享缓冲区，然后执行放置 new 以使用类型 T 的新值初始化存储。第一个构造函数复制构造传入值，而第二个构造函数移动构造传入的值。然后，构造函数设置鉴别器值以指示变体存储的（动态）类型。

我们的最终目标是能够从任何类型的值初始化变体，甚至考虑到隐式转换。例如：

`Variant<int, double, string> v("hello"); // implicitly converted to string`

为了实现这一点，我们通过引入 using 声明将 VariantChoice 构造函数继承到 Variant 本身：

实际上，此 using 声明生成从 Types 中的每个类型 T 复制或移动的 Variant 构造函数。对于 Variant<int, double, string>，构造函数实际上是

```c++
Variant(int const&);
Variant(int&&);
Variant(double const&);
Variant(double&&);
Variant(string const&);
Variant(string&&);
```

? 这里不太理解，为什么构造函数是这个展开而不是 VariantChoice 的展开，感觉是不是写错了。

## Destruction

当 Variant 初始化时，一个值被构造到它的缓冲区中。 destroy 操作处理该值的销毁:

```c++
template <typename T, typename... Types>
bool VariantChoice<T, Types...>::destroy() {
  if (getDerived().getDiscriminator() == Discriminator) {
    // if type matches, call placement delete:
    getDerived().template getBufferAs<T>()->~T();
    return true;
  }
  return false;
}
```

VariantChoice::destroy() 操作仅在鉴别器匹配时才有用。但是，我们通常希望销毁存储在变体中的值，而不考虑当前处于活动状态的类型。因此，Variant::destroy() 调用其基类中的所有 VariantChoice::destroy() 操作

```c++
template <typename... Types> void Variant<Types...>::destroy() {
  bool results = {VariantChoice<Types, Types...>::destroy()...};
  this->setDiscriminator(0);
}
```

## Assignment

赋值建立在初始化和销毁​​的基础上，如赋值运算符所示:

```c++
template <typename T, typename... Types>
auto VariantChoice<T, Types...>::operator=(T const &value) -> Derived & {
  if (getDerived().getDiscriminator() == Discriminator) {
    // assign new value of same type:
    *getDerived().template getBufferAs<T>() = value;
  } else {
    // assign new value of different type:
    getDerived().destroy();                     // try destroy() for all types
    new (getDerived().getRawBuffer()) T(value); // place new value
    getDerived().setDiscriminator(Discriminator);
  }
  return getDerived();
}

template <typename T, typename... Types>
auto VariantChoice<T, Types...>::operator=(T &&value) -> Derived & {
  if (getDerived().getDiscriminator() == Discriminator) {
    // assign new value of same type:
    *getDerived().template getBufferAs<T>() = std::move(value);
  } else {
    // assign new value of different type:
    getDerived().destroy(); // try destroy() for all types
    new (getDerived().getRawBuffer()) T(std::move(value)); // place new value
    getDerived().setDiscriminator(Discriminator);
  }
  return getDerived();
}
```

与从存储值类型之一进行初始化一样，每个 VariantChoice 都提供一个赋值运算符，用于将其存储值类型复制（或移动）到 Variant 的存储中。这些赋值运算符由 Variant 通过以下 using 声明继承：

```c++
using VariantChoice<Types, Types...>::operator=...;
```

注：当我们对 Variant 进行赋值的时候，首先因为 using operator=(value ...)，因此，Variant 的赋值运算符就使用了 VariantChoice 的；其次，编译器会根据 value 的类型来选定调用的 operator=，因此只会调用其中一个。

如果变量不存储 T 类型的值，则赋值过程需要两步：使用 Variant::destroy() 销毁当前值，然后使用placement new 初始化 T 类型的新值，并适当设置鉴别器。

使用新布局的两步分配存在三个常见问题，我们必须考虑这些问题：自赋值、例外和 std::launder()

### Self Assginment

由于如下表达式，变体 v 可能会发生自分配： 

```c++
v = v.get<T>() 
```

通过上面实现的两步过程，源值将在复制之前被破坏，从而可能导致内存损坏。

幸运的是，自赋值总是意味着鉴别器匹配，因此此类代码将调用 T 的赋值运算符，而不是这个两步过程。

### Exceptions

如果现有值的销毁完成，但新值的初始化抛出异常，则变体的状态是什么？在我们的实现中，Variant::destroy() 将鉴别器值重置为 0。在非例外情​​况下，鉴别器将在初始化完成后进行适当设置。当新值初始化期间发生异常时，鉴别器保持为 0 以指示变体不存储值。在我们的设计中，这是生成没有值的变体的唯一方法。

### std::launder()

第三个问题是一个微妙的问题，C++ 标准化委员会直到 C++17 标准化过程结束时才意识到这一问题。接下来我们简单解释一下。

C++编译器通常旨在生成高性能代码，也许提高生成代码性能的主要机制是避免重复地将数据从内存复制到寄存器。为了做好这一点，编译器必须做出一些假设，其中之一是某些类型的数据在其生命周期内是不可变的。其中包括 const 数据、引用（可以初始化，但之后不能修改）以及存储在多态对象中的一些簿记数据，这些数据用于分派虚拟函数、定位虚拟基类以及处理 typeid 和dynamic_cast 运算符。

上面的两步赋值过程的问题在于，它以编译器可能无法识别的方式偷偷地结束了一个对象的生命周期，并在同一位置开始了另一个对象的生命周期。因此，编译器可能会假设从 Variant 对象的先前状态获取的值仍然有效，而事实上，使用 new 放置的初始化使其无效。如果不采取缓解措施，最终结果将是使用具有不可变数据成员的类型变体的程序在编译以获得良好性能时有时可能会产生无效结果。此类错误通常很难追踪（部分原因是它们很少发生，部分原因是它们在源代码中并不真正可见）。

从 C++17 开始，这个问题的解决方案是通过 std::launder() 访问新对象的地址，它只返回其参数，但这会导致编译器识别出结果地址指向一个对象可能与编译器对传递给 std::launder() 的参数的假设不同。但是，请注意 std::launder() 仅修复它返回的地址，而不是传递给 std::launder() 的参数，因为编译器根据表达式而不是实际地址进行推理（因为它们直到运行时才存在）。因此，在用placement new构造出新值后，我们必须确保接下来的每次访问都使用“洗过的”数据。这就是为什么我们总是“清洗”指向 Variant 缓冲区的指针。有一些方法可以做得更好（例如添加一个额外的指针成员，该成员引用缓冲区并在每次使用placement new分配新值后获取“经过清洗的”地址），但它们使代码变得复杂，很难维护。我们的方法既简单又正确，只要我们通过 getBufferAs() 成员专门访问缓冲区即可。

std::launder() 的情况并不完全令人满意：它非常微妙，难以察觉，并且难以缓解（即，std::launder() 不太好用）。因此，委员会的几位成员要求开展更多工作，以找到更令人满意的解决方案。

# Visitors

is() 和 get() 成员函数允许我们检查活动值是否属于特定类型并访问该类型的值。然而，检查变体中所有可能的类型很快就会变成冗余的 if 语句链

```c++
if (v.is<int>()) {
  std::cout << v.get<int>();
} else if (v.is<double>()) {
  std::cout << v.get<double>();
} else {
  std::cout << v.get<string>();
}
```

为了概括这一点以打印存储在任意变体中的值，需要一个递归实例化的函数模板和一个助手。

```c++
template <typename V, typename Head, typename... Tail>
void printImpl(V const &v) {
  if (v.template is<Head>()) {
    std::cout << v.template get<Head>();
  } else if constexpr (sizeof...(Tail) > 0) {
    printImpl<V, Tail...>(v);
  }
}
template <typename... Types> void print(Variant<Types...> const &v) {
  printImpl<Variant<Types...>, Types...>(v);
}
int main() {
  Variant<int, short, float, double> v(1.5);
  print(v);
}
```

对于相对简单的操作来说，这是大量的代码。为了简化这个问题，我们通过使用 Visit() 操作扩展 Variant 来扭转问题。然后，客户端传入一个访问者函数对象，该对象的operator()将使用活动值来调用。因为活动值可能是变体的任何一种潜在类型，所以这个operator()很可能被重载或者本身就是一个函数模板。

例如，通用 lambda 提供了模板化的operator()，允许我们简洁地表示变体v的打印操作：

```c++
v.visit([](auto const& value) {
  std::cout << value;
});
```

Visit() 操作的核心类似于递归打印操作：它遍历 Variant 的类型，检查活动值是否具有给定类型（使用 is<T>()），然后在具有给定类型时执行操作找到合适的类型。

```c++
template <typename R, typename V, typename Visitor, typename Head,
          typename... Tail>
R variantVisitImpl(V &&variant, Visitor &&vis, Typelist<Head, Tail...>) {
  if (variant.template is<Head>()) {
    return static_cast<R>(std::forward<Visitor>(vis)(
        std::forward<V>(variant).template get<Head>()));
  } else if constexpr (sizeof...(Tail) > 0) {
    return variantVisitImpl<R>(std::forward<V>(variant),
                               std::forward<Visitor>(vis), Typelist<Tail...>());
  } else {
    throw EmptyVariant();
  }
}
```

variantVisitImpl() 是一个带有许多模板参数的非成员函数模板。模板参数R描述了访问操作的结果类型，我们稍后会返回。 V 是变体的类型，Visitor 是访问者的类型。 Head和Tail用于分解Variant中的类型以实现递归。

除了 VisitResult 提供的结果类型计算（将在下一节中讨论）之外，visit() 的实现也很简单：

```c++
template <typename... Types>
template <typename R, typename Visitor>
VisitResult<R, Visitor, Types &...> Variant<Types...>::visit(Visitor &&vis) & {
  using Result = VisitResult<R, Visitor, Types &...>;
  return variantVisitImpl<Result>(*this, std::forward<Visitor>(vis),
                                  Typelist<Types...>());
}
template <typename... Types>
template <typename R, typename Visitor>
VisitResult<R, Visitor, Types const &...>
Variant<Types...>::visit(Visitor &&vis) const & {
  using Result = VisitResult<R, Visitor, Types const &...>;
  return variantVisitImpl<Result>(*this, std::forward<Visitor>(vis),
                                  Typelist<Types...>());
}
template <typename... Types>
template <typename R, typename Visitor>
VisitResult<R, Visitor, Types &&...>
Variant<Types...>::visit(Visitor &&vis) && {
  using Result = VisitResult<R, Visitor, Types &&...>;
  return variantVisitImpl<Result>(std::move(*this), std::forward<Visitor>(vis),
                                  Typelist<Types...>());
}
```

实现直接委托给variantVisitImpl，传递 Variant 本身，转发访问者，并提供完整的类型列表。三种实现之间的唯一区别在于它们是否将 Variant 本身作为 Variant&、Variant const& 或 Variant&& 传递。

### Visit Result Type

Visit() 的结果类型仍然是个谜。给定的访问者可能有不同的operator()重载，产生不同的结果类型，模板化的operator()，其结果类型取决于其参数类型，或者它们的某种组合。考虑：

```c++
[](auto const& value) {
  return value + 1;
}
```

该 lambda 的结果类型取决于输入类型：给定一个 int，它将产生一个 int，但给定一个 double，它将产生一个 double。如果将此泛型 lambda 传递给 Variant<int, double> 的访问() 操作，结果应该是什么？

没有单一的正确答案，因此我们的访问（）操作允许显式提供结果类型。例如，人们可能想要捕获另一个 `Variant<int, double>` 中的结果。人们可以显式指定访问（）的结果类型作为第一个模板参数：

```c++
v.visit<Variant<int, double>>([](auto const &value) { return value + 1; });
```

当没有一刀切的解决方案时，显式指定结果类型的能力非常重要。但是，要求在所有情况下都显式指定结果类型可能会很冗长。因此，visit() 使用默认模板参数和简单元程序的组合来提供这两个选项。回想一下访问（）的声明：

```c++
template<typename R = ComputedResultType, typename Visitor>
VisitResult<R, Visitor, Types&...> visit(Visitor&& vis) &;
```

我们在上面的示例中显式指定的模板参数 R 也有一个默认参数，因此不必总是显式指定它。该默认参数是不完整的哨兵类型 ComputedResultType。

为了计算其结果类型，visit 将其所有模板参数传递给 VisitResult，这是一个别名模板，提供对新类型特征 VisitResultT 的访问：

```c++
template <typename R, typename Visitor, typename... ElementTypes>
class VisitResultT {
public:
  using Type = R;
};
template <typename R, typename Visitor, typename... ElementTypes>
using VisitResult = typename VisitResultT<R, Visitor, ElementTypes...>::Type;
```

VisitResultT 的主要定义处理已显式指定 R 参数的情况，因此 Type 被定义为 R。当 R 接收其默认参数 ComputedResultType 时，将应用单独的部分特化：

```c++
template <typename Visitor, typename... ElementTypes>
class VisitResultT<ComputedResultType, Visitor, ElementTypes...> {
  ...
}
```

此部分专业化负责计算常见情况的适当结果类型，并且是下一节的主题。

### Common Result Type

当调用可能为每个 Variant 元素类型返回不同类型的访问者时，我们如何将这些类型组合成访问（）的单个结果类型？有一些明显的情况 - 如果访问者为每个元素类型返回相同的类型，那么这应该是访问（）的结果类型。

C++ 已经有了合理结果类型的概念，在三元表达式 b ? x : y，表达式的类型是x和y类型之间的公共类型。例如，如果 x 的类型为 int，y 的类型为 double，则通用类型为 double，因为 int 提升为 double。我们可以在 type traits 中捕获公共类型的概念：

```c++
using std::declval;
template <typename T, typename U> class CommonTypeT {
public:
  using Type = decltype(true ? declval<T>() : declval<U>());
};
template <typename T, typename U>
using CommonType = typename CommonTypeT<T, U>::Type;
```

公共类型的概念扩展到类型的集合：公共类型是该集合中的所有类型都可以提升到的类型。对于我们的访问者，我们想要计算访问者在使用变体中的每种类型调用时将产生的结果类型的公共类型：

```c++
// the result type produced when calling a visitor with a value of type T:
template <typename Visitor, typename T>
using VisitElementResult = decltype(declval<Visitor>()(declval<T>()));
// the common result type for a visitor called with each of the given element
// types:
template <typename Visitor, typename... ElementTypes>
class VisitResultT<ComputedResultType, Visitor, ElementTypes...> {
  using ResultTypes = Typelist<VisitElementResult<Visitor, ElementTypes>...>;

public:
  using Type =
      Accumulate<PopFront<ResultTypes>, CommonTypeT, Front<ResultTypes>>;
};
```

VisitResult 计算分两个阶段进行。首先，VisitElementResult 计算使用类型 T 的值调用访问者时生成的结果类型。此元函数应用于每个给定的元素类型，以确定访问者可能生成的所有结果类型，捕获类型列表 ResultTypes 中的结果。

接下来，计算使用累加算法，将普通类型计算应用于结果类型的类型列表。它的初始值（Accumulate 的第三个参数）是第一个结果类型，它通过 CommonTypeT 与 ResultTypes 类型列表的其余部分中的连续值组合。最终结果是所有访问者的结果类型都可以转换为的通用类型，如果结果类型不兼容，则会出现错误。

从 C++11 开始，标准库提供了相应的类型特征 std::common_type<>，它使用这种方法来生成任意数量的传递类型的公共类型，有效地结合了 CommonTypeT 和 Accumulate。通过使用 std::common_type<>，VisitResultT 的实现更加简单：

```c++
template <typename Visitor, typename... ElementTypes>
class VisitResultT<ComputedResultType, Visitor, ElementTypes...> {
public:
  using Type = std::common_type_t<VisitElementResult<Visitor, ElementTypes>...>;
};
```

以下示例程序打印出通过传入通用 lambda 生成的类型，该 lambda 将其获取的值加 1：

```c++
#include "variant.hpp"
#include <iostream>
#include <typeinfo>
int main() {
  Variant<int, short, double, float> v(1.5);
  auto result = v.visit([](auto const &value) { return value + 1; });
  std::cout << typeid(result).name() << ’\n’;
}
```

该程序的输出将是 double 的 type_info 名称，因为这是所有结果类型都可以转换为的类型。

# Variant Initialization and Assignment

Variant 可以通过多种方式初始化和分配，包括默认构造、复制和移动构造以及复制和移动分配。本节详细介绍这些操作。

## Default Initialization

变体应该提供默认构造函数吗？如果不这样做，变体可能会不必要地难以使用，因为人们总是必须想出一个初始值（即使在编程上没有意义）。如果它确实提供了默认构造函数，那么语义应该是什么？

一种可能的语义是默认初始化没有存储值，由鉴别器 0 表示。但是，这种空 Variant 通常没有用（例如，无法访问它们或找到任何要提取的值），并将其设为默认值初始化行为会将空 Variant 的异常状态提升为常见状态。或者，默认构造函数可以构造某种类型的值。

对于我们的 Variant，我们遵循 C++17 的 std::variant<> 的语义，并默认构造类型列表中第一个类型的值：

```c++
template<typename... Types>
Variant<Types...>::Variant() {
  *this = Front<Typelist<Types...>>();
}
```

这种方法简单且可预测，并且避免在大多数用途中引入空变体。

## Copy/Move Initialization

复制和移动初始化更有趣。要复制源 variant，我们需要确定它当前存储的类型，将该值复制构造到缓冲区中，并设置该鉴别器。幸运的是，visit() 负责解码源变量的活动值，并且从 VariantChoice 继承的复制赋值运算符会将一个值复制构造到缓冲区中，从而实现紧凑的实现

```c++
template <typename... Types> Variant<Types...>::Variant(Variant const &source) {
  if (!source.empty()) {
    source.visit([&](auto const &value) { *this = value; });
  }
}
```

移动构造函数类似，不同之处仅在于访问源变体时使用 std::move 以及从源值进行移动赋值:

```c++
template <typename... Types> Variant<Types...>::Variant(Variant &&source) {
  if (!source.empty()) {
    std::move(source).visit([&](auto &&value) { *this = std::move(value); });
  }
}
```

基于访问者的实现的一个特别有趣的方面是它也适用于复制和移动操作的模板化形式。例如，模板化的复制构造函数可以定义如下：

```c++
template <typename... Types>
template <typename... SourceTypes>
Variant<Types...>::Variant(Variant<SourceTypes...> const &source) {
  if (!source.empty()) {
    source.visit([&](auto const &value) { *this = value; });
  }
}
```

因为此代码访问源，所以对于源 Variant 的每种类型都会发生对 *this 的赋值。此分配的重载解析将为每个源类型找到最合适的目标类型，并根据需要执行隐式转换。以下示例说明了不同 Variant 类型的构造和分配：

```c++
#include "variant.hpp"
#include <iostream>
#include <string>
int main() {
  Variant<short, float, char const *> v1((short)123);
  Variant<int, std::string, double> v2(v1);
  std::cout << "v2 contains the integer " << v2.get<int>() << '\n';
  v1 = 3.14f;
  Variant<double, int, std::string> v3(std::move(v1));
  std::cout << "v3 contains the double " << v3.get<double>() << '\n';
  v1 = "hello";
  Variant<double, int, std::string> v4(std::move(v1));
  std::cout << "v4 contains the string " << v4.get<std::string>() << '\n';
}
```

从 v1 到 v2 或 v3 的构造或赋值涉及整型提升（short 到 int）、浮点提升（float 到 double）和用户定义的转换（char const* 到 std::string）。该程序的输出如下：

```
v2 contains the integer 123
v3 contains the double 3.14
v4 contains the string hello
```

## Assignment

Variant 赋值运算符与上面的复制和移动构造函数类似。这里我们只举例说明复制赋值运算符：

```c++
template <typename... Types>
Variant<Types...> &Variant<Types...>::operator=(Variant const &source) {
  if (!source.empty()) {
    source.visit([&](auto const &value) { *this = value; });
  } else {
    destroy();
  }
  return *this;
}
```

唯一有趣的添加是在 else 分支中：当源变量不包含值（由鉴别器 0 表示）时，我们销毁目标的值，隐式将其鉴别器设置为 0。
