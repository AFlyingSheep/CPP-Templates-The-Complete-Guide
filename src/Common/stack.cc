#include <deque>
#include <iostream>
#include <memory>
#include <ostream>
#include <type_traits>
#include <vector>

template <typename T, template <typename Elem, typename = std::allocator<Elem>>
                      typename Container = std::vector>
class Stack {
public:
  Stack() = default;
  Stack(T t) { elems_.push_back(t); }

  bool empty() const { return elems_.empty(); }
  void pop() { return elems_.pop_back(); }
  T const &top() { return elems_.at(0); }
  template <typename T2,
            template <typename Elem, typename = std::allocator<Elem>>
            typename Container2>
  Stack<T, Container> &operator=(Stack<T2, Container2> &stack);

  void check_value() {
    for (auto iter = elems_.begin(); iter != elems_.end(); iter++) {
      std::cout << *iter << " ";
    }
    std::cout << std::endl;
  }

  template <typename, template <typename, typename> class> friend class Stack;

private:
  Container<T> elems_;
};
template <typename T, template <typename Elem, typename = std::allocator<Elem>>
                      typename Container>
template <typename T2, template <typename Elem, typename = std::allocator<Elem>>
                       typename Container2>
Stack<T, Container> &
Stack<T, Container>::operator=(Stack<T2, Container2> &stack) {
  // if not friend class
  Stack<T2, Container2> tmp(stack);
  elems_.clear();
  while (!tmp.empty()) {
    elems_.push_back(tmp.top());
    tmp.pop();
  }

  // if friend class
  elems_.clear();
  elems_.insert(elems_.begin(), stack.elems_.begin(), stack.elems_.end());

  return *this;
}

int main() {
  Stack<int> intStack(2);
  Stack<double> doubleStack(3.0);

  doubleStack = intStack;
  doubleStack.check_value();
}