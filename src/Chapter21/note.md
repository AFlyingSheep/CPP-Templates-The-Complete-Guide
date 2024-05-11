### Barton-Nackman Trick

在John Barton和Lee Nackman发明这个惯用法的时期，C++不支持函数模板的重载，而且很多C++实现尚不支持名字空间。当需要为类模板定义运算符重载时，这会导致一些问题。考虑下面的类：

要定义一个相等运算符，最自然的想法是在名字空间范围内定义一个非成员函数（由于那个时期的编译器不支持名字空间，所以在全局范围内定义）。将==运算符定义为非成员函数意味着函数的两个参数会被对称的处理，其中一个参数为这个对象的this指针的情况不会出现。这样一个相等运算符可能看起来像这样：

```c++
template<typename T>
bool operator==(List<T> const & lft, List<T> const & rgt) {
   //...
}
```

然而，在那时函数模板是不能被重载的，而且将函数放置在自己的名字空间的做法不能在所有平台上通用。这意味着只有一个类能有这样一个相等运算符。如果有第二个类型要使用，将会导致歧义。

```c++
template<typename T>
class List {
 public:
    friend bool operator==(const List<T> & lft,
                           const List<T> & rgt) {
        // ...
    }
};
```

解决方法：在类的内部定义运算符并声明为友元函数，对模板的实例化就会将这个函数注入到全局范围，与任何其他的非模板函数一样，这个模板函数可以被函数重载决议所选择。

于是可以使用 CRTP 方法，将友元函数作为基类的一部分：

```c++
template<typename T>
class EqualityComparable {
public:
    friend bool operator==(const T & lft, const T & rgt) { return lft.equalTo(rgt); }
    friend bool operator!=(const T & lft, const T & rgt) { return !lft.equalTo(rgt); }
};
  
class ValueType :
    private EqualityComparable<ValueType> {
 public:
    bool equalTo(const ValueType & other) const;
};
```

这里需要插一个内容：[友元函数查找规则](../Chapter13/note.md)
对于如下代码：

```c++
class S {};
template <typename T> class Wrapper {
private:
  T object;

public:
  Wrapper(T obj) : object(obj) { // implicit conversion from T to Wrapper<T>
  }
  friend void foo(Wrapper<T> const &) {}
};
int main() {
  S s;
  Wrapper<S> w(s);
  foo(w); // OK: Wrapper<S> is a class associated with w
  foo(s); // ERROR: Wrapper<S> is not associated with s
}
```

在该 trick 被提出的时候，foo(s) 是可以编译通过的。但是，在当前标准中就不能通过，因为 foo(s) 调用后，会根据它的参数确定关联类并在关联类中查询函数定义。然而 foo(s) 并没有关联到 Wrapper 中，因此查询不到该定义，即使 s 可以被隐式的转换为 Wrapper。

### Operator Implementations

在实现提供重载运算符的类时，通常会为许多不同（但相关）的运算符提供重载。例如，实现相等运算符 (==) 的类也可能实现不等运算符 (!=)，实现小于运算符 (<) 的类也可能实现其他关系运算符 (> ，<=，>=)。

考虑到许多不等运算符，可以概括为模板：

```c++
template<typename T>
bool operator!= (T const& x1, T const& x2) {
  return !(x1 == x2);
}
```

事实上，\<utility\> 中就包含类似的定义，但是他们在标准化过程中降级到了 std::rel_ops 命名空间。如果这些定义可见，那么任何 type 都将具有 != 运算符（可能实例化失败），并且这个定义 will always be an exact match. 即使第一个定义可以通过 SFINAE 解决，第二个问题依旧存在，std::rel_ops 内的定义将始终优于用户提供的。

这些基于 CRTP 的运算符模板的替代表述允许类选择通用运算符定义，从而提供增加代码重用的好处，而不会产生过于通用运算符的副作用：

```c++
template <typename Derived> class EqualityComparable {
public:
  friend bool operator!=(Derived const &x1, Derived const &x2) {
    return !(x1 == x2);
  }
};
class X : public EqualityComparable<X> {
public:
  friend bool operator==(X const &x1, X const &x2) {
    // implement logic for comparing two objects of type X
  }
};
int main() {
  X x1, x2;
  if (x1 != x2) {
  }
}
```

