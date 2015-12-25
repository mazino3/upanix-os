#include <map>
#include <typeinfo>
#include <sys/time.h>
#include "map.h"

struct timeval t1, t2;
double _t1;
double _t2;

typedef upan::map<int, int> UPAN_MAP;
typedef std::map<int, int> STD_MAP;

template <typename T>
const char* type()
{
  if(typeid(T) == typeid(UPAN_MAP))
    return "UPAN::MAP";
  return "STD::MAP";
}

template <typename T>
void Insert(T& m)
{
  gettimeofday(&t1, NULL);
  for(int i = 0; i < 1000 * 1000; ++i)
    m.insert(typename T::value_type(i, i));
  gettimeofday(&t2, NULL);

  _t1 = t1.tv_sec + t1.tv_usec / 1000000.0;
  _t2 = t2.tv_sec + t2.tv_usec / 1000000.0;
  
  printf("\n  %s INSERT TIME = %lf", type<T>(), _t2 - _t1);
}

template <typename T>
void Find(T& m)
{
  gettimeofday(&t1, NULL);
  for(int i = 0; i < 1000 * 1000; ++i)
    if(m.find(i) == m.end())
    {
      printf("\n Find for %d failed!", i);
      break;
    }
  gettimeofday(&t2, NULL);

  _t1 = t1.tv_sec + t1.tv_usec / 1000000.0;
  _t2 = t2.tv_sec + t2.tv_usec / 1000000.0;

  printf("\n  %s FIND TIME = %lf", type<T>(), _t2 - _t1);
}


template <typename T>
void FindNonExisting(T& m)
{
  gettimeofday(&t1, NULL);
  for(int i = 0; i < 1000 * 1000; ++i)
    if(m.find(i + 1000 * 1000) != m.end())
    {
      printf("\n Found non-existing node %d!", i);
      break;
    }
  gettimeofday(&t2, NULL);

  _t1 = t1.tv_sec + t1.tv_usec / 1000000.0;
  _t2 = t2.tv_sec + t2.tv_usec / 1000000.0;

  printf("\n  %s FIND NON-EXISTING TIME = %lf", type<T>(), _t2 - _t1);
}

template <typename T>
void IndexFindInsert(T& m)
{
  gettimeofday(&t1, NULL);
  for(int i = 0; i < 2000 * 1000; ++i)
    m[i] = i;
  gettimeofday(&t2, NULL);

  _t1 = t1.tv_sec + t1.tv_usec / 1000000.0;
  _t2 = t2.tv_sec + t2.tv_usec / 1000000.0;
  
  printf("\n  %s INDEX FIND+INSERT TIME = %lf", type<T>(), _t2 - _t1);
}

template <typename T>
void MiddleDelete(T& m)
{
  gettimeofday(&t1, NULL);
  for(int i = 500 * 1000; i < 1500 * 1000; ++i)
  {
    if(!m.erase(i))
    {
      printf("\n Middle delete failed for %d!", i);
      break;
    }
  }
  gettimeofday(&t2, NULL);

  _t1 = t1.tv_sec + t1.tv_usec / 1000000.0;
  _t2 = t2.tv_sec + t2.tv_usec / 1000000.0;
  printf("\n  %s MIDDLE DELETE TIME = %lf", type<T>(), _t2 - _t1);
}

template <typename T>
void FindAfterMiddleDelete(T& m)
{
  gettimeofday(&t1, NULL);
  for(int i = 0; i < 2000 * 1000; ++i)
  {
    auto it = m.find(i);
    if(i >= 500 * 1000 && i < 1500 * 1000)
    {
      if(it != m.end())
      {
        printf("\n Found deleted node %d!", i);
        break;
      }
    }
    else
    {
      if(it == m.end())
      {
        printf("\n Didn't find undeleted node: %d!", i);
        break;
      }
    }
  }
  gettimeofday(&t2, NULL);

  _t1 = t1.tv_sec + t1.tv_usec / 1000000.0;
  _t2 = t2.tv_sec + t2.tv_usec / 1000000.0;
  printf("\n  %s FIND AFTER MIDDLE TIME = %lf", type<T>(), _t2 - _t1);
}

template <typename T>
void IterateFull(T& m)
{
  gettimeofday(&t1, NULL);
  int i = 0;
  for(auto it = m.begin(); it != m.end(); ++it)
  {
    if(i == 500 * 1000)
      i = 1500 * 1000;
    if(it->first != i++)
    {
      printf("\n Iterating to non-existing node: %d (%d)!", it->first, i);
      break;
    }
  }
  if(i != 2000 * 1000)
    printf("\n Iterating full failed!");
  gettimeofday(&t2, NULL);

  _t1 = t1.tv_sec + t1.tv_usec / 1000000.0;
  _t2 = t2.tv_sec + t2.tv_usec / 1000000.0;

  printf("\n  %s FULL ITERATE = %lf", type<T>(), _t2 - _t1);
}

template <typename T>
void Clear(T& m)
{
  gettimeofday(&t1, NULL);
  m.clear();
  if(m.size() != 0)
    printf("\n clear failed!");
  if(m.begin() != m.end())
    printf("\n begin != end upon clear");
  gettimeofday(&t2, NULL);

  _t1 = t1.tv_sec + t1.tv_usec / 1000000.0;
  _t2 = t2.tv_sec + t2.tv_usec / 1000000.0;
  printf("\n  %s CLEAR TIME = %lf", type<T>(), _t2 - _t1);
}

template <typename T>
void IterativeErase(T& m)
{
  gettimeofday(&t1, NULL);
  auto it = m.begin();
  while(it != m.end())
    m.erase(it++);
  if(m.size() != 0)
    printf("\n umap is not empty after erasing all!");
  if(m.begin() != m.end())
    printf("\n begin != end after erasing all!");
  gettimeofday(&t2, NULL);

  _t1 = t1.tv_sec + t1.tv_usec / 1000000.0;
  _t2 = t2.tv_sec + t2.tv_usec / 1000000.0;
  printf("\n  %s ERASE ALL TIME = %lf", type<T>(), _t2 - _t1);
}

int main()
{
  UPAN_MAP um;
  STD_MAP sm;

  printf("\nInsertion...");
  Insert(um);
  Insert(sm);

  printf("\nFind...");
  Find(um);
  Find(sm);

  printf("\nFind non-existing...");
  FindNonExisting(um);
  FindNonExisting(sm);

  printf("\nIndex find + insert...");
  IndexFindInsert(um);
  IndexFindInsert(sm);

  printf("\nMiddle delete...");
  MiddleDelete(um);
  MiddleDelete(sm);

  printf("\nFind after middle delete...");
  FindAfterMiddleDelete(um);
  FindAfterMiddleDelete(sm);

  printf("\nIterate full...");
  IterateFull(um);
  IterateFull(sm);

  printf("\nclear...");
  Clear(um);
  Clear(sm);

  printf("\nInsertion again...");
  Insert(um);
  Insert(sm);

  printf("\nFind again...");
  Find(um);
  Find(sm);

  printf("\nIterative erase...");
  IterativeErase(um);
  IterativeErase(sm);

  printf("\nInsertion again...");
  Insert(um);
  Insert(sm);

  printf("\nFind again...");
  Find(um);
  Find(sm);
  
  if(um.verify_balance_factor())
    printf("\n\n**** UPAN::MAP NODE BALANCE FACTOR IS INTACT ****\n");
  else
    printf("\n\n***** UPAN::MAP NODE BALANCE FACTOR IS CORRUPTED!! ****\n\n");

  printf("\nclear...");
  Clear(um);
  Clear(sm);

  printf("\n");
  return 0;
}
