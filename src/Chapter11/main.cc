#include <functional>
#include <iostream>
#include <type_traits>
#include <utility>
#include <vector>

template <typename Iter, typename Callable>
void foreach (Iter current, Iter end, Callable op) {
  while (current != end) { // as long as not reached the end
    op(*current);          // call passed operator for current element
    ++current;             // and move iterator to next element
  }
}

// a function to call:
void func(int i) { std::cout << "func() called for: " << i << '\n'; }

// a function object type (for objects that can be used as functions):
class FuncObj {
public:
  // 注：定义operator()时，通常应该将其定义为常量成员函数。
  // 否则，当框架或库期望此调用不更改传递对象的状态时，可能会出现错误
  void operator()(int i) const { // Note: const member function
    std::cout << "FuncObj::op() called for: " << i << '\n';
  }
};

void foo1() {
  std::vector<int> primes = {2, 3, 5, 7, 11, 13, 17, 19};
  // Note that when pass "func", it will decay to function pointer link &func.
  // function as callable (decays to pointer)
  foreach (primes.begin(), primes.end(), func)
    ;
  // function pointer as callable
  foreach (primes.begin(), primes.end(), &func)
    ;
  // function object as callable
  foreach (primes.begin(), primes.end(), FuncObj())
    ;
  // lambda as callable
  foreach (primes.begin(), primes.end(),
           [](int i) { std::cout << "lambda called for: " << i << '\n'; })
    ;
}

template <typename Iter, typename Callable, typename... Args>
void for_each(Iter elem, Iter end, Callable op, Args const &...args) {
  while (elem != end) {
    std::invoke(op, *elem, args...);
    elem++;
  }
}

class Bar {
public:
  Bar(int x) : x_(x) {}
  void print(int y) const { printf("Bar: %d\n", x_ + y); }

private:
  int x_;
};

// std::invoke
void foo2() {
  std::vector<Bar> bar_arr = {Bar(2), Bar(3)};
  for_each(bar_arr.begin(), bar_arr.end(), &Bar::print, 1);
}

// perfect forward for std::invoke

// auto call(...) {}
// auto is not good, because when it returns a reference, auto can't get that.

template <typename Callable, typename... Args>
decltype(auto) call(Callable &&op, Args &&...args) {
  if constexpr (std::is_same_v<std::invoke_result_t<Callable, Args...>, void>) {
    decltype(auto) ret{
        std::invoke(std::forward<Callable>(op), std::forward<Args>(args)...)};
    return ret;
  } else {
    std::invoke(std::forward<Callable>(op), std::forward<Args>(args)...);
    return;
  }
}

void foo3() {
  int x = 2, y = 3;

  std::remove_const_t<int const &> t = x; // int const&
  std::remove_const_t<std::remove_reference_t<int const &>> t2 = x; // int
  std::remove_reference_t<std::remove_const_t<int const &>> t3 = x; // int const
  std::decay_t<int const &> t4 = x;                                 // int

  // Sometimes error remove will cause undefined behaviour
  std::make_unsigned_t<int> unsigned_int;
  // std::make_unsigned_t<int const&> error_unsigned_int;

  std::add_rvalue_reference<int> r1;         // int&&
  std::add_rvalue_reference<int const> r2;   // int const &&
  std::add_rvalue_reference<int const &> r3; // int & 折叠规则

  std::cout << "Int copy assignable: "
            << std::is_copy_assignable_v<int> << " assignable: "
            << std::is_assignable_v<int, int> << " swappable: "
            << std::is_swappable_v<int> << std::endl;

  // is_assignable_v<int&, int> is OK.
  // TODO: is_swappable and is_swappable_with -- std::swap()
}

template <typename T1, typename T2,
          typename RT = std::decay_t<decltype(true ? std::declval<T1>()
                                                   : std::declval<T2>())>>
RT max(T1 t1, T2 t2) {
  return t1 > t2;
}

void foo4() {}

// Defer Evaluations
class ImcompleteClass;

template <typename T> class Foo {
public:
  // error: imcomplete class type
  // std::conditional<std::is_move_constructible_v<T>, T &&, T &> foo() {
  //   // ...
  // }

  // 这样直到真正调用 foo 的时候才会实例化，此时 T 已经是一个完整的类型了。
  template<typename S = T>
  std::conditional<std::is_move_constructible_v<S>, S &&, S &> foo() {
    // ...
  }
};

void foo5() { Foo<ImcompleteClass> foo; }

int main() {

  foo2();
  foo3();
  foo4();
  foo5();
}