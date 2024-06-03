#include <cassert>
#include <cstddef>
#include <iostream>

template <typename T> class A_Scalar;
// primary template
template <typename T> class A_Traits {
public:
  using ExprRef = T const &; // type to refer to is constant reference
};
// partial specialization for scalars
template <typename T> class A_Traits<A_Scalar<T>> {
public:
  using ExprRef = A_Scalar<T>; // type to refer to is ordinary value
};

template <typename T, typename OP1, typename OP2> class A_Add {
private:
  typename A_Traits<OP1>::ExprRef op1; // first operand
  typename A_Traits<OP2>::ExprRef op2; // second operand
public:
  // constructor initializes references to operands
  A_Add(OP1 const &a, OP2 const &b) : op1(a), op2(b) {}
  // compute sum when value requested
  T operator[](std::size_t idx) const { return op1[idx] + op2[idx]; }
  // size is maximum size
  std::size_t size() const {
    assert(op1.size() == 0 || op2.size() == 0 || op1.size() == op2.size());
    return op1.size() != 0 ? op1.size() : op2.size();
  }
};

// class for objects that represent the multiplication of two operands
template <typename T, typename OP1, typename OP2> class A_Mult {
private:
  typename A_Traits<OP1>::ExprRef op1; // first operand
  typename A_Traits<OP2>::ExprRef op2; // second operand
public:
  // constructor initializes references to operands
  A_Mult(OP1 const &a, OP2 const &b) : op1(a), op2(b) {}
  // compute product when value requested
  T operator[](std::size_t idx) const { return op1[idx] * op2[idx]; }
  // size is maximum size
  std::size_t size() const {
    assert(op1.size() == 0 || op2.size() == 0 || op1.size() == op2.size());
    return op1.size() != 0 ? op1.size() : op2.size();
  }
};

// class for objects that represent scalars:
template <typename T> class A_Scalar {
private:
  T const &s; // value of the scalar
public:
  // constructor initializes value
  constexpr A_Scalar(T const &v) : s(v) {}
  // for index operations, the scalar is the value of each element
  constexpr T const &operator[](std::size_t) const { return s; }
  // scalars have zero as size
  constexpr std::size_t size() const { return 0; };
};
