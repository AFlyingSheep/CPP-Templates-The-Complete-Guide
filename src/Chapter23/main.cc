#include <iostream>

// Value Metaprogramming
#include <cstddef>
#include <iterator>
template <typename T> constexpr T sqrt(T x) {
  // handle cases where x and its square root are equal as a special case to
  // simplify the iteration criterion for larger x:
  if (x <= 1) {
    return x;
  }
  // repeatedly determine in which half of a [lo, hi] interval the square root
  // of x is located, until the interval is reduced to just one value:
  T lo = 0, hi = x;
  for (;;) {
    auto mid = (hi + lo) / 2, midSquared = mid * mid;
    if (lo + 1 >= hi || midSquared == x) {
      // mid must be the square root:
      return mid;
    }
    // continue with the higher/lower half-interval:
    if (midSquared < x) {
      lo = mid;
    } else {
      hi = mid;
    }
  }
}

// Type Metaprogramming
template <typename T> struct RemoveAllExtentsT { using Type = T; };

template <typename T, std::size_t SZ> struct RemoveAllExtentsT<T[SZ]> {
  using Type = typename RemoveAllExtentsT<T>::Type;
};

template <typename T> struct RemoveAllExtentsT<T[]> {
  using Type = typename RemoveAllExtentsT<T>::Type;
};

template <typename T>
using RemoveAllextents = typename RemoveAllExtentsT<T>::Type;

// Hybrid Metaprogramming
#include <array>

template <typename T, std::size_t N>
auto dotProduct(std::array<T, N> const &x, std::array<T, N> const &y) {
  T result{};
  for (std::size_t k = 0; k < N; ++k) {
    result += x[k] * y[k];
  }
  return result;
}

// 但是运行时会有开销，编译器可以做到循环展开。
// 我们使用模板来避免循环：

template <typename T, std::size_t N> struct DotProduct {
  static inline T result(T *a, T *b) {
    return *a * *b + DotProduct<T, N - 1>::result(a + 1, b + 1);
  }
};

template <typename T> struct DotProduct<T, 0> {
  static inline T result(T *a, T *b) { return a * b; }
};

template <typename T, std::size_t N>
auto dotProduct_2(std::array<T, N> const &x, std::array<T, N> const &y) {
  return DotProduct<T, N>::result(x.begin(), y.begin());
}

// Heterogeneous metaprogramming

template <unsigned N, unsigned D = 1> struct Ratio {
  static constexpr unsigned num = N; // numerator
  static constexpr unsigned den = D; // denominator
  using Type = Ratio<num, den>;
};

template <typename R1, typename R2> struct RatioAddImpl {
private:
  static constexpr unsigned den = R1::den * R2::den;
  static constexpr unsigned num = R1::num * R2::den + R2::num * R1::den;

public:
  typedef Ratio<num, den> Type;
};
// using declaration for convenient usage:
template <typename R1, typename R2>
using RatioAdd = typename RatioAddImpl<R1, R2>::Type;

// 定义 Duration

// duration type for values of type T with unit type U:
template <typename T, typename U = Ratio<1>> class Duration {
public:
  using ValueType = T;
  using UnitType = typename U::Type;

private:
  ValueType val;

public:
  constexpr Duration(ValueType v = 0) : val(v) {}
  constexpr ValueType value() const { return val; }
};

// 定义 duration 相加

template <typename T1, typename U1, typename T2, typename U2>
auto constexpr operator+(Duration<T1, U1> const &lhs,
                         Duration<T2, U2> const &rhs) {
  using VT = Ratio<1, RatioAdd<U1, U2>::den>;
  auto val = lhs.value() * VT::den / U1::den * U1::num +
             rhs.value() * VT::den / U2::den * U2::num;
  return Duration<decltype(val), VT>(val);
}

void test() {
  int x = 42;
  int y = 77;
  auto a = Duration<int, Ratio<1, 1000>>(x); // x milliseconds
  auto b = Duration<int, Ratio<2, 3>>(y);    // y 2/3 seconds
  auto c = a + b; // computes resulting unit type 1/3000 seconds
  // and generates run-time code for c = a*3 + b*2000

  // 打印 c 的值
  std::cout << c.value() << std::endl;
}

// recursion value metaprogramming without constexpr for function
template <std::size_t N, std::size_t L = 1, std::size_t R = N> struct Sqrt {
  static constexpr std::size_t mid = (L + R + 1) / 2;

  static constexpr std::size_t value =
      (mid * mid > N) ? Sqrt<N, L, mid - 1>::value : Sqrt<N, mid, R>::value;
};

template <std::size_t N, std::size_t M> struct Sqrt<N, M, M> {
  static constexpr std::size_t value = M;
};

int main() {
  test();
  Sqrt<16> q;
  std::cout << q.value << std::endl;
}

template <typename T> struct Foo { inline static const std::size_t value = 1; };

void func(size_t const &v) { std::cout << v << std::endl; }

void test2() { func(Foo<int>::value); }
