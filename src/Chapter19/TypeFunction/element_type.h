#pragma once

#include <list>
#include <vector>

template <typename T> struct ElementT;

template <typename T> struct ElementT<std::vector<T>> { using Type = T; };
template <typename T> struct ElementT<std::list<T>> { using Type = T; };
template <typename T, std::size_t N> struct ElementT<T[N]> { using Type = T; };
template <typename T> struct ElementT<T[]> { using Type = T; };

template <typename T> struct RemoveReferenceT { using Type = T; };
template <typename T> struct RemoveReferenceT<T &> { using Type = T; };
template <typename T> struct RemoveReferenceT<T &&> { using Type = T; };

template <typename T>
using RemoveReference = typename RemoveReferenceT<T>::Type;

template <typename T> struct RemoveConstT { using Type = T; };
template <typename T> struct RemoveConstT<T const> { using Type = T; };
template <typename T> using RemoveConst = typename RemoveConstT<T>::Type;

template <typename T> struct RemoveVolatileT { using Type = T; };
template <typename T> struct RemoveVolatileT<T volatile> { using Type = T; };
template <typename T> using RemoveVolatile = typename RemoveVolatileT<T>::Type;

template <typename T>
struct RemoveCVT : RemoveConstT<typename RemoveVolatile<T>::Type> {};
template <typename T> using RemoveCV = typename RemoveCVT<T>::Type;

// alias
template <typename T> using RemoveCVAlias = RemoveConst<RemoveVolatile<T>>;

// Decay
template <typename T> struct DecayT : RemoveCVT<T> {};

template <typename R, typename... Args> struct DecayT<R(Args..., ...)> {
  using Type = R (*)(Args..., ...);
};