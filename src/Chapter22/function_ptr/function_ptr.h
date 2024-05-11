#include "function_bridge.h"
#include <type_traits>
#include <utility>

template <typename R, typename... Args> class FunctionPtr;

template <typename R, typename... Args> class FunctionPtr<R(Args...)> {
private:
  FunctorBridge<R, Args...> *bridge;

public:
  // constructors:
  FunctionPtr() : bridge(nullptr) {}
  FunctionPtr(FunctionPtr const &other) : bridge(nullptr) {
    if (other.bridge)
      bridge = other.bridge->clone();
  }
  FunctionPtr(FunctionPtr &other)
      : FunctionPtr(static_cast<FunctionPtr const &>(other)) {}
  FunctionPtr(FunctionPtr &&other) : bridge(other.bridge) {
    other.bridge = nullptr;
  }

  // 从任意对象进行构造
  template <typename F> FunctionPtr(F &&f) : bridge(nullptr) {
    using FunctionType = std::decay_t<F>;
    using Bridge = SpecificFunctorBridge<FunctionType, R, Args...>;
    bridge = new Bridge(std::forward<F>(f));
  }

  // assignment operators:
  FunctionPtr &operator=(FunctionPtr const &other) {
    FunctionPtr tmp(other);
    swap(*this, tmp);
    return *this;
  }
  FunctionPtr &operator=(FunctionPtr &&other) {
    delete bridge;
    bridge = other.bridge;
    other.bridge = nullptr;
    return *this;
  }

  // construction and assignment from arbitrary function objects:
  template <typename F> FunctionPtr &operator=(F &&f) {
    FunctionPtr tmp(std::forward<F>(f));
    swap(*this, tmp);
    return *this;
  }
  // destructor:
  ~FunctionPtr() { delete bridge; }

  friend void swap(FunctionPtr &fp1, FunctionPtr &fp2) {
    std::swap(fp1.bridge, fp2.bridge);
  }
  explicit operator bool() const {
    return bridge != nullptr;
  }
  // invocation:
  R operator()(Args... args) const {
    return bridge->invoke(std::forward<Args>(args)...);
  }

  // check whether call the same function
  friend bool operator==(FunctionPtr const &f1, FunctionPtr const &f2) {
    if (!f1 || !f2) {
      return !f1 && !f2;
    }
    return f1.bridge->equals(f2.bridge);
  }

  friend bool operator!=(FunctionPtr const &f1, FunctionPtr const &f2) {
    return !(f1 == f2);
  }
};