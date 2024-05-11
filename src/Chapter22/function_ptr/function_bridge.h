#include <iostream>
#include <memory>
#include <utility>

template <typename R, typename... Args> class FunctorBridge {
public:
  virtual ~FunctorBridge() {}
  virtual FunctorBridge *clone() const = 0;

  // 将 invoke() 设为 const，为的是避免通过 const 函数 ptr 对象调用非 const
  // 操作符 () 重载，这与开发者的预期不符。
  virtual R invoke(Args... args) const = 0;

  // 判断是否调用同一个函数
  virtual bool equals(const FunctorBridge *other) const = 0;
};

template <typename Functor, typename R, typename... Args>
class SpecificFunctorBridge : public FunctorBridge<R, Args...> {
  Functor functor;

public:
  template <typename FunctorFwd>
  SpecificFunctorBridge(FunctorFwd &&ifunctor)
      : functor(std::forward<FunctorFwd>(ifunctor)) {}

  virtual FunctorBridge<R, Args...> *clone() const override {
    return new SpecificFunctorBridge(functor);
  }

  virtual R invoke(Args... args) const override {
    return functor(std::forward<Args>(args)...);
  }

  virtual bool equals(FunctorBridge<R, Args...> const *fb) const override {
    auto specFb =
        dynamic_cast<SpecificFunctorBridge<Functor, R, Args...> const *>(fb);

    if (specFb) {
      // return functor == specFb->functor;
    }
    return false;
  }

  // 但是上述实现存在缺点，如果 bridge 不存在合适的 == 运算符，比如
  // lambda，那么将会出现编译错误。那么我们可以在 function_ptr 比较的时候 check
  // 类型的 == 运算符是否存在。

  // 可以通过 SNIFAE 技术完成。
};