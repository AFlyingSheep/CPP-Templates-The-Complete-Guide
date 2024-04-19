#include <iterator>
#include <type_traits>
#include <vector>

template <typename... T> struct FunctionOverload;

template <> struct FunctionOverload<> { static void func(...); };

template <typename T, typename... Args>
struct FunctionOverload<T, Args...> : public FunctionOverload<Args...> {
  static T func(T);
  using FunctionOverload<Args...>::func;
};

template <typename T, typename... Types> struct BestMatchInSetT {
  using Type = decltype(FunctionOverload<Types...>::func(std::declval<T>()));
};

template <typename T, typename... Types>
using BestMatchInSet = typename BestMatchInSetT<T, Types...>::Type;

// primary template (intentionally undefined):
template <typename Iterator,
          typename Tag = BestMatchInSet<
              typename std::iterator_traits<Iterator>::iterator_category,
              std::input_iterator_tag, std::bidirectional_iterator_tag,
              std::random_access_iterator_tag>>
class Advance;

// general, linear-time implementation for input iterators:
template <typename Iterator> class Advance<Iterator, std::input_iterator_tag> {
public:
  using DifferenceType =
      typename std::iterator_traits<Iterator>::difference_type;
  void operator()(Iterator &x, DifferenceType n) const {
    while (n > 0) {
      ++x;
      --n;
    }
  }
};
// bidirectional, linear-time algorithm for bidirectional iterators:
template <typename Iterator>
class Advance<Iterator, std::bidirectional_iterator_tag> {
public:
  using DifferenceType =
      typename std::iterator_traits<Iterator>::difference_type;
  void operator()(Iterator &x, DifferenceType n) const {
    if (n > 0) {
      while (n > 0) {
        ++x;
        --n;
      }
    } else {
      while (n < 0) {
        --x;
        ++n;
      }
    }
  }
};
// bidirectional, constant-time algorithm for random access iterators:
template <typename Iterator>
class Advance<Iterator, std::random_access_iterator_tag> {
public:
  using DifferenceType =
      typename std::iterator_traits<Iterator>::difference_type;
  void operator()(Iterator &x, DifferenceType n) const { x += n; }
};

void test_advance() {
  std::vector<int> vec;
  Advance<decltype(vec.begin())> advance;
}