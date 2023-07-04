#ifndef EXTRAS_H
#define EXTRAS_H

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

#include "interface.h"

typedef uint64_t u64;
using namespace emp;
using namespace std;

u64 reduce(unsigned __int128 x);

u64 mul(u64 x, u64 y);

void beavers_triples(ClientFHE* cfhe, ServerFHE* sfhe, int num_triples);

#endif