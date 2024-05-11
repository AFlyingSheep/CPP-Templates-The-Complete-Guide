### Argument-Dependent Lookup of Friend Declarations

友元函数声明可以是指定函数的第一个声明。如果是这种情况，则假定该函数是在最近的命名空间范围（可能是全局范围）中声明的，其中包含包含友元声明的类。然而，这样的友元声明在该范围内并不直接可见。考虑下面的例子:

```c++
template <typename T> class C {
  //... 
  friend void f();

  friend void f(C<T> const &);
  //...
};
void g(C<int> *p) {
  f();   // is f() visible here?
  f(*p); // is f(C<int> const&) visible here?
}
```

如果在 g() 中，f() 可见，说明 friend 声明在命名空间中可见。可以发现，f() 发生了编译错误！

另一方面，仅在友元声明中声明（和定义）函数可能很有用（在[CRTP](../Chapter21/note.md)）。当它们作为友元的类属于 ADL 考虑的关联类时，可以找到这样的函数。

考虑例子，f() 没有关联任何的命名空间或类，因为他没有任何参数，所以在例子中这是一个无效调用；但是 f(*p) 关联了类 C\<int\>，因此第二个 f() 可以在关联的类中找到声明。C\<int\> 需要被完全实例化。



