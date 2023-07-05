// written by Dagny Brand and Jeremy Stevens

#include "helper_functions.h"

int SCALE = 256;

void scale_down(u64** layer, int channels, int len, int scale) {
    for (int chan = 0; chan < channels; chan++) {
        for (int idx = 0; idx < len; idx++) {
            if (((int64_t)layer[chan][idx]) < 0) { // if negative
                u64 temp_pos = ~(layer[chan][idx]) + 1; // make positive
                temp_pos = temp_pos / SCALE; // scale
                temp_pos = ~(temp_pos) + 1; // make negative
                layer[chan][idx] = temp_pos;
                continue;
            }
            layer[chan][idx] = layer[chan][idx] / scale;
        }
    }
}

u64** split_input_layer(u64** input_layer, int len) {
    int half = len / 2;
    u64** first_half = (u64**) malloc( sizeof(u64*) * half );
    u64** last_half  = (u64**) malloc( sizeof(u64*) * half );

    for (int i = 0; i < len; i++) {
        if (i < half) first_half[i] = input_layer[i];
        else last_half[i-half] = input_layer[i];
    }

    // free(*input_layer);
    // input_layer = &first_half;
    return last_half;
}

void linear_layer_sum(u64** target, u64** adder, int channels, int len) {
    for (int chan = 0; chan < channels; chan++) {
        for (int idx = 0; idx < len; idx++) {
            target[chan][idx] += adder[chan][idx];
        }
    }
}

void linear_layer_centerlift(u64** layer, int channels, int len) {
    for (int chan = 0; chan < channels; chan++) {
        for (int idx = 0; idx < len; idx++) {
            if (layer[chan][idx] > PLAINTEXT_MODULUS/2) {
                layer[chan][idx] -= PLAINTEXT_MODULUS;
            }
        }
    }
}