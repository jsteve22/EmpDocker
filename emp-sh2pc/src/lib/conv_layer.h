#ifndef CONV_LAYER_H
#define CONV_LAYER_H
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

#include "conv2d.h"
#include "read_txt.h"

typedef uint64_t u64;
using namespace emp;
using namespace std;


struct ConvOutput {
    Metadata data;
    int output_h;
    int output_w;
    int output_chan;
    ClientShares client_shares;
};

ConvOutput conv(ClientFHE* cfhe, ServerFHE* sfhe, int image_h, int image_w, int filter_h, int filter_w,
    int inp_chans, int out_chans, int stride, bool pad_valid, string weights_filename, u64** input_data=NULL);
#endif