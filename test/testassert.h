#ifndef _TEST_ASSERT_H_
#define _TEST_ASSERT_H_

#include <iostream>
#include <string>

#define TEST_ASSERT(EXPR) \
	if(!(EXPR)) \
	{\
    std::cout <<"Error@Line: " <<  __LINE__ << " --> " << #EXPR << " -- FAILED!" << std::endl; \
    exit(1); \
	}

#endif
