#include <algorithm>
#include <functional>
#include <iostream>
#include <ostream>
#include <string>
#include <type_traits>
#include <utility>

// When passing arguments to parameters by reference, they do not decay.

// Call parameter is declared as T const&, the template parameter T itself is
// not deduced as const.

template <typename T> void printR(T const &arg) { std::cout << arg; }

// For non-constant reference, temporary (prvalue) or an existing object passed
// with std::move() (xvalue) usually is not allowed

// However, when users called non-constant reference by const value with
// template, funtion modified the param will cause wrong. Such as this:
template <typename T> void wrong_example(T &arg) { arg += arg; }
template <typename T, std::enable_if<!std::is_const_v<T>>>
void wrong_example_new(T &arg) {
  arg += arg;
}
template <typename T>
requires(!std::is_const_v<T>) void wrong_example_new(T &arg) { arg += arg; }

void test() {
  // Error: prvalue.
  // wrong_example(std::string("Hello\n"));

  // Error: std::move
  std::string str = "Apple";
  // wrong_example(std::move(str));

  // Error: const value with funtion modified.
  const std::string str_const = "Banana";
  // wrong_example(str_const);

  // To solve this question, we can check whether param is const.
  // wrong_example_new(str_const);
}

// std::ref and std::cref
void printString(std::string const &s) { std::cout << s << '\n'; }

template <typename T> void printT(T arg) {
  // wrapper will be converted to std::string.
  printString(arg);
}

void test_ref() {
  std::string str = "Apple";
  // cref use a wrapper of ref
  printT(std::cref(str));
  printT(std::ref(str));
}

template <typename T> class Foo {
public:
  friend std::ostream &operator<<(std::ostream &out, Foo) {
    out << std::endl;
    return out;
  }
};

// std::ref and cref work only if you pass objects through generic code.
// condition below is error.
template <typename T> void printV(T arg) { std::cout << arg << '\n'; }
void test_ref_2() {
  std::string s = "hello";
  // printV(s); // OK

  // ERROR: no operator << for reference wrapper defined
  // printV(std::cref(s));

  int x = 2;        // Right
  auto y = "hello"; // Right
  Foo<int> foo;

  // Error: template don't know what the type of foo when compiling.
  // So in the printV, arg will be std::reference_wrapper
  // But it doesn't have operator <<.

  // But operator << override <<(int), so when std::ref(x), and call
  // std::cout << arg << "\n", x will be convert into int so there
  // is no error.

  printV(std::cref(x));
}
// All in all, use std::ref and cref only if you pass objects through generic
// code.
// You always finally need a conversion back to the underlying type

// =============================================================

// consider string literals and raw arrays

// only valid for arrays.
template <typename T, std::size_t L1, std::size_t L2>
void foo(T (&arg1)[L1], T (&arg2)[L2]) {
  T *pa = arg1; // decay arg1
  T *pb = arg2; // decay arg2
  // if (compareArrays(pa, L1, pb, L2)) {
  //   // ...
  // }
}

// use type trait to detect whether array is passed.
template <typename T, typename = std::enable_if_t<std::is_array_v<T>>>
void foo(T &&arg1, T &&arg2) {
  // ...
}

// =============================================================
// when should we return reference?
// • Returning elements of containers or strings (e.g., by operator[] or
// front())
// • Granting write access to class members
// • Returning objects for chained calls (operator<< and operator>> for streams
// and operator=for class objects in general)

// grant read access to return const references.

// We should ensure that function templates return their result by value.
// But sometimes, T will be deduced as a reference...

// To be safe:
template <typename T> typename std::remove_reference<T>::type retV(T p) {
  return T{}; // always returns by value
}

// Or use auto to let compiler deduce the type, because auto always decays.
template <typename T> auto retV(T p) {
  return T{}; // always returns by value
}

// =============================================================

// to std::pair
// in C++98
template <typename P1, typename P2>
std::pair<P1, P2> make_pair_98(P1 const &a, P2 const &b) {
  return std::pair<P1, P2>(a, b);
}

// in C++03
template <typename P1, typename P2> std::pair<P1, P2> make_pair_03(P1 a, P2 b) {
  return std::pair<P1, P2>(a, b);
}

template <typename P1, typename P2>
constexpr std::pair<typename std::decay<P1>::type,
                    typename std::decay<P2>::type>
make_pair_11(P1 &&a, P2 &&b) {
  return std::pair<typename std::decay<P1>::type,
                   typename std::decay<P2>::type>(std::forward<P1>(a),
                                                  std::forward<P2>(b));
}

void test_pair() {
  // Error in 98: can't init array with an array.
  // auto p1 = make_pair_98(1, "abc");

  // Okay. Array decay.
  auto p2 = make_pair_03("abc", "bcd");
  auto p3 = make_pair_11("abc", "bcd");
}

int main() {
  // T -> char[8]
  // So param: char const&[8]
  printR("Hello.\n");
  test_ref_2();
}