#include <cstdio>
#include <functional>
#include <iostream>

#include "function_ptr/function_ptr.h"

void print(int x) { printf("x: %d\n", x); }
void print2(int x) { printf("x: %d\n", x); }

void fun(int x, FunctionPtr<void(int)> f) { f(x); }

void fun(FunctionPtr<void(int)> f1, FunctionPtr<void(int)> f2) {
  std::cout << (f1 == f2);
}

class Foo {
public:
  void operator()(int x) { printf("x: %d\n", x); }

private:
};

int main() {
  int y = 5;
  fun(3, [&y](int x) { printf("x + y: %d\n", x + y); });
  fun(3, print);
  printf("Print: %p\n", print);
  fun(print, print2);
}