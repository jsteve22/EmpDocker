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

PaddedOutput pad(u64** input, int channels, int height, int width, int padding) {
    PaddedOutput output;

    // create empty padded 3D array
    u64*** padded = (u64***) malloc(sizeof(u64**) * channels);
    int padded_height = height + padding*2;
    int padded_width = width + padding*2;
    for (int i = 0; i < channels; i++) {
        padded[i] = (u64**) malloc(sizeof(u64*) * padded_height);
        for (int j = 0; j < padded_height; j++) {
            padded[i][j] = (u64*) malloc(sizeof(u64) * padded_width);
            for (int k = 0; k < padded_width; k++) {
                padded[i][j][k] = 0;
            } 
        }
    }

    // fill padded array
    for (int i = 0; i < channels; i++) {
        int idx = 0;
        for (int j = 0; j < height; j++) {
            for (int k = 0; k < width; k++) {
                padded[i][j + padding][k + padding] = input[i][idx]
                idx++;
            }
        }
    }

    // flatten to 2D array
    u64** flat_pad = (u64**) malloc(sizeof(u64*) * channels);
    for (int i = 0; i < channels; i++) {
        flat_pad[i] = (u64*) malloc(sizeof(u64) * padded_height * padded_width);
    }

    for (int i = 0; i < channels; i++) {
        int idx = 0;
        for (int j = 0; j < padded_height; j++) {
            for (int k = 0; k < padded_width; k++) {
                flat_pad[i][idx] = padded[i][j][k];
                idx++;
            }
        }
    }

    output.matrix = flat_pad;
    output.height = padded_height;
    output.width = padded_width;
    return output;
}

u64** residual(u64** input1, u64** input2, int channels, int height, int width) {
    u64** output = (u64**) malloc(sizeof(u64*) * channels);
    for (int i = 0; i < channels; i++) {
       output[i] = (u64*) malloc(sizeof(u64) * height * width);
    }

    for (int i = 0; i < channels; i++) {
        for (int j = 0; j < height * width; j++) {
            output[i][j] = input1[i][j] + input2[i][j];
        }
    }

    return output;
}