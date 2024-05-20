#include "../Chapter24/type_list.h"
#include <cstdlib>
#include <iostream>
#include <new> // for std::launder()
#include <type_traits>

template <typename... Types> class VariantStorage {
  using LargestT = LargestType<Typelist<Types...>>;
  alignas(Types...) unsigned char buffer[sizeof(LargestT)];
  unsigned char discriminator = 0;

public:
  unsigned char getDiscriminator() const { return discriminator; }
  void setDiscriminator(unsigned char d) { discriminator = d; }
  void *getRawBuffer() { return buffer; }
  const void *getRawBuffer() const { return buffer; }
  template <typename T> T *getBufferAs() {
    return std::launder(reinterpret_cast<T *>(buffer));
  }
  template <typename T> T const *getBufferAs() const {
    return std::launder(reinterpret_cast<T const *>(buffer));
  }
};

// FindIndexof
template <typename List, typename T, unsigned N = 0,
          bool Empty = IsEmpty<List>::value>
struct FindIndexOfT;
template <typename List, typename T, unsigned N>
struct FindIndexOfT<List, T, N, false>
    : public std::conditional_t<std::is_same_v<Front<List>, T>,
                                std::integral_constant<unsigned, N>,
                                FindIndexOfT<PopFront<List>, T, N + 1>> {};

template <typename List, typename T, unsigned N>
struct FindIndexOfT<List, T, N, true> {};

// Variant Choice
template <typename... Types> class Variant;
template <typename T, typename... Types> class VariantChoice {
  using Derived = Variant<Types...>;
  Derived &getDerived() { return *static_cast<Derived *>(this); }
  Derived const &getDerived() const {
    return *static_cast<Derived const *>(this);
  }

protected:
  // compute the discriminator to be used for this type
  constexpr static unsigned Discriminator =
      FindIndexOfT<Typelist<Types...>, T>::value + 1;

public:
  VariantChoice() {}
  VariantChoice(T const &value); // see variantchoiceinit.hpp
  VariantChoice(T &&value);      // see variantchoiceinit.hpp
  bool destroy() {
    if (getDerived().getDiscriminator() == Discriminator) {
      getDerived().template getBufferAs<T>()->~T();
    }
  }
  Derived &operator=(T const &value); // see variantchoiceassign.hpp
  Derived &operator=(T &&value);      // see variantchoiceassign.hpp
};

template <typename T, typename... Types>
VariantChoice<T, Types...>::VariantChoice(T const &value) {
  // place value in buffer and set type discriminator:
  new (getDerived().getRawBuffer()) T(value);
  getDerived().setDiscriminator(Discriminator);
}

template <typename T, typename... Types>
VariantChoice<T, Types...>::VariantChoice(T &&value) {
  // place moved value in buffer and set type discriminator:
  new (getDerived().getRawBuffer()) T(std::move(value));
  getDerived().setDiscriminator(Discriminator);
}

template <typename T, typename... Types>
auto VariantChoice<T, Types...>::operator=(T const &value) -> Derived & {
  if (getDerived().getDiscriminator() == Discriminator) {
    // assign new value of same type:
    *getDerived().template getBufferAs<T>() = value;
  } else {
    // assign new value of different type:
    getDerived().destroy();                     // try destroy() for all types
    new (getDerived().getRawBuffer()) T(value); // place new value
    getDerived().setDiscriminator(Discriminator);
  }
  return getDerived();
}

template <typename T, typename... Types>
auto VariantChoice<T, Types...>::operator=(T &&value) -> Derived & {
  if (getDerived().getDiscriminator() == Discriminator) {
    // assign new value of same type:
    *getDerived().template getBufferAs<T>() = std::move(value);
  } else {
    // assign new value of different type:
    getDerived().destroy(); // try destroy() for all types
    new (getDerived().getRawBuffer()) T(std::move(value)); // place new value
    getDerived().setDiscriminator(Discriminator);
  }
  return getDerived();
}

// Variant
template <typename... Types>
class Variant : private VariantStorage<Types...>,
                private VariantChoice<Types, Types...>... {
  template <typename T, typename... OtherTypes> friend class VariantChoice;

public:
  template <typename T> bool is() const {
    return this->getDiscriminator() ==
           VariantChoice<T, Types...>::Discriminator;
  }
  template <typename T> T &get() & {
    if (empty()) {
      // ... throw exception
    }
    static_assert(is<T>());
    return *this->template getBufferAs<T>();
  }
  template <typename T> T const &get() const & {
    if (empty()) {
      // ... throw exception
    }
    static_assert(is<T>());
    return *this->template getBufferAs<T>();
  }
  template <typename T> T &&get() && {
    if (empty()) {
      // ... throw exception
    }
    static_assert(is<T>());
    return std::move(*this->template getBufferAs<T>());
  }

  // see variantvisit.hpp:
  template <typename R = ComputedResultType, typename Visitor>
  VisitResult<R, Visitor, Types &...> visit(Visitor &&vis) &;
  template <typename R = ComputedResultType, typename Visitor>
  VisitResult<R, Visitor, Types const &...> visit(Visitor &&vis) const &;
  template <typename R = ComputedResultType, typename Visitor>
  VisitResult<R, Visitor, Types &&...> visit(Visitor &&vis) &&;
  using VariantChoice<Types, Types...>::VariantChoice...;
  Variant();                      // see variantdefaultctor.hpp
  Variant(Variant const &source); // see variantcopyctor.hpp
  Variant(Variant &&source);      // see variantmovector.hpp
  template <typename... SourceTypes>
  Variant(Variant<SourceTypes...> const &source); // variantcopyctortmpl.hpp
  template <typename... SourceTypes> Variant(Variant<SourceTypes...> &&source);
  using VariantChoice<Types, Types...>::operator=...;
  Variant &operator=(Variant const &source); // see variantcopyassign.hpp
  Variant &operator=(Variant &&source);
  template <typename... SourceTypes>
  Variant &operator=(Variant<SourceTypes...> const &source);
  template <typename... SourceTypes>
  Variant &operator=(Variant<SourceTypes...> &&source);
  bool empty() const;
  ~Variant() { destroy(); }

  void destroy();
};

template <typename... Types> void Variant<Types...>::destroy() {
  bool results = {VariantChoice<Types, Types...>::destroy()...};
  this->setDiscriminator(0);
}
