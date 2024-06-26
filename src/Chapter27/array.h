#include "props.h"
#include "sarray.h"

template <typename T, typename Rep = SArray<T>> class Array {
private:
  Rep expr_rep; // (access to) the data of the array
public:
  // create array with initial size
  explicit Array(std::size_t s) : expr_rep(s) {}
  // create array from possible representation
  Array(Rep const &rb) : expr_rep(rb) {}
  // assignment operator for same type
  Array &operator=(Array const &b) {
    assert(size() == b.size());
    for (std::size_t idx = 0; idx < b.size(); ++idx) {
      expr_rep[idx] = b[idx];
    }
    return *this;
  }

  // assignment operator for arrays of different type
  template <typename T2, typename Rep2>
  Array &operator=(Array<T2, Rep2> const &b) {
    assert(size() == b.size());
    for (std::size_t idx = 0; idx < b.size(); ++idx) {
      expr_rep[idx] = b[idx];
    }
    return *this;
  }
  // size is size of represented data
  std::size_t size() const { return expr_rep.size(); }
  // index operator for constants and variables
  decltype(auto) operator[](std::size_t idx) const {
    assert(idx < size());
    return expr_rep[idx];
  }
  T &operator[](std::size_t idx) {
    assert(idx < size());
    return expr_rep[idx];
  }
  // return what the array currently represents
  Rep const &rep() const { return expr_rep; }
  Rep &rep() { return expr_rep; }
};

// addition of two Arrays:
template <typename T, typename R1, typename R2>
Array<T, A_Add<T, R1, R2>> operator+(Array<T, R1> const &a,
                                     Array<T, R2> const &b) {
  return Array<T, A_Add<T, R1, R2>>(A_Add<T, R1, R2>(a.rep(), b.rep()));
}
// multiplication of two Arrays:
template <typename T, typename R1, typename R2>
Array<T, A_Mult<T, R1, R2>> operator*(Array<T, R1> const &a,
                                      Array<T, R2> const &b) {
  return Array<T, A_Mult<T, R1, R2>>(A_Mult<T, R1, R2>(a.rep(), b.rep()));
}
// multiplication of scalar and Array:
template <typename T, typename R2>
Array<T, A_Mult<T, A_Scalar<T>, R2>> operator*(T const &s,
                                               Array<T, R2> const &b) {
  return Array<T, A_Mult<T, A_Scalar<T>, R2>>(
      A_Mult<T, A_Scalar<T>, R2>(A_Scalar<T>(s), b.rep()));
}
