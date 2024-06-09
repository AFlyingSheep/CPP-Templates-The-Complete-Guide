在本章中，我们主要处理违反时会导致编译错误的约束，我们将这些约束称为语法约束。语法约束可以包括需要存在某种构造函数、需要明确的特定函数调用等等。

另一种约束我们称为语义约束。这些约束很难通过机械方式验证。在一般情况下，这样做甚至可能不切实际。例如，我们可能要求在模板类型参数上定义一个 < 运算符（这是一种语法约束），但通常我们还要求该运算符在其域上实际定义某种排序（这是一种语义）约束）。

术语 concept 通常用于表示模板库中重复需要的一组约束。例如，C++ 标准库依赖于随机访问迭代器和默认构造函数等概念。有了这个术语，我们可以说调试模板代码包括大量确定模板实现及其使用中如何违反概念的工作。

# Shallow Instantiation

当模板错误发生时，通常会在一长串实例化之后发现问题，从而导致冗长的错误消息.为了说明这一点，请考虑以下人为构造的错误代码：

```c++
template <typename T> void clear(T &p) {
  *p = 0; // assumes T is a pointer-like type
}
template <typename T> void core(T &p) { clear(p); }
template <typename T> void middle(typename T::Index p) { core(p); }
template <typename T> void shell(T const &env) {
  typename T::Index i;
  middle<T>(i);
}
```

此示例说明了软件开发的典型分层：像 shell() 这样的高级函数模板依赖于 middle() 这样的组件，这些组件本身使用 core() 这样的基本设施。当我们实例化 shell() 时，它下面的所有层也需要被实例化。在这个例子中，最深层暴露了一个问题：core()用 int 类型实例化（来自在middle()中使用Client::Index）并尝试取消引用该类型的值，这是一个错误。

该错误只能在实例化时检测到：

```c++
class Client {
public:
  using Index = int;
};
int main() {
  Client mainClient;
  shell(mainClient);
}
```

在前面的示例中，我们可以在 shell() 中添加尝试取消引用 T::Index 类型的值的代码。例如：

```c++
template <typename T> void ignore(T const &) {}
template <typename T> void shell(T const &env) {

  class ShallowChecks {
    void deref(typename T::Index p) { ignore(*p); }
  };

  typename T::Index i;
  middle<T>(i);
}
```


如果 T 是 T::Index 无法取消引用的类型，则现在会在本地类 ShallowChecks 上诊断出错误。请注意，由于本地类并未实际使用，因此添加的代码不会影响 shell() 函数的运行时间。不幸的是，许多编译器会警告未使用ShallowChecks（及其成员也未使用）。可以使用诸如使用ignore()模板之类的技巧来抑制此类警告，但它们会增加代码的复杂性。

显然，我们示例中的虚拟代码的开发可能变得与实现模板实际功能的代码一样复杂。为了控制这种复杂性，尝试在某种库中收集各种虚拟代码片段是很自然的。例如，这样的库可以包含扩展为代码的宏，当模板参数替换违反该特定参数背后的概念时，该代码会触发适当的错误。最流行的此类库是 Concept Check Library，它是 Boost 发行版的一部分。

不幸的是，该技术不是特别可移植（不同编译器诊断错误的方式有很大不同），有时会掩盖无法在更高级别捕获的问题。一旦我们有了 C++ 中的概念（参见附录 E），我们就有其他方法来支持需求和预期行为的定义。

# Static Assertions

C++ static_assert 关键字是随 C++11 引入的，具有相同的目的，但在编译时进行评估：如果条件（必须是常量表达式）评估为 false，编译器将发出错误消息。该错误消息将包含一个字符串（它是 static_assert 本身的一部分），向程序员指示出了什么问题。

我们可以使用 type traits 来完成类型的检测：

```c++
#include <type_traits> // for true_type and false_type
#include <utility>     // for declval()

template <typename T> class HasDereference {
private:
  template <typename U> struct Identity;
  template <typename U>
  static std::true_type test(Identity<decltype(*std::declval<U>())> *);
  template <typename U> static std::false_type test(...);

public:
  static constexpr bool value = decltype(test<T>(nullptr))::value;
};

template <typename T> void shell2(T const &env) {
  static_assert(HasDereference<T>::value,
                "Assert failed: T must have dereference operator");
  typename T::Index i;
  middle<T>(i);
}
```

这样编译器可以输出更加简洁的诊断信息。

# Archetypes

编写模板时，确保模板定义能够针对满足该模板指定约束的任何模板参数进行编译是一项挑战。考虑一个简单的 find() 算法，该算法在数组中查找值及其记录的约束：

```c++
// T must be EqualityComparable, meaning:
// two objects of type T can be compared with == and the result converted to
// bool
template <typename T> int find(T const *array, int n, T const &value) {
  int i = 0;
  while (i != n && array[i] != value)
    ++i;
  return i;
}
```

此模板定义存在两个问题，当给定某些在技术上满足模板要求但行为与模板作者预期略有不同的模板参数时，这两个问题都会表现为编译错误。我们将使用原型的概念来根据 find() 模板指定的要求来测试我们的实现对其模板参数的使用。

