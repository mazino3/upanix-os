INCLUDES="-I${UPANIX_HOME}/libmcpp/infra -I${UPANIX_HOME}/libmcpp/ds"
DFLAGS=-D__LOCAL_TEST__
CXXFLAGS=-std=c++11

rm -f test_list test_map test_queue test_vector

echo "compiling test_vector.C..."
g++ ${CXXFLAGS} ${INCLUDES} ${DFLAGS} test_vector.C -o test_vector

echo "compiling test_queue.C..."
g++ ${CXXFLAGS} ${INCLUDES} ${DFLAGS} test_queue.C -o test_queue

echo "compiling test_list.C..."
g++ ${CXXFLAGS} ${INCLUDES} ${DFLAGS} test_list.C -o test_list

echo "compiling test_map.C..."
g++ ${CXXFLAGS} ${INCLUDES} ${DFLAGS} test_map.C -o test_map
