#include <iostream>
#include <vector>

int main(int, char **)
{
  std::cout << "Hello, from teleop-control!\n";

  int64_t *f = (int64_t *)malloc(sizeof(int64_t));

  std::cout << "value of f: " << *f << std::endl;

  *f = 1000;

  std::cout << "value of f: " << *f << std::endl;
  std::cout << "sizeof: " << sizeof(int) << std::endl;
  std::cout << "sizeof: " << sizeof(int64_t) << std::endl;

  *f = 2147483647;

  std::cout << "value of f: " << *f << std::endl;

  *f = (2147483649);

  std::cout << "value of f: " << *f << std::endl;

  std::vector<int>
      vec;
  vec.push_back(4);
  vec.push_back(3);

  for (int i : vec)
  {
    std::cout << "id: " << i << std::endl;
  }
}
