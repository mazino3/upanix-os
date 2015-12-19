#include <iostream>
#include "list.h"
#include "algorithm.h"

int main()
{
  upan::list<int> x;
  upan::list<const int> y;
  upan::list<int*> a;
  upan::list<const int*> b;

  x.push_back(10);
  x.push_back(20);
  x.push_back(30);
  x.push_back(40);
  x.push_back(50);
  x.push_front(9);
  x.push_front(8);
  x.push_front(7);
  x.push_front(6);
  x.push_front(5);
  for(const auto i : x)
    std::cout << i << std::endl;
  
  if(upan::find(x.begin(), x.end(), 30) != x.end())
    std::cout << "Found 30" << std::endl;
  else
    std::cout << "Didn't find 30" << std::endl;

  auto i = x.begin();
  while(i != x.end())
  {
    std::cout << "Searching 30 " << *i << std::endl;
    if(*i == 30)
      x.erase(i++);
     else
       ++i;
  }

  if(upan::find(x.begin(), x.end(), 30) != x.end())
    std::cout << "Found 30" << std::endl;
  else
    std::cout << "Didn't find 30" << std::endl;

  if(upan::find_if(x.begin(), x.end(), upan::equal_to<int>(40)) != x.end())
    std::cout << "Found 40" << std::endl;
  else
    std::cout << "Didn't find 40" << std::endl;

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
