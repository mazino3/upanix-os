#include <map>
#include "map.h"
#include <sys/time.h>

int main()
{
  struct timeval t1, t2;
  upan::map<int, int> um;
  std::map<int, int> sm;

  gettimeofday(&t1, NULL);
  for(int i = 0; i < 1000 * 1000 * 10; ++i)
    um.insert(upan::pair<int, int>(i, i));
  gettimeofday(&t2, NULL);

  double _t1 = t1.tv_sec + t1.tv_usec / 1000000.0;
  double _t2 = t2.tv_sec + t2.tv_usec / 1000000.0;
  printf("\n UPAN::MAP TIME = %lf \n", _t2 - _t1);

  gettimeofday(&t1, NULL);
  for(int i = 0; i < 1000 * 1000 * 10; ++i)
    sm.insert(std::pair<int, int>(i, i));
  gettimeofday(&t2, NULL);

  _t1 = t1.tv_sec + t1.tv_usec / 1000000.0;
  _t2 = t2.tv_sec + t2.tv_usec / 1000000.0;
  printf("\n STD::MAP TIME = %lf \n", _t2 - _t1);

  return 0;
}
