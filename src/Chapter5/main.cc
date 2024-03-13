#include <array>
#include <bitset>
#include <cstddef>
#include <deque>
#include <iostream>
#include <iterator>
#include <memory>
#include <string>
#include <vector>

// 1. where to use typename?
//  a. name that depends on a template paramater is a type
namespace n1 {
template <typename T> void foo() {
  // if not, compiler doesn't know whether value is a pointer
  // or type * value.
  typename T::type *value;
}

//  b. iterators of standard containers
template <typename T> void bar(const T &t) {
  // because STL defines const_interator: using const_interator = ...
  // so we should add typename before it.
  typename T::const_iterator pos;
  typename T::const_iterator end = t.end();

  for (pos = t.begin(); pos != end; pos++) {
    std::cout << *pos << " ";
  }
}
} // namespace n1

// 2. zero-initialization
namespace n2 {
class Bar {
public:
  Bar() { printf("Bar come!\n"); }
  Bar(int x) { printf("Bar come with %d!\n", x); }
};

template <typename T> class Foo {
  T t{1}; // Make sure T is be initialized(build-in type) or construct.
};

template <typename T> void foo2(T t = T{}) {
  // ...
}
} // namespace n2

// 3. Using this->
namespace n3 {
template <typename T> class Base {
public:
  void bar() {}
};

template <typename T> class Derived : public Base<T> {
public:
  void foo() {
    // error
    // bar();
    // right
    this->bar();
    Base<T>::bar();
  }
};
} // namespace n3

// 4. Templates for Raw Arrays and String Literals
//    if pass by value, type will be decayed.
//    But if pass by ref, arg don't be decayed.
namespace n4 {
template <typename T, std::size_t M, std::size_t N>
bool less(const T (&A)[M], const T (&B)[N]) {
  for (int i = 0; i < M && i < N; i++) {
    if (A[i] < B[i])
      return true;
    if (A[i] > B[i])
      return false;
  }
  return N < M;
}
} // namespace n4

// 5. Member templates
namespace n5 {
// See Stack.
} // namespace n5

// Special Member Function Templates
// (1) .template construct
//    if we not have template, compiler doesn't know bs.to_string < ... or a
//    template.
template <unsigned long N> void printBitset(std::bitset<N> const &bs) {
  std::cout << bs.template to_string<char, std::char_traits<char>,
                                     std::allocator<char>>();
}

// (2) lambda is better. Skip.

// 6. Variable templates

// Note:  A variable template is a variable that is a template (variable is a
// noun here). A variadic template is a template for a variadic number of
// template parameters (variadic is an adjective here).

// So we can use it by pi<T>, pi<>. but just use pi is error.
template <typename T = long double> constexpr T pi{3.141592653589};

template <std::size_t N> std::array<int, N> arr{};

template <auto N> constexpr decltype(N) dval = N;

// Variable Templates for Data Members
template <typename T> class Myclass {
public:
  static constexpr int max = 100;
};
template <typename T> int myMax = Myclass<T>::max;

// 7. template template paramenters
// Before c++17,  exactly match the parameters of the template template
// parameter it substitutes
// So add a default typename = std::allocator
template <typename T, template <class Elem, typename = std::allocator<Elem>>
                      class Cont = std::deque>
class Foo {
public:
  Cont<T> elems_;
};

int main() {
  std::vector<int> foo_v(10, 5);
  n1::bar(foo_v);

  n2::Foo<n2::Bar> foo;

  int a[] = {1, 2, 3};
  int b[] = {4, 5, 6};
  n4::less(a, b);

  arr<100>[0] = 42;
  arr<50>[0] = 30;

  Foo<int, std::vector> foo1;

  printf("arr_100: %p, arr_50: %p\n", &arr<100>, &arr<50>);
  return 0;
}