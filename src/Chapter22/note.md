现在可以有两种方式使用多态：**静态多态（通过模板）和动态多态（通过虚继承和虚函数）**。

然而有问题：静态多态在运行时所使用的类型集合需要在编译期已知；另一方面，通过继承的动态多态性允许单一版本的多态函数处理编译时未知的类型，但它的灵活性较差，因为类型必须从公共基类继承。

# Function Objects, Pointers, and std::function<>

Function objects are useful for providing customizable behavior to templates.

比如可以使用如下代码：

```c++
template<typename T>
void forUpTo(int n, F f) { ... }
```

这个模板可以与任何函数对象一起用，包括 lambda，函数指针以及合适的 operator() 或者转换为函数指针、引用。

但是当模板太大时，实例化可能会让代码体积过大。

为了解决问题，将模板转换为非模板，首先使用的方法是使用 function pointer 来完成：

```c++
void forUpTo(int n, void (*f)(int)) { ... }
```

然而依旧存在问题：lambda 无法转换为函数指针。标准库提供 std::function 来完成上述转换：

```c++
void forUpTo(int n, std::function<void(int)> f) { ... }
```

# 通用函数指针

std::function 提供了通用的函数指针:

- 它可以调用函数而无需知道函数本身；

- 可以被复制、移动和赋值；

- 可以从另外一个 std::function 初始化；

- 可以有一个空状态，代表没有绑定任何函数。

std::function 相较于函数指针来说，它可以额外存储 lambda 对象或带有 operator() 操作符的函数对象。

```c++
void fun(int x, std::function<void(int)> f) { f(x); }

class Foo {
public:
  void operator()(int x) { printf("x: %d\n", x); }

private:
  int y;
};

int main() {
  int y;
  fun(3, [&y](int x) { printf("x: %d\n", x); });
  fun(4, Foo());
}
```

## 实现 FunctionPtr

见代码：function_ptr/function_ptr.h & function_bridge.h。
