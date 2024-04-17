#include <iostream>

template <typename T>
concept GetDraw = requires(T x) {
  { x.draw() } -> std::same_as<void>;
  { x.run() } -> std::same_as<int>;
};

// concept 语法：{expression} [nonexcept] -> 类型约束
// expression 合法
// 如果有 nonexcept，需要要求 expression no throw
// decltype(expression) 是否满足类型约束

template <typename T>
requires GetDraw<T>
void draw(T const &t) { t.draw(); }

class FooClass {
public:
  void draw() const {}
  int run() const { return 1; }
};

void foo1() { draw(FooClass()); }

int main() {}