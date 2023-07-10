#ifndef PRINT_OUTPUTS_H
#define PRINT_OUTPUTS_H

#include <iostream>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <fstream>
#include <sstream>
#include <string>
#include <filesystem>

typedef uint64_t u64;
using namespace std;

void print_3D_output(u64** output, int height, int width, int channels, string filename);
void print_1D_output(u64* output, int len, string filename);
void clear_files();

#endif