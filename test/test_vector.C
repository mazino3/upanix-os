#include <iostream>
#include "vector.h"
#include "algorithm.h"
#include "testassert.h"

struct Obj
{
  Obj(int x, int y) : k(x), v(y) {}
  int k;
  int v;
};

int main()
{
  upan::vector<int> x;

  TEST_ASSERT(x.empty());
  TEST_ASSERT(x.size() == 0);

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
  
  std::cout << "Find 30" << std::endl;
  TEST_ASSERT(upan::find(x.begin(), x.end(), 30) != x.end());

  auto i = x.begin();
  while(i != x.end())
  {
    std::cout << "Searching 30 " << *i << std::endl;
    if(*i == 30)
      i = x.erase(i);
     else
       ++i;
  }

  std::cout << "30 not found" << std::endl;
  TEST_ASSERT(upan::find(x.begin(), x.end(), 30) == x.end());

  std::cout << "Find 40 using equal_to predicate" << std::endl;
  TEST_ASSERT(upan::find_if(x.begin(), x.end(), upan::equal_to<int>(40)) != x.end());

  std::cout << "delete first element" << std::endl;
  x.erase(5);
  TEST_ASSERT(x.empty() == false);
  TEST_ASSERT(x.size() == 8);
  TEST_ASSERT(*x.begin() == 6);
  TEST_ASSERT(x[0] == 6);
  TEST_ASSERT(*(x.begin() + 1) == 7);
  auto it = x.begin() + 1;
  TEST_ASSERT(*(it + 1) == 8);
  TEST_ASSERT(x[3] == 9);
  TEST_ASSERT(x[4] == 10);
  TEST_ASSERT(x[5] == 20);
  TEST_ASSERT(x[6] == 40);
  TEST_ASSERT(x[7] == 50);
  TEST_ASSERT(*(x.end() - 1) == 50);

  i = x.begin();
  while(i != x.end())
  {
    std::cout << "Deleting " << *i << std::endl;
    i = x.erase(i);
  }
  for(const auto i : x)
    std::cout << i << std::endl;

  TEST_ASSERT(x.empty());
  TEST_ASSERT(x.size() == 0);

  TEST_ASSERT(x._capacity == 13 * sizeof(int));

  std::cout << "10 new elements" << std::endl;
  for(int i = 0; i < 10; ++i) x.push_back(i);
  TEST_ASSERT(!x.empty());
  TEST_ASSERT(x.size() == 10);
  TEST_ASSERT(x._capacity == 13 * sizeof(int));

  std::cout << "90 new elements" << std::endl;
  for(int i = 10; i < 100; ++i) x.push_back(i);
  TEST_ASSERT(!x.empty());
  TEST_ASSERT(x.size() == 100);
  TEST_ASSERT(x._capacity == 141 * sizeof(int));

  std::cout << "Insert 10 elements in the middle" << std::endl;
  for(int i = 50; i < 60; ++i) x.insert(i, i);
  TEST_ASSERT(!x.empty());
  TEST_ASSERT(x.size() == 110);
  TEST_ASSERT(x._capacity == 141 * sizeof(int));

  for(int i = 0; i < 110; ++i)
  {
    if(i > 59)
    {
      TEST_ASSERT(x[i] == i - 10);
    }
    else
    {
      TEST_ASSERT(x[i] == i);
    }
  }

  std::cout << "delete 10 elements in the middle" << std::endl;
  x.erase(50, 60);
  TEST_ASSERT(!x.empty());
  TEST_ASSERT(x.size() == 100);
  TEST_ASSERT(x._capacity == 141 * sizeof(int));
  for(int i = 0; i < 100; ++i)
    TEST_ASSERT(x[i] == i);

  std::cout << "delete last 11 elements" << std::endl;
  x.erase(x.begin() + 89, x.end());
  TEST_ASSERT(!x.empty());
  TEST_ASSERT(x.size() == 89);
  TEST_ASSERT(x._capacity == 141 * sizeof(int));
  for(int i = 0; i < 89; ++i)
    TEST_ASSERT(x[i] == i);

  std::cout << "insert last 11 elements" << std::endl;
  for(int i = 89; i < 100; ++i) x.insert(x.end(), i);
  TEST_ASSERT(!x.empty());
  TEST_ASSERT(x.size() == 100);
  TEST_ASSERT(x._capacity == 141 * sizeof(int));
  for(int i = 0; i < 100; ++i)
    TEST_ASSERT(x[i] == i);

  std::cout << "delete first 11 elements" << std::endl;
  x.erase(x.begin(), x.begin() + 11);
  TEST_ASSERT(!x.empty());
  TEST_ASSERT(x.size() == 89);
  TEST_ASSERT(x._capacity == 141 * sizeof(int));
  for(int i = 0; i < 89; ++i)
    TEST_ASSERT(x[i] == i + 11);

  std::cout << "pop 2 elements" << std::endl;
  x.pop_back();
  x.pop_back();
  TEST_ASSERT(!x.empty());
  TEST_ASSERT(x.size() == 87);
  TEST_ASSERT(x._capacity == 141 * sizeof(int));
  for(int i = 0; i < 87; ++i)
    TEST_ASSERT(x[i] == i + 11);

  std::cout << "assignment" << std::endl;
  upan::vector<Obj> o1;
  for(int i = 0; i < 10; ++i)
    o1.push_back(Obj(i, i * 10));
  TEST_ASSERT(o1.size() == 10);
  TEST_ASSERT(o1._capacity == 13 * sizeof(Obj));

  upan::vector<Obj> o2;
  for(int i = 0; i < 20; ++i)
    o2.push_back(Obj(i, i * 10));
  TEST_ASSERT(o2.size() == 20);
  TEST_ASSERT(o2._capacity == 28 * sizeof(Obj));

  o2 = o1;
  TEST_ASSERT(o2.size() == 10);
  TEST_ASSERT(o2._capacity == 13 * sizeof(Obj));

  o1.push_back(Obj(-1, -1));
  TEST_ASSERT(o2.size() == 10);
  TEST_ASSERT(o2._capacity == 13 * sizeof(Obj));
  for(int i = 0; i < 10; ++i)
    TEST_ASSERT(o2[i].k == i && o2[i].v == i * 10);
  
  TEST_ASSERT(o1.size() == 11);
  TEST_ASSERT(o1._capacity == 13 * sizeof(Obj));
  for(int i = 0; i < 10; ++i)
    TEST_ASSERT(o1[i].k == i && o1[i].v == i * 10);
  TEST_ASSERT(o1[10].k == -1 && o1[10].v == -1);

  std::cout << "swap" << std::endl;
  o1.swap(o2);
  TEST_ASSERT(o1.size() == 10);
  TEST_ASSERT(o1._capacity == 13 * sizeof(Obj));
  for(int i = 0; i < 10; ++i)
    TEST_ASSERT(o1[i].k == i && o1[i].v == i * 10);
  
  TEST_ASSERT(o2.size() == 11);
  TEST_ASSERT(o2._capacity == 13 * sizeof(Obj));
  for(int i = 0; i < 10; ++i)
    TEST_ASSERT(o2[i].k == i && o2[i].v == i * 10);
  TEST_ASSERT(o2[10].k == -1 && o2[10].v == -1);

  return 0;
}
