#ifndef FLATTENS_H
#define FLATTENS_H

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

struct PaddedOutput {  
    u64** matrix;
    int height;
    int width;
};

u64* flatten(u64** input, int channels, int height, int width);

u64** unflatten(u64* input, int channels, int height, int width);

PaddedOutput pad(u64** input, int channels, int height, int width, int padding);

u64** residual(u64** input1, u64** input2, int channels, int height, int width);

#endif