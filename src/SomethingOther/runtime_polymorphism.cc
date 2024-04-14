#include <iostream>
#include <memory>
#include <string>
#include <vector>

// 运行时多态，其特点是基类不是基类，基类的嵌套类才是基类：Animal::Interface
// 才是用于类体系的抽象基类，它是纯虚的，但却不影响 std::vector<Animal>
// 的有效编译与工作。Animal 使用简单的转接技术将 Animal::Interface 的接口（如
// toString()）映射出来

class Animal {
public:
  struct Interface {
  public:
    virtual std::string to_string() const = 0;
    virtual ~Interface() = default;
  };

  std::shared_ptr<const Interface> p_;

public:
  Animal(Interface *p) : p_(p) {}
  std::string to_string() { return p_->to_string(); }
};

class Bird : public Animal::Interface {
public:
  Bird(std::string name) : name_(name) {}
  std::string to_string() const {
    return std::string("I'm a brid named ") + name_ + ".\n";
  }

  ~Bird() { printf("Bird destroy.\n"); }

private:
  std::string name_;
};

class Ant : public Animal::Interface {
public:
  Ant(std::string name) : name_(name) {}
  std::string to_string() const {
    return std::string("I'm an ant named ") + name_ + ".\n";
  }

  ~Ant() { printf("Ant destroy.\n"); }

private:
  std::string name_;
};

int main() {
  std::vector<Animal> animals;
  animals.push_back(new Bird("Albert"));
  animals.push_back(new Bird("Bert"));
  animals.push_back(new Ant("Cola"));

  for (auto animal : animals) {
    std::cout << animal.to_string();
  }

  return 0;
}