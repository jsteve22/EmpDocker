#ifndef DENSE_LAYER_H
#define DENSE_LAYER_H

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

#include "fc_layer.h"
#include "read_txt.h"

typedef uint64_t u64;
using namespace emp;
using namespace std;

u64* fc(ClientFHE* cfhe, ServerFHE* sfhe, int vector_len, int matrix_h, u64* dense_input, string weights_filename, int split_index=0);

#endif