#pragma once

#include <limits>
#include <type_traits>
template <bool b, typename TrueType, typename FalseType> struct IfThenElseT {
  using Type = TrueType;
};

template <typename TrueType, typename FalseType>
struct IfThenElseT<false, TrueType, FalseType> {
  using Type = FalseType;
};

template <bool b, typename TrueType, typename FalseType>
using IfThenElse = typename IfThenElseT<b, TrueType, FalseType>::Type;

template <auto N> struct SmallestIntT {
  using Type = typename IfThenElseT<
      N <= std::numeric_limits<char>::max(), char,
      typename IfThenElseT<
          N <= std::numeric_limits<short>::max(), short,
          typename IfThenElseT<
              N <= std::numeric_limits<int>::max(), int,
              typename IfThenElseT<
                  N <= std::numeric_limits<long>::max(), long,
                  typename IfThenElseT<
                      N <= std::numeric_limits<long long>::max(),
                      long long, // then
                      void       // fallback
                      >::Type>::Type>::Type>::Type>::Type;
};

// yield T when using member Type:
template <typename T> struct IdentityT { using Type = T; };
// to make unsigned after IfThenElse was evaluated:
template <typename T> struct MakeUnsignedT {
  using Type = typename std::make_unsigned<T>::type;
};
template <typename T> struct UnsignedT {
  using Type = typename IfThenElse<std::is_integral<T>::value &&
                                       !std::is_same<T, bool>::value,
                                   MakeUnsignedT<T>, IdentityT<T>>::Type;
};

void test() { UnsignedT<bool>::Type f; }

