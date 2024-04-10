# 模板类和类模板

1. 类模板指的是一个模板，而模板类则是类模板实例化以后的类。
  如 `template<typename T> class Foo;` 是一个类模板，而 `class Foo<int>` 就是一个模板类。

# Substitution, Instantiation, and Specialization

- Instantiation：实例化
- Specialization：特化

# Declarations versus Definitions

定义与声明，注意使用 extern 修饰的没有初始化器的变量为声明，否则为定义。

如：`extern int v;` 是一个声明，`extern int v = 3;` 是一个定义。

## Imcomplete type

不完整的类型为下面的一种：

1. 声明但未定义的类；（`class C;`）
2. 没有明确边界的数组；（`extern int arr[]`）
3. 不完整类类型的数组；（`extern C elems[10]`）
4. void
5. 枚举值或底层类型没有被定义的枚举类型；
6. 应用 const 和/或 volatile 的任何上述类型（？）

# 单一定义规则（ODR）

1. 普通（即非模板）非内联函数和成员函数，以及（非内联）全局变量和静态数据成员应该在整个程序中仅定义一次；
2. 类类型（包括结构体和联合）、模板（包括部分特化但不完全特化）以及内联函数和变量每个翻译单元最多定义一次，并且所有这些定义应该相同。