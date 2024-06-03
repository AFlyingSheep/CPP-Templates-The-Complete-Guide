#include "array.h"

#define LENGTH 10

int main() {
  Array<double> x(LENGTH), y(LENGTH);
  for (int i = 0; i < LENGTH; i++) {
    x[i] = i;
    y[i] = x[i];
  }

  x = 1.2 * x + y;
  for (int i = 0; i < LENGTH; i++) {
    std::cout << x[i] << std::endl;
  }

  return 0;
}