在这里，我们将 CRTP 与 Barton-Nackman 技巧结合起来。 EqualityComparable<> 使用 CRTP 根据派生类的运算符 == 的定义为其派生类提供运算符！=。它实际上通过友元函数定义（Barton-Nackman 技巧）提供了该定义，该定义将两个参数赋予运算符！= 相等的转换行为。

当将**行为分解到基类中同时保留最终派生类的标识时**，CRTP 非常有用。与 Barton-Nackman 技巧一起，CRTP 可以基于一些规范运算符为许多运算符提供通用定义。这些特性使得采用 Barton-Nackman 技巧的 CRTP 成为 C++ 模板库作者最喜欢的技术。

### Facades

CRTP base class defines **most or all of the public interface of a class** in terms of a much smaller (but easier to implement) interface exposed by the CRTP derived class.

这个 pattern 就叫做 facades pattern。在定义需要满足某些现有接口（数字类型、迭代器、容器等）要求的新类型时特别有用。

为了说明 facades 模式，我们将为迭代器实现一个 facades，这大大简化了编写符合标准库要求的迭代器的过程。迭代器类型（特别是随机访问迭代器）所需的接口非常大。以下类模板 IteratorFacade 的框架演示了迭代器接口的要求：

```c++
template <typename Derived, typename Value, typename Category,
          typename Reference = Value &, typename Distance = std::ptrdiff_t>
class IteratorFacade {
public:
  using value_type = typename std::remove_const<Value>::type;
  using reference = Reference;
  using pointer = Value *;
  using difference_type = Distance;
  using iterator_category = Category;
  // input iterator interface:
  reference operator*() const { // ...
  }
  pointer operator->() const { // ...
  }
  Derived &operator++() { // ...
  }
  Derived operator++(int) { // ...
  }
  friend bool operator==(IteratorFacade const &lhs,
                         IteratorFacade const &rhs) { // ...
  }
  // ...
  // bidirectional iterator interface:
  Derived &operator--() { // ...
  }
  Derived operator--(int) { // ...
  }

  // random access iterator interface:
  reference operator[](difference_type n) const { // ...
  }
  Derived &operator+=(difference_type n) { // ...
  }
  // ...
  friend difference_type operator-(IteratorFacade const &lhs,
                                   IteratorFacade const &rhs) { // ...
  }
  friend bool operator<(IteratorFacade const &lhs,
                        IteratorFacade const &rhs) { // ...
  }
  // ...
};
```

太多了，麻烦死了。但幸运的是，这些接口可以分为这几个部分：

对于所有迭代器： 
  - dereference()：访问迭代器引用的值（通常通过运算符 * 和 -> 使用）。 
  - increment()：移动迭代器以引用序列中的下一项。
  - equals()：确定两个迭代器是否引用序列中的同一项目。 

对于双向迭代器： 
  - decrement()：移动迭代器以引用列表中的前一项。

对于随机访问迭代器： 
  - advance()：将迭代器向前（或向后）移动n 步。 
  - measureDistance()：确定序列中从一个迭代器到另一个迭代器的步骤数。

facade 的目的就是为了继承类仅需要实现核心接口，就可以拥有完整的功能。可以通过如下方式访问继承类：

```c++
Derived& asDerived() { return *static_cast<Derived*>(this); }
Derived const& asDerived() const {
  return *static_cast<Derived const*>(this);
}
```

有了这些核心函数，其他函数实现就很简单了：

```c++
reference operator*() const { return asDerived().dereference(); }
Derived &operator++() {
  asDerived().increment();
  return asDerived();
}
Derived operator++(int) {
  Derived result(asDerived());
  asDerived().increment();
  return result;
}
friend bool operator==(IteratorFacade const &lhs, IteratorFacade const &rhs) {
  return lhs.asDerived().equals(rhs.asDerived());
}
```

于是就可以很 easy 的实现一个链表迭代器：

```c++
template <typename T> class ListNode {
public:
  T value;
  ListNode<T> *next = nullptr;
  ~ListNode() { delete next; }
};

template <typename T>
class ListNodeIterator
    : public IteratorFacade<ListNodeIterator<T>, T, std::forward_iterator_tag> {
  ListNode<T> *current = nullptr;

public:
  T &dereference() const { return current->value; }
  void increment() { current = current->next; }
  bool equals(ListNodeIterator const &other) const {
    return current == other.current;
  }
  ListNodeIterator(ListNode<T> *current = nullptr) : current(current) {}
};
```

