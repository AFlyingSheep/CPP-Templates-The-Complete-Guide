#include <cstddef>
#include <iostream>

// In c++11 constexpr can only contain one return statement.

// p: number to check, d: current divisor
constexpr bool doIsPrime(unsigned p, unsigned d) {
  return d != 2 ? (p % d != 0) &&
                      doIsPrime(p, d - 1) // check this and smaller divisors
                : (p % 2 != 0);           // end recursion if divisor is 2
}
constexpr bool isPrime_11(unsigned p) {
  return p < 4 ? !(p < 2)             // handle special cases
               : doIsPrime(p, p / 2); // start recursion with divisor from p/2
}

// In c++14, there is no limit.
constexpr bool isPrime_14(unsigned int p) {
  for (unsigned int d = 2; d <= p / 2; ++d) {
    if (p % d == 0) {
      return false; // found divisor without remainder
    }
  }
  return p > 1; // no divisor without remainder found
}

// Whether funtion will compute in compile time,
//  otherwise the call is left as a run-time call instead.

// We can get different implement

// default implement
template <size_t T, bool b = isPrime_14(T)> struct Helper {};
template <size_t T> struct Helper<T, true> {};

// but funtion doesn't support partial specialization
// We can use class static funtion, enable_if or SFINAE to solve it.

// SFINAE
//  std::thread has a constructor:
/*
template<typename F, typename... Args>
explicit thread(F&& f, Args&&... args);

But this constructor shall not participate in overload resolution if decay_t<F>
is the same type as std::thread.

The reason is that otherwise a member template like this sometimes might better
match than any predefined copy or move constructor.

So we use enable_if like this:
template<typename F, typename... Args,
typename = std::enable_if_t<!std::is_same_v<std::decay_t<F>,
thread>>>
explicit thread(F&& f, Args&&... args);
*/

// To solve situation like this
class Foo {
public:
  using size_type = int;
  size_type size() const { return 2; }
};
// template <typename T> typename T::size_type len(T const &t) { return
// t.size(); }
template <typename T>
auto len(T const &t) -> decltype(t.size(), typename T::size_type()) {
  return t.size();
}
void test_decltype() {
  std::allocator<int> x;
  Foo f;
  // Error! no member named size
  // std::cout << len(x) << std::endl;
  std::cout << len(f) << std::endl;
}

// Compile-time if (since C++17)
template <size_t SZ> void func() {
  if constexpr (isPrime_14(SZ)) {
    // ...
  } else {
    // ...
  }
}

template <typename T, typename... Args> void print_args(T &&t, Args &&...args) {
  std::cout << t;
  if constexpr (sizeof...(args) >= 1) {
    print_args(args...);
  }
}

int main() { print_args(1, 2, "Apple"); }