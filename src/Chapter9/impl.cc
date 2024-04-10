#include "main.h"

// template void f<int>();
template <typename T> void f() { printf("Haha!\n"); }

// 显式实例化（不能有函数体）
template void f<int>();

// same as a origin function
// 部分特化->完全特化
template <> void f<int>() { printf("Haha1!\n"); }

// 显式实例化的目的是为了具现化模板，不可以提供新的函数体，而不是为了某一个特殊类提供一个新的实现；
// 部分特化或完全特化的目的便是为了针对某一种实现使用新的处理措施。

// 如果在完全特化 int 以后再声明显式实例化
// int，这时候显式实例化将会被忽略，因为总是会匹配到特化实现上。
//* 感觉一般把显式实例化放在最后