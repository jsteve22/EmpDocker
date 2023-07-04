// written by Dagny Brand and Jeremy Stevens
#include "flattens.h"

// outputs a vector of size (# of channels x height x width)
u64* flatten(u64** input, int channels, int height, int width) {
    u64* flattened = (u64*) malloc(sizeof(u64)*channels*width*height);

    int idx = 0;
    for (int i = 0; i < channels; i++) {
        for (int j = 0; j < width * height; j++) {
            flattened[idx] = input[i][j];
            idx++;
        }
    }
    return flattened;
}

// outputs a 2D matrix of size (channels) by (height x width)
u64** unflatten(u64* input, int channels, int height, int width) {
    u64** unflattened = (u64**) malloc(sizeof(u64*) * channels);
    for (int i = 0; i < channels; i++) {
        unflattened[i] = (u64*) malloc(sizeof(u64) * width * height);
    }

    int idx = 0;
    for (int i = 0; i < channels; i++) {
        for (int j = 0; j < width * height; j++) {
            unflattened[i][j] = input[idx];
            idx++;
        }
    }
    return unflattened;
}