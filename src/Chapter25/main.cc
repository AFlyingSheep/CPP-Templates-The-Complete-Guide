#include "tuple"
#include "tuple.h"
#include <string>

int main() {
  Tuple<int, double, std::string> t(1, 2.0, "3");
  printf("Construct.\n");
  Tuple<long, long double, std::string> t2(t);

  std::cout << t2 << std::endl;

  // Test push front
  Tuple<int, int, double, std::string> t3 = pushFront(t, 0);
  std::cout << t3 << std::endl;

  Tuple<int, double, std::string> t1(17, 3.14, "Hello, World!");
  auto t4 = pushBack(t1, true);
  auto t5 = popFront(t4);
  std::cout << std::boolalpha << t5 << '\n';

  std::cout << reverse(makeTuple(1, 2.5, std::string("hello"))) << std::endl;

  Tuple<double, double> t6;
  std::cout << sizeof(t6) << std::endl;

  auto tt = makeTuple(0, '1', 2.2f, std::string{"hello"});
  auto a = tt[CTValue<unsigned, 2>{}];
  auto b = tt[CTValue<unsigned, 3>{}];

  std::cout << a << " " << b << std::endl;
}
