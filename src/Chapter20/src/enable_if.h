#pragma once

#include <iostream>
#include <type_traits>

template <typename T> class Foo1 {
public:
  template <typename U = T, typename = std::enable_if_t<std::is_array_v<U>>>
  Foo1() {
    printf("Array.\n");
  }
  template <typename U = T, typename = std::enable_if_t<!std::is_array_v<U>>,
            typename = void>
  Foo1() {
    printf("Not Array.\n");
  }
};

void test() { Foo1<int[]> foo; }