#pragma once
#include "../../Chapter19/SFINAE/detect.h"
#include <map>
#include <tuple>
#include <unordered_map>
#include <utility>
#include <vector>

template <typename Key, typename Value, typename = void> class Dictionary {
private:
  std::vector<std::pair<Key const, Value>> data;

public:
  // subscripted access to the data:
  Value &operator[](Key const &key) {
    // search for the element with this key:
    for (auto &element : data) {
      if (element.first == key) {
        return element.second;
      }
    }
    // there is no element with this key; add one
    data.push_back(pair<Key const, Value>(key, Value()));
    return data.back().second;
  }
  // ...
  Dictionary() { printf("Default implement.\n"); }
};

template <typename Key, typename Value>
class Dictionary<Key, Value, std::enable_if_t<hasHash(type<Key>)>> {
private:
  std::unordered_map<Key, Value> data;

public:
  // subscripted access to the data:
  Value &operator[](Key const &key) { return data[key]; }
  // ...
  Dictionary() { printf("Unorder Map implement.\n"); }
};

template <typename Key, typename Value>
class Dictionary<
    Key, Value,
    std::enable_if_t<hasLess(type<int>, type<int>) && !hasHash(type<Key>)>> {
private:
  std::map<Key const, Value> data;

public:
  // subscripted access to the data:
  Value &operator[](Key const &key) { return data[key]; }
  // ...
  Dictionary() { printf("Map implement.\n"); }
};

void test_dic() { Dictionary<int, int> dic; }