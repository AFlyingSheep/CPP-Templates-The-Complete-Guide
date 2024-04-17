#pragma once

#include <iostream>

class DefaultPolicy1 {
public:
  static void execute() { std::cout << "DefaultPolicy1.\n"; }
};
class DefaultPolicy2 {
public:
  static void execute() { std::cout << "DefaultPolicy2.\n"; }
};
class DefaultPolicy3 {
public:
  static void execute() { std::cout << "DefaultPolicy3.\n"; }
};
class CustomPolicy {
public:
  static void execute() { std::cout << "Custom.\n"; }
};

template <typename Policy1 = DefaultPolicy1, typename Policy2 = DefaultPolicy2,
          typename Policy3 = DefaultPolicy3>
class BreadSlicer {
  // ...
};

// 但是如果需要指定 policy3，那么需要指定前面所有的值

class DefaultPolicy {
public:
  using P1 = DefaultPolicy1;
  using P2 = DefaultPolicy2;
  using P3 = DefaultPolicy3;
};

// 对于 Selector 的编写，他需要允许各种 Setter Type 正确传入

class DefaultPolicyArgs : public virtual DefaultPolicy {};

template <typename Policy> class Policy1_is : public virtual DefaultPolicy {
public:
  using P1 = Policy;
};

template <typename Policy> class Policy2_is : public virtual DefaultPolicy {
public:
  using P2 = Policy;
};

template <typename Policy> class Policy3_is : public virtual DefaultPolicy {
public:
  using P3 = Policy;
};

template <typename Base, int D> class Discriminator : public Base {};

template <typename P1, typename P2, typename P3>
class PolicySelector : public Discriminator<P1, 1>,
                       public Discriminator<P2, 2>,
                       public Discriminator<P3, 3> {};

template <typename PolicySetter1 = DefaultPolicyArgs,
          typename PolicySetter2 = DefaultPolicyArgs,
          typename PolicySetter3 = DefaultPolicyArgs>
class BreadSlicer_sec {
  using Policies = PolicySelector<PolicySetter1, PolicySetter2, PolicySetter3>;

public:
  // use Policies::P1, Policies::P2, ... to refer to the various policies
  // ...
  static void execute() { Policies::P3::execute(); }
};
