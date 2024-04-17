#include "SFINAE/SFINAE.h"
#include "acc_sum.h"
#include "named_template_args.h"
#include <iostream>

#include <iostream>
#include <type_traits>
#include <typeinfo>

template <typename T> void f(T) {}
template <typename A> void printParameterType(void (*)(A)) {
  std::cout << "Parameter type: " << typeid(A).name() << '\n';
  std::cout << "- is int: " << std::is_same<A, int>::value << '\n';
  std::cout << "- is const: " << std::is_const<A>::value << '\n';
  std::cout << "- is pointer: " << std::is_pointer<A>::value << '\n';
}

template <typename T> T &&my_declval() noexcept;

class A {
public:
  A() {}
  static void execute(){};

private:
  ~A() {}
};

int main() {
  int arr[5] = {1, 2, 3, 4, 5};
  std::cout << accmulate(arr, arr + 5) << std::endl;

  BreadSlicer_sec<Policy3_is<CustomPolicy>>::execute();

  printParameterType(&f<int>);
  printParameterType(&f<int const>);
  printParameterType(&f<int[7]>);
  printParameterType(&f<int(int)>);

  // decltype(my_declval<A>()) m;
  test2();
}
