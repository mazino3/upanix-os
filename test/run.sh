INCLUDES="-I${UPANIX_HOME}/libmcpp/infra -I${UPANIX_HOME}/libmcpp/ds"
DFLAGS=-D__LOCAL_TEST__
CXXFLAGS=-std=c++11

rm -f test_list test_map

g++ ${CXXFLAGS} ${INCLUDES} ${DFLAGS} test_list.C -o test_list && ./test_list
rm -f test_list

g++ ${CXXFLAGS} ${INCLUDES} ${DFLAGS} test_map.C -o test_map && ./test_map
rm -f test_map
