#pragma once

#include <cassert>
#include <iostream>

#define CHECK(arg) assert(arg)
#define CHECK_EQ(arg1, arg2) assert(arg1 == arg2)
#define CHECK_NE(arg1, arg2) assert(arg1 != arg2)
#define CHECK_LT(arg1, arg2) assert(arg1 < arg2)
#define CHECK_LE(arg1, arg2) assert(arg1 <= arg2)
#define CHECK_GE(arg1, arg2) assert(arg1 >= arg2)
#define LOG(type) std::cout << #type 
