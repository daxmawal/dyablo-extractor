#include <cmath>
#include <iostream>
#include <vector>

const float PI = 3.14159f;

struct Data {
  std::vector<float> values;
  int scale;
};

float
scale_value(float x, int scale)
{
  return x * std::pow(2, scale);
}

void
print_data(const Data& d)
{
  for (float v : d.values) {
    std::cout << "val: " << v << "\n";
  }
}

void
my_kernel(const Data& d)
{
  std::cout << "Running kernel...\n";
  for (float v : d.values) {
    float r = scale_value(v, d.scale);
    std::cout << "scaled: " << r << "\n";
  }
  print_data(d);
}
