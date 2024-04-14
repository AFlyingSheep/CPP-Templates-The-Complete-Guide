#include <array>
#include <iostream>
#include <optional>
#include <variant>

void test2();
void test3();

// std::variant & std::visit 的应用场景
// 1. 配置管理：配置中可能包含多种选项，可以用 std::map<std::string,
// std::variant<int, double, std::string>> 存储，并且使用 visit
// 对不同的类型采取不同的处理方式。

// 2. 状态机：使用枚举类型 + variant + visit
// std::variant<IdleState, RunningState, ErrorState> currentState;

int main() {
  // 类似于 union
  std::variant<int, double> x, y;

  x = 1.2;
  std::cout << sizeof(x) << std::endl;
  std::cout << "x - " << x.index() << std::endl;
  std::cout << "y - " << y.index() << std::endl;

  int z = std::get<1>(x);
  std::cout << z << "\n";

  // Optional
  std::optional<int> option = 4;
  option.value_or(3);
  if (option.has_value()) {
    printf("Has value.\n");
  } else {
    printf("No value.\n");
  }

  std::cout << option.value() << std::endl;

  test2();
  test3();
}

template <class... Ts> struct overloaded : Ts... {
  // variadic template参数展开支持using表达式, 是C++17才支持的特性
  using Ts::operator()...; //继承父类所有operator() 操作符
};

// 自定义模板推导
template <class... Ts> overloaded(Ts...) -> overloaded<Ts...>;

// 使用 visit 优雅的访问 variant
void test2() {
  std::variant<int, double> x;
  x = 3.0;
  std::visit(overloaded{[](double a) -> void { printf("Call double.\n"); },
                        [](int a) -> void { printf("Call int.\n"); }},
             x);
}

// 同样可以使用 visit 实现特殊的业务函数，如比较、转换等。
template <typename T> struct CompareVisitor {
  template <typename U> T operator()(U &u) const {
    // 或其他的转换逻辑
    return static_cast<T>(u);
  }

  T operator()(T &&t) const { return std::move(t); }
  T operator()(const T &t) { return t; }
};

void test3() {
  std::variant<long, double> x;
  const CompareVisitor<long> cvisitor;
  x = 3.2;

  std::cout << std::visit(cvisitor, x) << std::endl;

}