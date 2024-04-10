#include "main.h"
#include <cstddef>

// 有关于模板特例化
// 不要特例化函数模板，而是使用函数重载

template <typename T> // (1) - 主模板
std::string getTypeName(T) {
  return "unknown";
}
template <typename T> // (2) - 重载自 (1) 的主模板
std::string getTypeName(T *) {
  return "pointer";
}
template <> // (3) - (2) 的显式特例化
std::string getTypeName(int *) {
  return "int pointer";
}

// getTypeName2
template <typename T> // (4) - 主模板
std::string getTypeName2(T) {
  return "unknown";
}
template <> // (5) - (4) 的显式特例化
std::string getTypeName2(int *) {
  return "int pointer";
}
template <typename T> // (6) - 重载自 (4) 的主模板
std::string getTypeName2(T *) {
  return "pointer";
}

void test() {
  std::cout << '\n';
  int *p = nullptr;

  std::cout << "getTypeName(p): " << getTypeName(p) << '\n';
  std::cout << "getTypeName2(p): " << getTypeName2(p) << '\n';
  std::cout << '\n';

  // Output:
  // getTypeName(p): int pointer
  // getTypeName2(p): pointer

  // 注意：函数模板的实例化不参与重载！所以不要使用函数模板实例化，要用普通函数。
}

template <typename T, typename F> void foo() {}
// 函数部分实例化是不允许的！
// template <typename T> void foo<int>() {}

int main() {
  f<int>();
  // f<char>();

  test();
}
