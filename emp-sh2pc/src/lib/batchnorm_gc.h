#ifndef BATCHNORM_GC_H
#define BATCHNORM_GC_H

#include "emp-sh2pc/emp-sh2pc.h"
#include <sys/time.h>
#include <iostream>
#include <inttypes.h>
#include <math.h>

#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include "interface.h"
#include "read_txt.h"
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

u64* batchnorm_gc(int bitsize, int64_t* inputs_a, int height, int width, int channels, int party, string weights_path="", unsigned dup_test=1);

#endif