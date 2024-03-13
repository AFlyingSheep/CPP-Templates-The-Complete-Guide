#include <cstring>
#include <iostream>
#include <type_traits>

template <typename T, typename F>
auto max(T a, F b) -> std::decay_t<decltype(a > b ? a : b)> {
  if (a > b)
    return a;
  return b;
}

// note: common_type also decays.
template <class T, class F> std::common_type<T, F> max2(T a, F b) {
  if (a > b)
    return a;
  return b;
}

template <class T, class F, class RT = std::common_type<T, F>>
RT max3(T a, F b) {
  if (a > b)
    return a;
  return b;
}

template <class RT = long, class T, class F> RT max4(T a, F b) {
  if (a > b)
    return a;
  return b;
}

// an error caused by reference
template <typename T> T const &max5(T const &a, T const &b) {
  return b < a ? a : b;
}

char const *max5(char const *a, char const *b) {
  return std::strcmp(b, a) < 0 ? a : b;
}

template <typename T> T const &max5(T const &a, T const &b, T const &c) {
  // error: return reference to local temporary object!
  return max(a, max(b, c));
}

int main() {
  int a;
  std::cin >> a;
  max(a, 4.2);

  char const *s1 = "apple";
  char const *s2 = "banana";
  char const *s3 = "pear";
  auto s4 = max5(s1, s2, s3);
  std::cout << s4;
  return 0;
}