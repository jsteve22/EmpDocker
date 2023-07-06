#ifndef HELPER_FUNCTIONS_H
#define HELPER_FUNCTIONS_H

#include "emp-sh2pc/emp-sh2pc.h"
#include <sys/time.h>
#include <iostream>
#include <inttypes.h>

#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include "interface.h"
#include <time.h>
#include <string.h>
#include <vector>
#include <fstream>
#include <sstream>
#include <string>
#include <map>

typedef uint64_t u64;
using namespace emp;
using namespace std;

void scale_down(u64* layer, int len, int scale);

u64** split_input_layer(u64** input_layer, int len);

void linear_layer_sum(u64** target, u64** adder, int channels, int len);

void linear_layer_centerlift(u64** layer, int channels, int len);

#endif
