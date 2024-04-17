#pragma once

// member template
class SumPolicy {
public:
  template <typename T1, typename T2>
  static void acc(T1 &total, T2 const &right) {
    total += right;
  }
};

template <typename T1, typename T2> class SumPolicy2 {
public:
  static void acc(T1 &total, T2 const &right) { total += right; }
};