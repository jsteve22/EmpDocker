#ifndef READ_TXT_H
#define READ_TXT_H

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

using namespace std;

vector<vector<vector<int> > > read_image(string file);

vector<int> read_weights_1(string file);

vector<vector<int> > read_weights_2(string file);

vector<vector<vector<vector<int> > > > read_weights_4(string file);

#endif