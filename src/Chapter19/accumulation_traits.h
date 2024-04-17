class BigInt;

template <typename T> struct AccumulationTraits;

template <> struct AccumulationTraits<char> {
  using AccT = int;
  static constexpr AccT zero() { return 0; }
};
template <> struct AccumulationTraits<short> {
  using AccT = int;
  static constexpr AccT zero() { return 0; }
};
template <> struct AccumulationTraits<int> {
  using AccT = long;
  static constexpr AccT zero() { return 0; }
};
template <> struct AccumulationTraits<unsigned int> {
  using AccT = unsigned long;
  static constexpr AccT zero() { return 0; }
};
template <> struct AccumulationTraits<float> {
  using AccT = double;
  static constexpr AccT zero() { return 0; }
};

template <> struct AccumulationTraits<BigInt> {
  using AccT = BigInt;
  // static BigInt zero() { return BigInt{0}; }
};

// 针对 zero，对于 literal type，可以直接使用 static const int/enum 或 static
// constexpr float ...（因为C++仅允许类内初始化整型或枚举类型）

// 而对于用户定义的非 literal type，上述方法只能通过类外初始化，这使得代码变得冗余。

// 一种解决方法是：inline static BigInt const zero = BigInt{0}; // since c++17

// 另一种解决方法是用 static 函数返回，对于 literal type 表明为 constexpr，如代码所示。