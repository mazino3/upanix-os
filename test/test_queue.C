#include <iostream>
#include "queue.h"
#include "testassert.h"

class TestC
{
public:
  static int _instanceCount;
  TestC()
  {
    ++_instanceCount;
  }

};

int main()
{
  std::cout << "Create a queue of size 4" << std::endl;
  upan::queue<int> iq(4);
  TEST_ASSERT(iq.empty());
  TEST_ASSERT(!iq.full());

  std::cout << "Insert 4 elements" << std::endl;
  TEST_ASSERT(iq.push_back(1));
  TEST_ASSERT(iq.push_back(2));
  TEST_ASSERT(iq.push_back(3));
  TEST_ASSERT(iq.push_back(4));

  TEST_ASSERT(!iq.empty());
  TEST_ASSERT(iq.full());

  std::cout << "Remove 1 element" << std::endl;
  TEST_ASSERT(iq.front() == 1);
  TEST_ASSERT(iq.pop_front());
  TEST_ASSERT(!iq.empty());
  TEST_ASSERT(!iq.full());

  std::cout << "Add another element" << std::endl;
  TEST_ASSERT(iq.push_back(5));
  TEST_ASSERT(!iq.empty());
  TEST_ASSERT(iq.full());

  std::cout << "Remove all elements" << std::endl;
  TEST_ASSERT(iq.front() == 2);
  TEST_ASSERT(iq.pop_front());
  TEST_ASSERT(!iq.empty());
  TEST_ASSERT(!iq.full());

  TEST_ASSERT(iq.front() == 3);
  TEST_ASSERT(iq.pop_front());
  TEST_ASSERT(!iq.empty());
  TEST_ASSERT(!iq.full());

  TEST_ASSERT(iq.front() == 4);
  TEST_ASSERT(iq.pop_front());
  TEST_ASSERT(!iq.empty());
  TEST_ASSERT(!iq.full());

  TEST_ASSERT(iq.front() == 5);
  TEST_ASSERT(iq.pop_front());
  TEST_ASSERT(iq.empty());
  TEST_ASSERT(!iq.full());

  return 0;
}
