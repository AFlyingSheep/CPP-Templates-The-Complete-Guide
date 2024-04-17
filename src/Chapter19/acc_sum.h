#pragma once
#include "accumulation_traits.h"
#include "policy.h"

template <typename T, typename AT = AccumulationTraits<T>,
          typename Policy = SumPolicy,
          template <typename, typename> class Policy_2 = SumPolicy2>
auto accmulate(T const *start, T const *end) {
  using ResType = typename AT::AccT;
  ResType res = AT::zero();

  while (start != end) {
    // res += *start;
    Policy::acc(res, *start); // member templates
    Policy_2<ResType, T>::acc(res, *start);
    ++start;
  }
  return res;
}
