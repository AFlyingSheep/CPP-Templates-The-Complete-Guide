#include <iostream>
#include <type_traits>
#include <utility>

class X {
public:
  X() = default;
};

void foo(X &x) { printf("Call X&\n"); }
void foo(X &&x) { printf("Call X&&\n"); }

// Note that T&& and X&& are different!
template <typename T> void bar(T &&t) { foo(std::forward<T>(t)); }

void bar2(X &&x) { foo(x); }

//

#include <iostream>
#include <string>
#include <utility>
class Person {
private:
  std::string name;

public:
  // generic constructor for passed initial name:
  template <typename STR, typename = std::enable_if_t<
                              std::is_convertible_v<STR, std::string>>>
  explicit Person(STR &&n) : name(std::forward<STR>(n)) {
    std::cout << "TMPL-CONSTR for " << name << "\n";
  }
  // copy and move constructor:
  Person(Person const &p) : name(p.name) {
    std::cout << "COPY-CONSTR Person " << name << "\n";
  }
  Person(Person &&p) : name(std::move(p.name)) {
    std::cout << "MOVE-CONSTR Person " << name << "\n";
  }
};

void test() {
  std::string name = "Young";
  Person p1(name);
  Person p2("Yang");

  // Error! Call MPL-CONSTR because it's better than Person const& p.
  Person p3(p2);
}

// Note that member function templates never count as special member functions
// and are ignored

// We can declare a copy constructor for const volatile arguments and mark it
// “deleted” (i.e., define it with = delete). Doing so prevents another copy
// constructor from being implicitly declared.

class Base {};

class Foo : public Base {
public:
  Foo() = default;
  Foo(Foo const &) { std::cout << "Default copy run." << std::endl; }
  template <typename T> Foo(T const &t) {
    std::cout << "Template copy run." << std::endl;
  }
};

void test2() {
  Foo f1;
  Base b;
  Foo f2(f1);
  Foo f3(b);
}

// enable if is too clumsy, so we use concept
template <typename T>
concept ConvertibleToString = std::is_convertible_v<T, std::string>;

// So class Person's copy construction also can like this:
class Person2 {
public:
  // generic constructor for passed initial name:
  template <typename STR>
  requires ConvertibleToString<STR>
  explicit Person2(STR &&n) : name(std::forward<STR>(n)) {
    std::cout << "TMPL-CONSTR for " << name << "\n";
  }
  // ...
private:
  std::string name;
};

// or template<ConvertibleToString STR>.

int main() {
  X x;
  // bar(X());
  // bar(x);

  // error!
  // bar2(x);

  test();
  test2();

  return 0;
}