但是这样有一个缺点：我们需要将 dereference()、advance() 和 equals() 操作公开为公共接口。为了消除这个要求，我们可以重新设计 IteratorFacade，通过一个单独的访问类（我们称之为 IteratorFacadeAccess）在派生的 CRTP 类上执行其所有操作。

IteratorFacadeAccess 就是为了上面的目的而实现的：

**没看明白咋做的**

```c++
// 'friend' this class to allow IteratorFacade access to core iterator
// operations:
class IteratorFacadeAccess {
  // only IteratorFacade can use these definitions
  template <typename Derived, typename Value, typename Category,
            typename Reference, typename Distance>
  friend class IteratorFacade;
  // required of all iterators:
  template <typename Reference, typename Iterator>
  static Reference dereference(Iterator const &i) {
    return i.dereference();
  }
  // ...
  // required of bidirectional iterators:
  template <typename Iterator> static void decrement(Iterator &i) {
    return i.decrement();
  }
  // required of random-access iterators:
  template <typename Iterator, typename Distance>
  static void advance(Iterator &i, Distance n) {
    return i.advance(n);
  }
  // ...
};
```

根据以上讲解，我们实现一个 Iterator Adapter：

定义 People 类：

```c++
struct Person {
  std::string firstName;
  std::string lastName;
  friend std::ostream& operator<<(std::ostream& strm, Person const& p) {
    return strm << p.lastName << ", " << p.firstName;
  }
};
```

但是在迭代的过程中，我们只想显示 firstname，于是自定义一个迭代器 ProjectionIterator：

```c++
template <typename Iterator, typename T>
class ProjectionIterator
    : public IteratorFacade<
          ProjectionIterator<Iterator, T>, T,
          typename std::iterator_traits<Iterator>::iterator_category, T &,
          typename std::iterator_traits<Iterator>::difference_type> {
  using Base = typename std::iterator_traits<Iterator>::value_type;
  using Distance = typename std::iterator_traits<Iterator>::difference_type;
  // Iterator 是迭代器，比如本次 Iterator 为 vector 的迭代器
  Iterator iter;
  // 指向成员的指针，返回 T 为 string
  T Base::*member;
  friend class IteratorFacadeAccess;
  // ... 
  // implement core iterator operations for IteratorFacade
  public : 
    ProjectionIterator(Iterator iter, T Base::*member) : iter(iter), member(member) {}
};

template <typename Iterator, typename Base, typename T>
auto project(Iterator iter, T Base::*member) {
  return ProjectionIterator<Iterator, T>(iter, member);
}
```

接着我们对核心函数进行定义，最特别的是 dereference：

```c++
T &dereference() const { return (*iter).*member; }

void increment() { ++iter; }
bool equals(ProjectionIterator const &other) const {
  return iter == other.iter;
}
void decrement() { --iter; }
```

facade 模式对于创建符合某些特定接口的新类型特别有用。新类型只需要向外观公开一些少量的核心操作（对于我们的迭代器外观来说，在三到六个之间），并且 facade 使用 CRTP 和 Barton-Nackman 技巧的组合来负责提供完整且正确的公共接口。

## Mixins

简单的 Polygon 类，由一系列 Point 组成:

```c++
class Point {
public:
  double x, y;
  Point() : x(0.0), y(0.0) {}
  Point(double x, double y) : x(x), y(y) {}
};
class Polygon {
private:
  std::vector<Point> points;

public:
  ... // public operations
};
```

若用户可以扩展与每个 Point 关联的信息，包括应用程序特定的数据，比如：每个 Point 的颜色，或者可能将标签与每个 Point 相关联。可以根据 Point 的类型参数化 Polygon，使这个扩展成为可能:

```c++
template <typename P> class Polygon {
private:
  std::vector<P> points;

public:
  ... // public operations
};
```

用户可以使用继承创建自己的 Point 数据类型，提供与 Point 相同的接口，但包含其他应用程序特定的数据:

