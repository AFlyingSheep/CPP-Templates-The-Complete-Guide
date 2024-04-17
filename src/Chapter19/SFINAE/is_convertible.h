#include <iostream>
#include <type_traits>

template <typename FROM, typename TO,
          bool = std::is_array_v<TO> || std::is_function_v<TO> ||
                 std::is_void_v<TO>>
class IsConvertibleHelper {
public:
  using Type =
      std::integral_constant<bool, std::is_void_v<TO> || std::is_void_v<FROM>>;
};

template <typename FROM, typename TO>
class IsConvertibleHelper<FROM, TO, false> {
private:
  static void aux(TO);
  template <typename U = FROM, typename = decltype(aux(std::declval<U>()))>
  static std::true_type test(void *);

  template <typename> static std::false_type test(...);

public:
  using Type = decltype(test<FROM>(nullptr));
};

template <typename FROM, typename TO>
using IsConvertibleT = typename IsConvertibleHelper<FROM, TO>::Type;

template <typename FROM, typename TO>
static constexpr auto isConvertible = IsConvertibleT<FROM, TO>::value;

class Fooo {};

void test_conver() { std::cout << isConvertible<int *, double *> << std::endl; }