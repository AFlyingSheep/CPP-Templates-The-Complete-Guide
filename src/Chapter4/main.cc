#include <array>
#include <cstddef>
#include <iostream>
#include <iterator>
#include <ostream>
#include <type_traits>
#include <vector>

template <typename T> void print(T t) { std::cout << t; }

template <typename T, typename... Args> void print(T t, Args... args) {
  // std::cout << t;
  std::cout << sizeof...(args) << " not showed.\n";
  print(args...);
}

// sizeof... number of elements a parameter pack contain

// Not work, cause each branch of if will be instantiated.
// template <typename T, typename... Args> void print2(T t, Args... args) {
//   std::cout << t;
//   if (sizeof...(args) > 0) {
//     print2(args...);
//   }
// }

// Fold expressions
// empty parameter is ill-form, expect &&(true), ||(false), ','(void())
template <typename... T> auto foldSum(T... t) { return (... + t); }

// Use Fold expressions to transform the tree
struct Node {
  int value;
  Node *left;
  Node *right;
  Node(int i = 0) : value(i), left(nullptr), right(nullptr) {}
};

template <typename T, typename... TP> Node *transform_tree(T np, TP... paths) {
  return (np->*...->*paths);
}

// Use Fold expressions to print
template <typename T> class Addspace {
public:
  Addspace(T const &t) : ref(t) {}

  friend std::ostream &operator<<(std::ostream &os, Addspace<T> s) {
    return os << s.ref << " ";
  }

private:
  T const &ref;
};

// Addspace<Types>(args) each argument creates an AddSpace object
template <typename... Types> void print_fold(Types const &...args) {
  (std::cout << ... << Addspace(args));
}

// Variadic Expression
template <typename... T> void double_print(T const &...t) { print(t + t...); }
template <typename... T> void addOne(T const &...t) { print(t + 1 ...); }
template <typename T, typename... OTHERT>
constexpr bool isTheSame(T, OTHERT...) {
  return (std::is_same_v<T, OTHERT> && ...);
}

// Variadic Indices
template <typename T, typename... Idx> void IdxPrint(T const &t, Idx... idx) {
  print(t[idx]...);
}

// Variadic Class Templates
template <typename... T> class Tuple;
template <std::size_t... Idx> class Indices {};

template <typename T, std::size_t... Idx>
void printByIdx(T const &t, Indices<Idx...>) {
  print(std::get<Idx>(t)...);
};

// Variadic deduction gudies
// deduction guide for std::arrays

// template <typename T, typename... U>
// std::array(T, U...)
//     ->std::array<std::enable_if_t<std::is_same_v<T, U> &&..., T>,
//                  (1 + sizeof...(U))>;

int main() {
  print(1, 2, 4, "haha");
  // print2(1, 2, 4, "haha");

  auto x = foldSum(1);

  // transform the tree
  Node n1(1), n2(2), n3(3);
  n3.left = &n1;
  n3.right = &n2;
  transform_tree(&n3, &Node::left, &Node::right);

  print_fold(1, 2);

  std::cout << isTheSame("A", "apple") << " " << isTheSame(1, "Apple");
  return 0;
}