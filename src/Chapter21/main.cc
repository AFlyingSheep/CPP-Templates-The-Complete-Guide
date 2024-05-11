#include <iostream>
#include <utility>

template <typename Derive> class Base {
public:
  friend bool operator!=(const Derive &d1, const Derive &d2) {
    printf("Call base.\n");
    return !(d1 == d2);
  }
};

class Foo : Base<Foo> {
public:
  Foo(int x) : x_(x) {}

  friend bool operator==(const Foo &f1, const Foo &f2) {
    printf("Call derived.\n");
    return !(f1.x_ == f2.x_);
  }

private:
  int x_;
};

int main() {
  Foo f1(1), f2(2);
  std::cout << (f1 != f2) << std::endl;
}

#include <iostream>
class NotVirtual {};
class Virtual {
public:
  virtual void foo() {}
};

template <typename... Mixins> class Base1 : public Mixins... {
public:
  // the virtuality of foo() depends on its declaration
  // (if any) in the base classes Mixins...
  void foo() { std::cout << "Base::foo()" << '\n'; }
};
template <typename... Mixins> class Derived : public Base1<Mixins...> {
public:
  void foo() { std::cout << "Derived::foo()" << '\n'; }
};
void test_virtual() {
  Base1<NotVirtual> *p1 = new Derived<NotVirtual>;
  p1->foo(); // calls Base::foo()
  Base1<Virtual> *p2 = new Derived<Virtual>;
  p2->foo(); // calls Derived::foo()
}