原型是用户定义的类，**可用作模板参数来测试模板定义是否遵守其对其相应模板参数施加的约束**。原型是专门为以尽可能最少的方式满足模板的要求而设计的，**而不提供任何无关的操作**。如果使用原型作为模板参数的模板定义实例化成功，那么我们就知道模板定义不会尝试使用模板未明确要求的任何操作。

例如，这是一个原型，旨在满足 find() 算法文档中描述的 EqualityComparable 概念的要求：

```c++
class EqualityComparableArchetype {};
class ConvertibleToBoolArchetype {
public:
  operator bool() const;
};
ConvertibleToBoolArchetype operator==(EqualityComparableArchetype const &,
                                      EqualityComparableArchetype const &);
```

EqualityComparableArchetype 没有成员函数或数据，它提供的唯一操作是重载运算符 == 以满足 find() 的相等要求。该运算符 == 本身相当小，返回另一个原型 ConvertibleToBoolArchetype，其唯一定义的操作是用户定义的 bool 转换。

EqualityComparableArchetype 显然满足 find() 模板的规定要求，因此我们可以通过尝试使用 EqualityComparableArchetype 实例化 find() 来检查 find() 的实现是否额外使用了其他的运算符：

```c++
template int find(EqualityComparableArchetype const *, int,
                  EqualityComparableArchetype const &);
```

find<EqualityComparableArchetype> 的实例化将会失败，这表明我们已经找到了第一个问题：EqualityComparable 描述只需要 ==，但 find() 的实现依赖于将 T 对象与 != 进行比较。我们的实现适用于大多数用户定义类型，这些类型将 == 和 != 作为一对实现，但实际上是不正确的。原型旨在在模板库开发的早期发现此类问题。


更改 find() 的实现以使用相等而不是不等解决了第一个问题，并且 find() 模板将使用原型成功编译。

使用原型揭示 find() 中的第二个问题需要更多的独创性。请注意，find() 的新定义现在应用 !运算符直接返回 == 的结果。就我们的原型而言，这依赖于用户定义的 bool 转换和内置逻辑否定运算符！。更仔细地实现 ConvertibleToBoolArchetype 毒药运算符！以免其被不当使用：

```c++
class ConvertibleToBoolArchetype {
public:
  operator bool() const;
  bool operator!() = delete;
};
```

我们可以进一步扩展这个原型，使用删除的函数(删除函数是像普通函数一样参与重载决策的函数。但是，如果通过重载决策选择它们，编译器会产生错误。)来毒害运算符 && 和 ||帮助查找其他模板定义中的问题。通常，模板实现者希望为模板库中标识的每个概念开发一个原型，然后使用这些原型根据其规定的要求测试每个模板定义。

# Tracers

到目前为止，我们已经讨论了编译或链接包含模板的程序时出现的错误。然而，确保程序在运行时正确运行的最具挑战性的任务通常是在成功构建之后。模板有时会使此任务变得更加困难，因为模板表示的通用代码的行为唯一取决于该模板的客户端（当然比普通的类和函数更重要）。跟踪器是一种软件设备，可以通过在开发周期的早期检测模板定义中的问题来减轻调试方面的问题。

跟踪器是一个用户定义的类，可以用作要测试的模板的参数。通常，跟踪器也是一种原型，只是为了满足模板的要求而编写的。更重要的是，跟踪器应该生成对其调用的操作的跟踪。例如，这可以通过实验验证算法的效率以及操作顺序。

用于测试排序算法的跟踪器示例如 tracer.h 所示。这个特殊的跟踪器允许我们跟踪实体创建和销毁的模式以及给定模板执行的分配和比较。以下测试程序针对 C++ 标准库的 std::sort() 算法说明了这一点，如 main.cc 所示。

部分输出信息：

```text
std::sort() of 10 SortTracer’s was performed by:
 9 temporary tracers
 up to 11 tracers at the same time (10 before)
 33 assignments
 27 comparisons
```

例如，我们看到虽然在排序时我们的程序中创建了 9 个临时跟踪器，但同时最多存在两个附加跟踪器。因此，我们的跟踪器履行了两个角色：它证明标准 sort() 算法不需要比我们的跟踪器更多的功能（例如，不需要运算符 == 和 >），并且它让我们了解算法的成本。然而，它并没有透露太多有关排序模板的正确性的信息。

# Oracles

跟踪器相对简单且有效，但它们允许我们仅针对特定输入数据及其相关功能的特定行为来跟踪模板的执行。例如，我们可能想知道比较运算符必须满足哪些条件才能使排序算法有意义（或正确），但在我们的示例中，我们仅测试了一个与整数的小于完全相同的比较运算符。

跟踪器的扩展在某些圈子中被称为 Oracle（或运行时分析 oracle）。它们是连接到推理引擎的跟踪器，推理引擎是一个可以记住断言及其原因以推断某些结论的程序。

在某些情况下，预言机允许我们动态验证模板算法，而无需完全指定替换模板参数（预言机是参数）或输入数据（推理引擎在卡住时可能会请求某种输入假设）。然而，以这种方式分析的算法的复杂性仍然适中（由于推理引擎的限制），并且工作量相当大。由于这些原因，我们不深入研究预言机的发展，但感兴趣的读者应该检查后注中提到的出版物（以及其中包含的参考文献）。
