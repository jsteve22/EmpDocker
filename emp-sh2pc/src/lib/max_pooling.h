#ifndef MAX_POOLING_H
#define MAX_POOLING_H

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

struct MaxPoolOutput {  
    u64* matrix;
    int height;
    int width;
};

MaxPoolOutput MaxPooling(int bitsize, int64_t* inputs_a, int height, int width, int window_size, int stride, int party, unsigned dup_test=10);

#endif