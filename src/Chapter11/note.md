# Generic Libraries

## Callables

许多库包含客户端代码向其传递必须“调用”的实体的接口，比如线程调度的操作需要传入一个函数。

callback：回调函数，作为参数传递的实体（而不是模板参数）。在 C++ 中适合作为回调函数的类型：

1. Pointer-to-function types

2. Class types with an overloaded operator() (sometimes called functors), including lambdas

3. Class types with a conversion function yielding a pointer-to-function or reference-to-function

这些类型称为函数对象类型，其值称为函数对象。

举例如 main.cc 中的 foo1 函数所示。

针对成员函数无法使用 funtion() 调用的问题，C++17 中的 invoke 可以解决。如 main.cc 的 foo2 函数所示。std::invoke 的参数中，第一个参数为 callable 函数，如果该函数是成员函数，第二个参数为类的实例，后面为参数。否则剩下的都是参数。

## Other Utilities to Implement Generic Libraries

### Type Traits

在某些时候，Type Traits 往往和想象中的不同，如 foo3() 所示。

### std::declval()

std::declval() 的主要作用是虚拟构建出一个类的实例，获取实例的类型。这在类没有默认构造函数的时候很好用。

总结来说，declval 就是专门治那些无法实例化具体对象的场合的。

example:

```c++
using foo_t = decltype(std::declval<A>().foo());
using deref_t = decltype(std::declval<X>() + std::declval<Y>());
```

在纯虚基类上有时候元编程会比较麻烦，这时候可能可以借助 declval 来避开纯虚基类不能实例化的问题。

下面介绍了一些延伸和 tricks：

1. 采用一个普通的抽象类作为基类：如果基类的代码、数据很多，可能会导致膨胀问题。一个解决方法是采用一个普通的基类，并在其基础上建立模板化的基类：

```c++
struct base {
  virtual ~base_t(){}
  
  void operation() { do_sth(); }
  
  protected:
  virtual void do_sth() = 0;
};

template <class T>
  struct base_t: public base{
    protected:
    virtual void another() = 0;
  };

template <class T, class C=std::list<T>>
  struct vec_style: public base_t<T> {
    protected:
    void do_sth() override {}
    void another() override {}
    
    private:
    C _container{};
  };
```

这样的写法，可以将通用逻辑（不必泛型化的）抽出到 base 中，避免留在 base_t 中随着泛型实例化而膨胀。

2. 运行时多态，在 SomethingOther 里面。

如 foo4() 所示。

## Perfect Forwarding Temporaries

如果想要实现非模板参数的完美转发，可以使用 auto&& 保存中间变量，如：

```c++
template<typename T>
void foo(T x) {
  auto &&val = get(x);
  // ....

  set(std::forward<decltype(val)>(val));
}
```

这样便可以避免中间值的无关副本。

## References as Template Parameters

注意模板可能会被推导成引用，从而发生一些未定义行为。尽量在推导时使用 auto 而不是 decltype(auto)。

```c++
template<typename T, decltype(auto) SZ> class Arr; // BAD
template<typename T, auto SZ> class Arr; // OK
```

## Defer Evaluations

有些类模板可以使用不完整的类型，如仅使用其指针等情况。但有时类模板内对类的特殊用法会导致失去使用不完整类型的能力。解决方法为将成员函数替换为成员模板，如 foo5() 所示。