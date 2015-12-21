#include <map>
#include "map.h"
#include <sys/time.h>

int main()
{
  struct timeval t1, t2;
  upan::map<int, int> um;
  std::map<int, int> sm;

  for(int i = 0; i < 1000 * 12; ++i)
    um.insert(upan::pair<int, int>(i, i));

  for(int i = 100 * 10; i < 1000 * 9; ++i)
    um.erase(i);

  return 0;
  printf("\nInsertion...");
  gettimeofday(&t1, NULL);
  for(int i = 0; i < 1000 * 1000; ++i)
    um.insert(upan::pair<int, int>(i, i));
  gettimeofday(&t2, NULL);

  double _t1 = t1.tv_sec + t1.tv_usec / 1000000.0;
  double _t2 = t2.tv_sec + t2.tv_usec / 1000000.0;
  printf("\n  UPAN::MAP INSERT TIME = %lf", _t2 - _t1);

  gettimeofday(&t1, NULL);
  for(int i = 0; i < 1000 * 1000; ++i)
    sm.insert(std::pair<int, int>(i, i));
  gettimeofday(&t2, NULL);

  _t1 = t1.tv_sec + t1.tv_usec / 1000000.0;
  _t2 = t2.tv_sec + t2.tv_usec / 1000000.0;
  printf("\n  STD::MAP INSERT TIME = %lf", _t2 - _t1);

  printf("\nFind...");
  gettimeofday(&t1, NULL);
  for(int i = 0; i < 1000 * 1000; ++i)
    if(um.find(i) == um.end())
    {
      printf("\n Find for %d failed!", i);
      break;
    }
  gettimeofday(&t2, NULL);

  _t1 = t1.tv_sec + t1.tv_usec / 1000000.0;
  _t2 = t2.tv_sec + t2.tv_usec / 1000000.0;
  printf("\n  UPAN::MAP FIND TIME = %lf", _t2 - _t1);

  gettimeofday(&t1, NULL);
  for(int i = 0; i < 1000 * 1000; ++i)
    if(sm.find(i) == sm.end())
    {
      printf("\n Find for %d failed!", i);
      break;
    }
  gettimeofday(&t2, NULL);

  _t1 = t1.tv_sec + t1.tv_usec / 1000000.0;
  _t2 = t2.tv_sec + t2.tv_usec / 1000000.0;
  printf("\n  STD::MAP FIND TIME = %lf", _t2 - _t1);

  printf("\nFind non-existing...");
  gettimeofday(&t1, NULL);
  for(int i = 0; i < 1000 * 1000; ++i)
    if(um.find(i + 1000 * 1000) != um.end())
    {
      printf("\n Found non-existing node %d!", i);
      break;
    }
  gettimeofday(&t2, NULL);

  _t1 = t1.tv_sec + t1.tv_usec / 1000000.0;
  _t2 = t2.tv_sec + t2.tv_usec / 1000000.0;
  printf("\n  UPAN::MAP FIND NON-EXISTING TIME = %lf", _t2 - _t1);

  gettimeofday(&t1, NULL);
  for(int i = 0; i < 1000 * 1000 * 10; ++i)
    if(sm.find(i + 1000 * 1000) != sm.end())
    {
      printf("\n Found non-existing node %d!", i);
      break;
    }
  gettimeofday(&t2, NULL);

  _t1 = t1.tv_sec + t1.tv_usec / 1000000.0;
  _t2 = t2.tv_sec + t2.tv_usec / 1000000.0;
  printf("\n  STD::MAP FIND NON-EXISTING TIME = %lf", _t2 - _t1);

  printf("\nIndex find + insert...");
  gettimeofday(&t1, NULL);
  for(int i = 0; i < 2000 * 1000; ++i)
    um[i] = i;
  gettimeofday(&t2, NULL);

  _t1 = t1.tv_sec + t1.tv_usec / 1000000.0;
  _t2 = t2.tv_sec + t2.tv_usec / 1000000.0;
  printf("\n  UPAN::MAP INDEX FIND+INSERT TIME = %lf", _t2 - _t1);
 
  gettimeofday(&t1, NULL);
  for(int i = 0; i < 2000 * 1000; ++i)
    sm[i] = i;
  gettimeofday(&t2, NULL);

  _t1 = t1.tv_sec + t1.tv_usec / 1000000.0;
  _t2 = t2.tv_sec + t2.tv_usec / 1000000.0;
  printf("\n  STD::MAP INDEX FIND+INSERT TIME = %lf", _t2 - _t1);

  printf("\nMiddle delete...");
  gettimeofday(&t1, NULL);
  try
  {
    for(int i = 500 * 1000; i < 1500 * 1000; ++i)
    {
      printf("\n deleting %d", i);
      if(!um.erase(i))
      {
        printf("\n Middle delete failed for %d!", i);
        break;
      }
    }
  }
  catch(upan::exception& ex)
  {
    printf("\n exception");
  }
  gettimeofday(&t2, NULL);

  _t1 = t1.tv_sec + t1.tv_usec / 1000000.0;
  _t2 = t2.tv_sec + t2.tv_usec / 1000000.0;
  printf("\n  UPAN::MAP MIDDLE DELETE TIME = %lf", _t2 - _t1);

  gettimeofday(&t1, NULL);
  for(int i = 500 * 1000; i < 1500 * 1000; ++i)
    if(!sm.erase(i))
    {
      printf("\n Middle delete failed for %d!", i);
      break;
    }
  gettimeofday(&t2, NULL);

  _t1 = t1.tv_sec + t1.tv_usec / 1000000.0;
  _t2 = t2.tv_sec + t2.tv_usec / 1000000.0;
  printf("\n  STD::MAP MIDDLE DELETE TIME = %lf", _t2 - _t1);

  printf("\nFind after middle delete...");
  gettimeofday(&t1, NULL);
  for(int i = 0; i < 2000 * 1000; ++i)
  {
    auto it = um.find(i);
    if(i >= 500 * 1000 && i < 1500 * 1000)
    {
      if(it != um.end())
      {
        printf("\n Found deleted node %d!", i);
        break;
      }
    }
    else
    {
      if(it == um.end())
      {
        printf("\n Didn't find undeleted node: %d!", i);
        break;
      }
    }
  }
  gettimeofday(&t2, NULL);

  _t1 = t1.tv_sec + t1.tv_usec / 1000000.0;
  _t2 = t2.tv_sec + t2.tv_usec / 1000000.0;
  printf("\n  UPAN::MAP FIND AFTER MIDDLE TIME = %lf", _t2 - _t1);

  gettimeofday(&t1, NULL);
  for(int i = 0; i < 2000 * 1000; ++i)
  {
    auto it = sm.find(i);
    if(i >= 500 * 1000 && i < 1500 * 1000)
    {
      if(it != sm.end())
      {
        printf("\n Found deleted node %d!", i);
        break;
      }
    }
    else
    {
      if(it == sm.end())
      {
        printf("\n Didn't find undeleted node: %d!", i);
        break;
      }
    }
  }
  gettimeofday(&t2, NULL);

  _t1 = t1.tv_sec + t1.tv_usec / 1000000.0;
  _t2 = t2.tv_sec + t2.tv_usec / 1000000.0;
  printf("\n  STD::MAP FIND AFTER MIDDLE DELETE TIME = %lf", _t2 - _t1);

  printf("\nIterate full...");
  gettimeofday(&t1, NULL);
  int i = 0;
  for(auto it = um.begin(); it != um.end(); ++it)
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
  printf("\n  UPAN::MAP FULL ITERATE = %lf", _t2 - _t1);

  gettimeofday(&t1, NULL);
  i = 0;
  for(auto it = sm.begin(); it != sm.end(); ++it)
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
  printf("\n  STD::MAP FULL ITERATE = %lf", _t2 - _t1);

  printf("\nclear...");
  gettimeofday(&t1, NULL);
  um.clear();
  if(um.size() != 0)
    printf("\n clear failed!");
  if(um.begin() != um.end())
    printf("\n begin != end upon clear");
  gettimeofday(&t2, NULL);

  _t1 = t1.tv_sec + t1.tv_usec / 1000000.0;
  _t2 = t2.tv_sec + t2.tv_usec / 1000000.0;
  printf("\n  UPAN::MAP CLEAR TIME = %lf", _t2 - _t1);

  gettimeofday(&t1, NULL);
  sm.clear();
  gettimeofday(&t2, NULL);

  _t1 = t1.tv_sec + t1.tv_usec / 1000000.0;
  _t2 = t2.tv_sec + t2.tv_usec / 1000000.0;
  printf("\n  STD::MAP FULL ITERATE = %lf", _t2 - _t1);

  printf("\nInsertion again...");
  gettimeofday(&t1, NULL);
  for(int i = 0; i < 1000 * 1000; ++i)
    um.insert(upan::pair<int, int>(i, i));
  gettimeofday(&t2, NULL);

  _t1 = t1.tv_sec + t1.tv_usec / 1000000.0;
  _t2 = t2.tv_sec + t2.tv_usec / 1000000.0;
  printf("\n  UPAN::MAP INSERT TIME = %lf", _t2 - _t1);

  gettimeofday(&t1, NULL);
  for(int i = 0; i < 1000 * 1000; ++i)
    sm.insert(std::pair<int, int>(i, i));
  gettimeofday(&t2, NULL);

  _t1 = t1.tv_sec + t1.tv_usec / 1000000.0;
  _t2 = t2.tv_sec + t2.tv_usec / 1000000.0;
  printf("\n  STD::MAP INSERT TIME = %lf", _t2 - _t1);

  printf("\nFind...");
  gettimeofday(&t1, NULL);
  for(int i = 0; i < 1000 * 1000; ++i)
    if(um.find(i) == um.end())
    {
      printf("\n Find for %d failed!", i);
      break;
    }
  gettimeofday(&t2, NULL);
  
  printf("\nFind again...");
  gettimeofday(&t1, NULL);
  for(int i = 0; i < 1000 * 1000; ++i)
    if(um.find(i) == um.end())
    {
      printf("\n Find for %d failed!", i);
      break;
    }
  gettimeofday(&t2, NULL);

  _t1 = t1.tv_sec + t1.tv_usec / 1000000.0;
  _t2 = t2.tv_sec + t2.tv_usec / 1000000.0;
  printf("\n  UPAN::MAP FIND TIME = %lf", _t2 - _t1);

  gettimeofday(&t1, NULL);
  for(int i = 0; i < 1000 * 1000; ++i)
    if(sm.find(i) == sm.end())
    {
      printf("\n Find for %d failed!", i);
      break;
    }
  gettimeofday(&t2, NULL);

  _t1 = t1.tv_sec + t1.tv_usec / 1000000.0;
  _t2 = t2.tv_sec + t2.tv_usec / 1000000.0;
  printf("\n  STD::MAP FIND TIME = %lf", _t2 - _t1);

  printf("\nIterative erase...");
  gettimeofday(&t1, NULL);
  auto uit = um.begin();
  while(uit != um.end())
    if(!um.erase(uit++))
    {
      printf("\n Delete failed!");
      break;
    }
  if(um.size() != 0)
    printf("\n umap is not empty after erasing all!");
  if(um.begin() != um.end())
    printf("\n begin != end after erasing all!");
  gettimeofday(&t2, NULL);

  _t1 = t1.tv_sec + t1.tv_usec / 1000000.0;
  _t2 = t2.tv_sec + t2.tv_usec / 1000000.0;
  printf("\n  UPAN::MAP ERASE ALL TIME = %lf", _t2 - _t1);

  gettimeofday(&t1, NULL);
  auto sit = sm.begin();
  while(sit != sm.end())
    sm.erase(sit++);
  if(sm.size() != 0)
    printf("\n smap is not empty after erasing all!");
  if(sm.begin() != sm.end())
    printf("\n begin != end after erasing all!");
  gettimeofday(&t2, NULL);

  _t1 = t1.tv_sec + t1.tv_usec / 1000000.0;
  _t2 = t2.tv_sec + t2.tv_usec / 1000000.0;
  printf("\n  STD::MAP ERASE ALL TIME = %lf", _t2 - _t1);

  return 0;
}
