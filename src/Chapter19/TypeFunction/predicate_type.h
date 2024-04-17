#pragma once

template <bool val> struct BoolConstant {
  using Type = BoolConstant<val>;
  static constexpr bool value = val;
};
using TrueType = BoolConstant<true>;
using FalseType = BoolConstant<false>;

template <typename T1, typename T2> struct IsSameT : FalseType {};
template <typename T> struct IsSameT<T, T> : TrueType {};

