#include <algorithm>
#include <iostream>
#include <vector>

template <int val, typename T> T add_value(T t) { return t + val; }

// For nontype template paramenters, they can be only constant integral values
// (including enumerations), pointers to objects/functions/members, lvalue
// references to objects or functions, or std::nullptr_t

template <double *VAL> double Foo(double t) { return t + *VAL; }

class Bar {};

template <Bar &bar> void fun() {}
template <Bar *bar> void fun() {}

const double *mt;

char x1[] = "saaaa";       //全局变量
char *x2 = "qweqeq";       //全局变量
const char *x3 = "qweqeq"; //全局变量 指针常量

template <typename T, char *x> void string_test(T t) {
  std::cout << t << ", " << x << std::endl;
};

// 未完待续
void string_test() {
  string_test<int, x1>(3); // 这是那个例外
  // string_test<int, x2>(4); // 错误： 应为编译时常量表达式

  // 错误：非const的全局指针! &
  // 涉及带有内部链接的对象的表达式不能用作非类型参数
  // string_test<int, x3>(5);

  char *x4 = "adsas";

  //  错误：包含非静态存储持续时间的变量不能用作非类型参数
  // string_test<int, x4>(6);

  //错误：字符串，浮点型即使是常量表达式也不可以作为非类型实参
  // string_test<int, "sdfsd">(7);
}

int main() {
  std::vector<int> v1;
  std::vector<int> v2(5);
  for (int i = 0; i < 5; i++) {
    v1.push_back(i);
  }
  std::transform(v1.begin(), v1.end(), v2.begin(), add_value<5, int>);

  for (int i = 0; i < 5; i++) {
    std::cout << v2[i];
  }

  // static double cv = 2;
  // static double *ptr = &cv;
  // double v = Foo<cv>(3.2);

  return 0;
}