#include <cstring>
#include <string>
#include <vector>

template <typename T> class Stack {
public:
  Stack() = default;
  Stack(T t) { elems.push_back(t); }

  void push(T t) { elems.push_back(t); }

private:
  std::vector<T> elems;
};

// Deduction Guides
Stack(const char *)->Stack<std::string>;

// Note: templates can only be declared and defined in global/namespace scope or
// inside class declarations

int main() {
  std::string s1 = "apple";
  Stack s = {"Hello"};

  s.push(s1);
}