#include <iostream>

template <typename T> void clear(T &p) {
  *p = 0; // assumes T is a pointer-like type
}
template <typename T> void core(T &p) { clear(p); }
template <typename T> void middle(typename T::Index p) { core(p); }

template <typename T> void ignore(T const &) {}
template <typename T> void shell(T const &env) {

  class ShallowChecks {
    void deref(typename T::Index p) { ignore(*p); }
  };

  typename T::Index i;
  middle<T>(i);
}

class Client {
public:
  using Index = int;
};

#include <type_traits> // for true_type and false_type
#include <utility>     // for declval()

template <typename T> class HasDereference {
private:
  template <typename U> struct Identity;
  template <typename U>
  static std::true_type test(Identity<decltype(*std::declval<U>())> *);
  template <typename U> static std::false_type test(...);

public:
  static constexpr bool value = decltype(test<T>(nullptr))::value;
};

template <typename T> void shell2(T const &env) {
  static_assert(HasDereference<T>::value,
                "Assert failed: T must have dereference operator");
  typename T::Index i;
  middle<T>(i);
}

// T must be EqualityComparable, meaning:
// two objects of type T can be compared with == and the result converted to
// bool
template <typename T> int find(T const *array, int n, T const &value) {
  int i = 0;
  while (i != n && !(array[i] == value))
    ++i;
  return i;
}

class EqualityComparableArchetype {};
class ConvertibleToBoolArchetype {
public:
  operator bool() const;
  // bool operator!() = delete;
};
ConvertibleToBoolArchetype operator==(EqualityComparableArchetype const &,
                                      EqualityComparableArchetype const &);

// template int find(EqualityComparableArchetype const *, int,
//                   EqualityComparableArchetype const &);

#include "tracer.h"
#include <algorithm>
#include <iostream>
int main() {
  // prepare sample input:
  SortTracer input[] = {7, 3, 5, 6, 4, 2, 0, 1, 9, 8};
  // print initial values:
  for (int i = 0; i < 10; ++i) {
    std::cerr << input[i].val() << ' ';
  }
  std::cerr << '\n';
  // remember initial conditions:
  long created_at_start = SortTracer::creations();
  long max_live_at_start = SortTracer::max_live();

  long assigned_at_start = SortTracer::assignments();
  long compared_at_start = SortTracer::comparisons();
  // execute algorithm:
  std::cerr << "---[ Start std::sort() ]--------------------\n";
  std::sort<>(&input[0], &input[9] + 1);
  std::cerr << "---[ End std::sort() ]----------------------\n";
  // verify result:
  for (int i = 0; i < 10; ++i) {
    std::cerr << input[i].val() << ' ';
  }
  std::cerr << "\n\n";
  // final report:
  std::cerr << "std::sort() of 10 SortTracer’s"
            << " was performed by:\n "
            << SortTracer::creations() - created_at_start
            << " temporary tracers\n "
            << "up to " << SortTracer::max_live()
            << " tracers at the same time (" << max_live_at_start
            << " before)\n " << SortTracer::assignments() - assigned_at_start
            << " assignments\n "
            << SortTracer::comparisons() - compared_at_start
            << " comparisons\n\n";
}

// int main() {
//   Client mainClient;
//   // shell(mainClient);
// }
