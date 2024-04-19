#pragma once

#include <iterator>
#include <vector>
template <typename Iterator, typename Distance>
void advanceIterImpl(Iterator &iter, Distance distance,
                     std::random_access_iterator_tag) {
  iter += distance;
}

template <typename Iterator, typename Distance>
void advanceIterImpl(Iterator &iter, Distance distance,
                     std::bidirectional_iterator_tag) {
  while (distance > 0) {
    iter++;
    distance--;
  }
}

template <typename Iterator, typename Distance>
void advanceIter(Iterator &iter, Distance distance) {
  advanceIterImpl(iter, distance,
                  typename std::iterator_traits<Iterator>::iterator_category());
}

void test() {
  std::vector<int> vec(10);
  auto iter = vec.begin();
  advanceIter(iter, 2);
}

