#include <iostream>
#include "vector.h"
#include "algorithm.h"
#include "testassert.h"

int main()
{
  upan::vector<int> x;
  upan::vector<const int> y;
  upan::vector<int*> a;
  upan::vector<const int*> b;

  x.push_back(10);
  x.push_back(20);
  x.push_back(30);
  x.push_back(40);
  x.push_back(50);
  x.insert(0, 9);
  x.insert(0, 8);
  x.insert(0, 7);
  x.insert(0, 6);
  x.insert(0, 5);
  for(const auto i : x)
    std::cout << i << std::endl;
  
  std::cout << "Find 30 successfully" << std::endl;
  TEST_ASSERT(upan::find(x.begin(), x.end(), 30) != x.end());

  auto i = x.begin();
  while(i != x.end())
  {
    std::cout << "Searching 30 " << *i << std::endl;
    if(*i == 30)
      x.erase(i++);
     else
       ++i;
  }

  std::cout << "30 not found" << std::endl;
  TEST_ASSERT(upan::find(x.begin(), x.end(), 30) == x.end());

  std::cout << "Find 40 using equal_to predicate" << std::endl;
  TEST_ASSERT(upan::find_if(x.begin(), x.end(), upan::equal_to<int>(40)) != x.end());

  i = x.begin();
  while(i != x.end())
  {
    std::cout << "Deleting " << *i << std::endl;
    x.erase(i++);
  }
  for(const auto i : x)
    std::cout << i << std::endl;
  return 0;
}