```c++
class LabeledPoint : public Point {
public:
  std::string label;
  LabeledPoint() : Point(), label("") {}
  LabeledPoint(double x, double y) : Point(x, y), label("") {}
};
```

实现的缺点：需要向用户公开 Point，并且需要提供和 Label 完全相同的接口（如构造函数）。于是可以使用混合类：

混合类本质上颠倒了继承的方向，因为新类作为类模板的基类“混合”到继承层次中，而不是作为新的派生类创建。这种方法允许引入新的数据成员和其他操作，而不需要复制任何接口。

```c++
template <typename... Mixins> class Point : public Mixins... {
public:
  double x, y;
  Point() : Mixins()..., x(0.0), y(0.0) {}
  Point(double x, double y) : Mixins()..., x(x), y(y) {}
}
```

现在，可以“混合”一个包含标签的基类来生成一个 LabeledPoint，或者混合几个基类：

```c++
class Label {
public:
  std::string label;
  Label() : label("") {}
};
using LabeledPoint = Point<Label>;

class Color {
public:
  unsigned char red = 0, green = 0, blue = 0;
};
using MyPoint = Point<Label, Color>;
```

有了这个基于混合的 Point，在不改变接口的情况下，可以很容易地向 Point 引入其他信息，所以 Polygon 很容易使用和扩展。用户只需要将特化的 Point 隐式转换到自定义混合类 (上面的 Label或 Color)，就可以访问该数据或接口。此外，Point 类可以隐藏，并混合提供给 Polygon 类模板本身:

```c++
template<typename... Mixins>
class Polygon
{
private:
std::vector<Point<Mixins...>> points;
public:
... // public operations
};
```

**模板需要进行少量定制的情况下，混合类很有用——比如适用用户指定的数据存储内部对象——而不需要库公开和记录这些内部数据类型及其接口。** 我的理解：比如用户要存储指定类型的数据，并且对这些数据的容器有一些公共操作的接口，便可以使用混合类；用户实例化 Polygon 并且调用 Polygon 里面的公共方法操作。

### Curious Mixins

通过将混合类与第 21.2 节描述 CRTP 相结合，混合类可以更强大。每个混合实际上都是一个类模板，将与派生类的类型一起提供，从而可以对派生类进行定制。一个 CRTP-混合版本的 Point 可以这样:

```c++
template <template <typename>... Mixins> class Point : public Mixins<Point>... {
public:
  double x, y;
  Point() : Mixins<Point>()..., x(0.0), y(0.0) {}
  Point(double x, double y) : Mixins<Point>()..., x(x), y(y) {}
};
```

上面这种形式需要对要混合的每个类做更多工作，因此像 Label 和 Color 这样的类要成为类模板。但混合类可以根据混合到的派生类特定实例，调整其行为。可以将前面讨论的 ObjectCounter 模板混合到 Point 中，以计算 Polygon 创建的点的数量，并将该混合类与其他类混合在一起。

### Parameterized Virtuality

混合类还允许间接参数化派生类的其他属性，例如成员函数的虚函数。一个简单的例子展示了这种奇特的技巧：

```c++
#include <iostream>
class NotVirtual {};
class Virtual {
public:
  virtual void foo() {}
};

template <typename... Mixins> class Base1 : public Mixins... {
public:
  // the virtuality of foo() depends on its declaration
  // (if any) in the base classes Mixins...
  void foo() { std::cout << "Base::foo()" << '\n'; }
};
template <typename... Mixins> class Derived : public Base1<Mixins...> {
public:
  void foo() { std::cout << "Derived::foo()" << '\n'; }
};
void test_virtual() {
  Base1<NotVirtual> *p1 = new Derived<NotVirtual>;
  p1->foo(); // calls Base::foo()
  Base1<Virtual> *p2 = new Derived<Virtual>;
  p2->foo(); // calls Derived::foo()
}

```

这种技术可以提供一种方式来设计类模板，该模板既可用于实例化具体类，也可用于使用继承进行扩展。然而，在一些成员函数上使用虚拟性来获得一个类还不够，这个类可以作为更好的基类来实现更特化的功能。这种开发方法需要更基本的设计决策，因此设计两个不同的工具 (类或类模板架构) 通常比将全部集成到一个模板结构中更为实用。

但是我还是不懂具体的应用场景。

