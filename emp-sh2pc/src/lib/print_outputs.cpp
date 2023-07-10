#include "print_outputs.h"

void print_3D_output(u64** output, int height, int width, int channels, string filename) {
    ofstream File(filename);
    for (int chan = 0; chan < channels; chan++) {
        for (int row = 0; row < height; row++) {
            File << "[ ";
            int col = 0;
            for (; col < width-1; col++) {
                File << (int)output[chan][row*width + col] << ", ";
            }
            File << (int)output[chan][row*width + col] << " ]\n";
        }
    }
    File.close();
    File.clear();
}

void print_1D_output(u64* output, int len, string filename) {
    ofstream File(filename);
    File << "[ ";
    for (int i = 0; i < len; i++) {
        File << (int)output[i] << " ";
    }
    File << " ]";
}

void clear_files() { 
    filesystem::remove_all("output_files");
    filesystem::create_directory("./output_files");
}