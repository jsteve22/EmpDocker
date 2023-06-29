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

#include "read_weights.h"

typedef uint64_t u64;
std::vector<double> iotime_ms;
std::vector<double> ANDgatetime_ms, XORgatetime_ms;
using namespace emp;
using namespace std;

int SCALE = 256;

struct ConvOutput {
    Metadata data;
    int output_h;
    int output_w;
    int output_chan;
    ClientShares client_shares;
};

struct MeanPoolOutput {
    u64* matrix;
    int height;
    int width;
};

MeanPoolOutput MeanPooling(int bitsize, int64_t* inputs_a, int height, int width, int window_size, int party = 0, unsigned dup_test=10) {
        MeanPoolOutput output_struct;
        vector<vector<Integer> > output(height, vector<Integer>(width, Integer(bitsize, 0, PUBLIC))); // Integer product(bitsize, 0, PUBLIC);
        // vector<Integer> zero_int(len, Integer(bitsize, 0, PUBLIC)); // Integer zero_int(bitsize, 0, PUBLIC);

        struct timeval t_start_a, t_end_a, t_end_run, t_end_reveal;
        vector<vector<Integer> > a(height, vector<Integer>(width));
        gettimeofday(&t_start_a, NULL);
        int idx = 0;
        for (int i = 0; i < height; i++) {
            for (int j = 0; j < width; ++j){
                a[i][j] = Integer(bitsize, party == ALICE ? inputs_a[idx] : 0, ALICE);
                idx++;
            }
        }


        Integer modulus = Integer(bitsize, PLAINTEXT_MODULUS, PUBLIC);
        Integer half_mod = Integer(bitsize, PLAINTEXT_MODULUS / 2, PUBLIC);

        gettimeofday(&t_end_a, NULL);
        for (int i = 0; i < height; i+=window_size) {
            for (int j = 0; j < width; j+=window_size){
                Integer total = Integer(bitsize, 0, PUBLIC);
                for (int k = 0; k < window_size; k++) {
                    for (int l = 0; l < window_size; l++) {
                        total = total + a[i + k][j + l];
                        //if ((total.geq(half_mod)) == 1) {
                          //  total = total - modulus;
                        //}
                    }
                } 
                output[i / window_size][j / window_size] = total / Integer(bitsize, window_size * window_size, PUBLIC);  
            }
        }

        gettimeofday(&t_end_run, NULL);
        u64* product_reveal = (u64*) malloc( (sizeof(u64) * height * width) );
        idx = 0;
        for (int i = 0; i < height; ++i) {
            for (int j = 0; j < width; ++j) {
                product_reveal[idx] = output[i][j].reveal<int>(ALICE);
                idx++;
            }
        }

        gettimeofday(&t_end_reveal, NULL);

        double mseconds_a = 1000 * (t_end_a.tv_sec - t_start_a.tv_sec) + (t_end_a.tv_usec - t_start_a.tv_usec) / 1000.0;
        double mseconds_run = (1000 * (t_end_run.tv_sec - t_end_a.tv_sec) + (t_end_run.tv_usec - t_end_a.tv_usec) / 1000.0)/dup_test;
        double mseconds_reveal = 1000 * (t_end_reveal.tv_sec - t_end_run.tv_sec) + (t_end_reveal.tv_usec - t_end_run.tv_usec) / 1000.0;
        double io_ms = 0;
        for (size_t i = 0; i < iotime_ms.size(); i++)
        {
                io_ms += iotime_ms[i]/dup_test;
        }

        printf("Running Party: %d\n", party);
        printf("%d %d Time of init a:            %f ms\n", party, getpid(), mseconds_a);
        printf("%d %d Time of run circuit:       %f ms\n", party, getpid(), mseconds_run);
        printf("%d %d Time of REAL RUN circuit:  %f ms\n", party, getpid(), mseconds_run-io_ms);
        printf("%d %d Time of reveal the output: %f ms\n", party, getpid(), mseconds_reveal);

        output_struct.matrix = product_reveal;
        output_struct.height = height / window_size;
        output_struct.width = width / window_size;

        return output_struct;
}

u64* ReLU(int bitsize, int64_t* inputs_a, int len, int party = 0, unsigned dup_test=10)
{ // void ReLU(int bitsize, string inputs_a, string inputs_b) {

        vector<Integer> product(len, Integer(bitsize, 0, PUBLIC)); // Integer product(bitsize, 0, PUBLIC);
        vector<Integer> zero_int(len, Integer(bitsize, 0, PUBLIC)); // Integer zero_int(bitsize, 0, PUBLIC);

        //     int64_t _inputs_a = stoi(inputs_a), _inputs_b = stoi(inputs_b), _inputs_r = stoi(inputs_r);

        struct timeval t_start_a, t_end_a, t_end_run, t_end_reveal;
        vector<Integer> a(len);
        gettimeofday(&t_start_a, NULL);
        for (int j = 0; j < len; ++j){
                a[j] = Integer(bitsize, party == ALICE ? inputs_a[j] : 0, ALICE);
        }
        // Integer a(bitsize, party == ALICE ? inputs_a : 0, ALICE);
        gettimeofday(&t_end_a, NULL);
        for(unsigned m = 0; m < dup_test; ++m){ // 10 duplicate test
                for (int j = 0; j < len; ++j){
                        // ifThenElse function in file emp-tool/emp-tool/circuits/integer.hpp
                        // ifThenElse(Bit *dest, const Bit *tsrc, const Bit *fsrc, int size, Bit cond)
                        ifThenElse(&product[j].bits[0], &a[j].bits[0], &zero_int[j].bits[0], bitsize, a[j].geq(zero_int[j]));
                }
        }
        gettimeofday(&t_end_run, NULL);
        u64* product_reveal = (u64*) malloc( (sizeof(u64) * len) );
        for (int j = 0; j < len; ++j){
                product_reveal[j] = product[j].reveal<int>(ALICE);
        }
        gettimeofday(&t_end_reveal, NULL);
        /*
        for (int j = 0; j < len; ++j){
                cout<< "Running Party: " <<party<< " ReLU(" << inputs_a[j] << ") = " << product_reveal[j] << endl;
        }
        */

        double mseconds_a = 1000 * (t_end_a.tv_sec - t_start_a.tv_sec) + (t_end_a.tv_usec - t_start_a.tv_usec) / 1000.0;
        double mseconds_run = (1000 * (t_end_run.tv_sec - t_end_a.tv_sec) + (t_end_run.tv_usec - t_end_a.tv_usec) / 1000.0)/dup_test;
        double mseconds_reveal = 1000 * (t_end_reveal.tv_sec - t_end_run.tv_sec) + (t_end_reveal.tv_usec - t_end_run.tv_usec) / 1000.0;
        double io_ms = 0;
        for (size_t i = 0; i < iotime_ms.size(); i++)
        {
                io_ms += iotime_ms[i]/dup_test;
        }

        printf("Running Party: %d\n", party);
        printf("%d %d Time of init a:            %f ms\n", party, getpid(), mseconds_a);
        printf("%d %d Time of run circuit:       %f ms\n", party, getpid(), mseconds_run);
        printf("%d %d Time of REAL RUN circuit:  %f ms\n", party, getpid(), mseconds_run-io_ms);
        printf("%d %d Time of reveal the output: %f ms\n", party, getpid(), mseconds_reveal);

        return product_reveal;
}


vector<vector<vector<int> > > read_image(string file) {
    ifstream fp;
    string line;
    vector<vector<vector<int> > > image_data;

    fp.open(file);
    if (fp.is_open()) {
        getline(fp, line);

        int i = 0;
        string dim = "";
        vector<int> dims;
        while (line[i] != '\0') {
            if (line[i] != ' ') {
                dim += line[i];
            }
            else {
                dims.push_back(stoi(dim));
                dim = "";
            }
            i++;
        }

        string line;
        getline(fp, line);
        string num = "";

        int m = 0;
        for (int i = 0; i < dims[0]; i++) {
            vector<vector<int> > empty2;
            image_data.push_back(empty2);
            for (int j = 0; j < dims[1]; j++) {
                vector<int> empty1;
                image_data[i].push_back(empty1);
                for (int k = 0; k < dims[2]; k++) {
                    while (line[m] != '\0') {
                        if (line[m] != ' ') {
                            num += line[m];
                        }
                        else {
                            image_data[i][j].push_back(stoi(num));
                            num = "";
                            m++;
                            break;
                        }
                        m++;
                    }
                }
            }
        }
        fp.close();
        return image_data;
    }
    cout << "file reading error\n";
    return image_data;
}

vector<int> read_weights_1(string file) {
    ifstream fp;
    string line;
    vector<int> data1;

    fp.open(file);
    if (fp.is_open()) {
        getline(fp, line);

        int i = 0;
        string dim = "";
        vector<int> dims;
        while (line[i] != '\0') {
            if (line[i] != ' ') {
                dim += line[i];
            }
            else {
                dims.push_back(stoi(dim));
                dim = "";
            }
            i++;
        }

        if (dims.size() == 1) {
            //
            string line;
            getline(fp, line);
            string num = "";

            int j = 0;
            for (int i = 0; i < dims[0]; i++) {
                while (line[j] != '\0') {
                    if (line[j] != ' ') {
                        num += line[j];
                    }
                    else {
                        data1.push_back(stoi(num));
                        num = "";
                    }
                    j++;
                }
            }
            fp.close();
            return data1;
        }
    }
    cout << "file reading error\n";
    return data1;
}

vector<vector<int> > read_weights_2(string file) {
    ifstream fp;
    string line;
    vector<vector<int> > data2;

    fp.open(file);
    if (fp.is_open()) {
        getline(fp, line);

        int i = 0;
        string dim = "";
        vector<int> dims;
        while (line[i] != '\0') {
            if (line[i] != ' ') {
                dim += line[i];
            }
            else {
                dims.push_back(stoi(dim));
                dim = "";
            }
            i++;
        }

        if (dims.size() == 2) {
            //
            string line;
            getline(fp, line);
            string num = "";

            int k = 0;
            for (int i = 0; i < dims[0]; i++) {
                vector<int> empty;
                data2.push_back(empty);
                for (int j = 0; j < dims[1]; j++) {
                    while (line[k] != '\0') {
                        if (line[k] != ' ') {
                            num += line[k];
                        }
                        else {
                            data2[i].push_back(stoi(num));
                            num = "";
                            k++;
                            break;
                        }
                        k++;
                    }
                }
            }
            fp.close();
            return data2;
        }
    }
    cout << "file reading error\n";
    return data2;
}

vector<vector<vector<vector<int> > > > read_weights_4(string file) {
    ifstream fp;
    string line;
    vector<vector<vector<vector<int> > > > data4;

    fp.open(file);
    if (fp.is_open()) {
        getline(fp, line);

        int i = 0;
        string dim = "";
        vector<int> dims;
        while (line[i] != '\0') {
            if (line[i] != ' ') {
                dim += line[i];
            }
            else {
                dims.push_back(stoi(dim));
                dim = "";
            }
            i++;
        }

        if (dims.size() == 4) {
            string line;
            getline(fp, line);
            string num = "";

            int m = 0;
            for (int i = 0; i < dims[0]; i++) {
                vector<vector<vector<int> > > empty3;
                data4.push_back(empty3);
                for (int j = 0; j < dims[1]; j++) {
                    vector<vector<int> > empty2;
                    data4[i].push_back(empty2);
                    for (int k = 0; k < dims[2]; k++) {
                        vector<int> empty1;
                        data4[i][j].push_back(empty1);
                        for (int l = 0; l < dims[3]; l++) {
                            while (line[m] != '\0') {
                                if (line[m] != ' ') {
                                    num += line[m];
                                }
                                else {
                                    data4[i][j][k].push_back(stoi(num));
                                    num = "";
                                    m++;
                                    break;
                                }
                                m++;
                            }
                        }
                    }
                }
            }
            fp.close();
            return data4;
        }
    }
    cout << "file reading error\n";
    return data4;
}


ConvOutput conv(ClientFHE* cfhe, ServerFHE* sfhe, int image_h, int image_w, int filter_h, int filter_w,
    int inp_chans, int out_chans, int stride, bool pad_valid, string weights_filename, u64** input_data=NULL) {
    Metadata data = conv_metadata(cfhe->encoder, image_h, image_w, filter_h, filter_w, inp_chans, 
        out_chans, stride, stride, pad_valid);
    string filename;
   
    printf("\nClient Preprocessing: ");
    float origin = (float)clock()/CLOCKS_PER_SEC;

    u64** input = (u64**) malloc(sizeof(u64*)*data.inp_chans);
    for (int chan = 0; chan < data.inp_chans; chan++) {
        input[chan] = (u64*) malloc(sizeof(u64)*data.image_size);
    }

    if (input_data == NULL) {
        vector<vector<vector<int> > > image_data;
        filename = "cifar_image.txt";
        //filename = "mnist_image_2.txt";
        image_data = read_image(filename);
        int height = image_data[0].size();
        int width = image_data[0][0].size();

        for (int chan = 0; chan < data.inp_chans; chan++) {
           int idx = 0;
           for (int i = 0; i < height; i++) {
               for (int j = 0; j < width; j++) {
                   input[chan][idx] = image_data[chan][i][j] + PLAINTEXT_MODULUS;
                   idx++;
               }
           }  
        }
    }
    else {
        for (int chan = 0; chan < data.inp_chans; chan++) {
           for (int i = 0; i < data.image_h * data.image_w; i++) {
                input[chan][i] = input_data[chan][i] + PLAINTEXT_MODULUS;
           }  
        }
        for (int i = 0; i < data.inp_chans; i++) {
            free(input_data[i]);
        }
        free(input_data);
    }

    // print client pre-processing
    /*
    for (int chan = 0; chan < data.inp_chans; chan++) {
        for (int row = 0; row < data.image_h; row++) {
            printf(" [");
            int col = 0;
            for (; col < data.image_w-1; col++) {
                printf("%d, " , input[chan][row*data.image_w + col]);
            }
            printf("%d ]\n" , input[chan][row*data.image_w + col]);
        }
        printf("\n");
    }
    */
    

    ClientShares client_shares = client_conv_preprocess(cfhe, &data, input);

    float endTime = (float)clock()/CLOCKS_PER_SEC;
    float timeElapsed = endTime - origin;
    printf("[%f seconds]\n", timeElapsed);

    vector<vector<vector<vector<int> > > > data4;
    //filename = "conv2d.kernel.txt";
    data4 = read_weights_4(weights_filename);
    int width = data4[0][0].size();
    int height = data4[0][0][0].size();

    printf("Server Preprocessing: ");
    float startTime = (float)clock()/CLOCKS_PER_SEC;

    // Server creates filter
    u64*** filters = (u64***) malloc(sizeof(u64**)*data.out_chans);
    for (int out_c = 0; out_c < data.out_chans; out_c++) {
        filters[out_c] = (u64**) malloc(sizeof(u64*)*data.inp_chans);
        for (int inp_c = 0; inp_c < data.inp_chans; inp_c++) {
            filters[out_c][inp_c] = (u64*) malloc(sizeof(u64)*data.filter_size);
            int idx = 0;
            for (int w = 0; w < width; w++) {
                for (int h = 0; h < height; h++) {
                    filters[out_c][inp_c][idx] = data4[out_c][inp_c][w][h] + PLAINTEXT_MODULUS;
                    idx++;
                }
            }
        }
    }


    uint64_t** linear_share = (uint64_t**) malloc(sizeof(uint64_t*)*data.out_chans);
    
    for (int chan = 0; chan < data.out_chans; chan++) {
        linear_share[chan] = (uint64_t*) malloc(sizeof(uint64_t)*data.output_h*data.output_w);
        for (int idx = 0; idx < data.output_h*data.output_w; idx++) {
            // TODO: Adjust these for testing
            linear_share[chan][idx] = 0;
        }
    }

    char**** masks = server_conv_preprocess(sfhe, &data, filters); 
    ServerShares server_shares = server_conv_preprocess_shares(sfhe, &data, linear_share);

    endTime = (float)clock()/CLOCKS_PER_SEC;
    timeElapsed = endTime - startTime;
    printf("[%f seconds]\n", timeElapsed);

    printf("Convolution: ");
    startTime = (float)clock()/CLOCKS_PER_SEC;

    server_conv_online(sfhe, &data, client_shares.input_ct, masks, &server_shares);

    endTime = (float)clock()/CLOCKS_PER_SEC;
    timeElapsed = endTime - startTime;
    printf("[%f seconds]\n", timeElapsed);

    printf("Post process: ");
    startTime = (float)clock()/CLOCKS_PER_SEC;

    // This simulates the client receiving the ciphertexts 
    client_shares.linear_ct.inner = (char*) malloc(sizeof(char)*server_shares.linear_ct.size);
    client_shares.linear_ct.size = server_shares.linear_ct.size;
    memcpy(client_shares.linear_ct.inner, server_shares.linear_ct.inner, server_shares.linear_ct.size);
    client_conv_decrypt(cfhe, &data, &client_shares);

    endTime = (float)clock()/CLOCKS_PER_SEC;
    timeElapsed = endTime - startTime;
    printf("[%f seconds]\n", timeElapsed);

    timeElapsed = endTime - origin;
    printf("Total [%f seconds]\n\n", timeElapsed);

    
    
    for (int chan = 0; chan < data.out_chans; chan++) {
        for (int idx = 0; idx < data.output_h * data.output_w; idx++) {
            if (client_shares.linear[chan][idx] > PLAINTEXT_MODULUS / 2) { // center lift
                client_shares.linear[chan][idx] -= PLAINTEXT_MODULUS;
            }
            u64 mask = 0x8000000000000000;
            if (mask & client_shares.linear[chan][idx]) { // if negative
                u64 temp_pos = ~(client_shares.linear[chan][idx]) + 1; // make positive
                temp_pos = temp_pos / SCALE; // scale
                temp_pos = ~(temp_pos) + 1; // make negative
                client_shares.linear[chan][idx] = temp_pos + PLAINTEXT_MODULUS - 1;
            }
            else {
                client_shares.linear[chan][idx] = client_shares.linear[chan][idx] / SCALE;
            }
        }
    }


    
    
    
    /*
    cout << "CONV OUTPUT\n";
    for (int idx = 0; idx < 150; idx++) {
        if (client_shares.linear[0][idx] > PLAINTEXT_MODULUS / 2) {
            printf("%lld ", client_shares.linear[0][idx] - PLAINTEXT_MODULUS);
        }
        else {
            printf("%lld ", client_shares.linear[0][idx]);
        }
    }
    */

    

    /*
    printf("RESULT: \n");
    for (int chan = 0; chan < data.out_chans; chan++) {
        int idx = 0;
        for (int row = 0; row < data.output_h; row++) {
            printf(" [");
            int col = 0;
            for (; col < data.output_w-1; col++) {
                printf("%d, " , client_shares.linear[chan][row*data.output_w + col]);
            }
            printf("%d ]\n" , client_shares.linear[chan][row*data.output_w + col]);
        }
        printf("\n");
    }
    */
    
    // Free filters
    for (int out_c = 0; out_c < data.out_chans; out_c++) {
        for (int inp_c = 0; inp_c < data.inp_chans; inp_c++)
            free(filters[out_c][inp_c]);
      free(filters[out_c]);
    }
    free(filters);

    // Free image
    for (int chan = 0; chan < data.inp_chans; chan++) {
        free(input[chan]);
    }
    free(input);

    // Free secret shares
    for (int chan = 0; chan < data.out_chans; chan++) {
        free(linear_share[chan]);
    }
    free(linear_share);

    ConvOutput conv_output;
    conv_output.data = data;
    conv_output.output_h = data.output_h;
    conv_output.output_w = data.output_w;
    conv_output.output_chan = data.out_chans;
    conv_output.client_shares = client_shares;
    
    // Free C++ allocations
    //free(client_shares.linear_ct.inner);
    //client_conv_free(&data, &client_shares);
    //server_conv_free(&data, masks, &server_shares);
    return conv_output;
}

u64* fc(ClientFHE* cfhe, ServerFHE* sfhe, int vector_len, int matrix_h, u64* dense_input, string weights_filename, int split_index=0) {
    Metadata data = fc_metadata(cfhe->encoder, vector_len, matrix_h);
   
    printf("\nClient Preprocessing: ");
    float origin = (float)clock()/CLOCKS_PER_SEC;

    u64* input = (u64*) malloc(sizeof(u64)*vector_len);
    for (int idx = 0; idx < vector_len; idx++)
        //input[idx] = 1;
        input[idx] = dense_input[idx];

    ClientShares client_shares = client_fc_preprocess(cfhe, &data, input);

    float endTime = (float)clock()/CLOCKS_PER_SEC;
    float timeElapsed = endTime - origin;
    printf("[%f seconds]\n", timeElapsed);

    printf("Server Preprocessing: ");
    float startTime = (float)clock()/CLOCKS_PER_SEC;

    vector<vector<int> > data2;
    // string filename = "dense.kernel.txt";
    data2 = read_weights_2(weights_filename);

    u64** matrix = (u64**) malloc(sizeof(u64*)*matrix_h);
    for (int ct = 0; ct < matrix_h; ct++) {
        matrix[ct] = (u64*) malloc(sizeof(u64)*vector_len);
        for (int vecs = 0; vecs < vector_len; vecs++) {
            matrix[ct][vecs] = data2[ct][vecs] + PLAINTEXT_MODULUS;
        }
    }

    u64** split_matrix = (u64**) malloc(sizeof(u64*)*matrix_h);
    for (int ct = 0; ct < matrix_h; ct++) {
        split_matrix[ct] = matrix[ct] + split_index;
    }

    cout << "vector len: " << vector_len << "\n";

    uint64_t* linear_share = (uint64_t*) malloc(sizeof(uint64_t)*matrix_h);
    for (int idx = 0; idx < matrix_h; idx++) {
        linear_share[idx] = 0;
    }

    char** enc_matrix = server_fc_preprocess(sfhe, &data, split_matrix); 
    ServerShares server_shares = server_fc_preprocess_shares(sfhe, &data, linear_share);
    
    endTime = (float)clock()/CLOCKS_PER_SEC;
    timeElapsed = endTime - startTime;
    printf("[%f seconds]\n", timeElapsed);

    printf("Layer: ");
    startTime = (float)clock()/CLOCKS_PER_SEC;

    server_fc_online(sfhe, &data, client_shares.input_ct, enc_matrix, &server_shares);

    endTime = (float)clock()/CLOCKS_PER_SEC;
    timeElapsed = endTime - startTime;
    printf("[%f seconds]\n", timeElapsed);

    // This simulates the client receiving the ciphertexts 
    client_shares.linear_ct.inner = (char*) malloc(sizeof(char)*server_shares.linear_ct.size);
    client_shares.linear_ct.size = server_shares.linear_ct.size;
    memcpy(client_shares.linear_ct.inner, server_shares.linear_ct.inner, server_shares.linear_ct.size);

    printf("Post process: ");
    startTime = (float)clock()/CLOCKS_PER_SEC;

    client_fc_decrypt(cfhe, &data, &client_shares);

    endTime = (float)clock()/CLOCKS_PER_SEC;
    timeElapsed = endTime - startTime;
    printf("[%f seconds]\n", timeElapsed);

    timeElapsed = endTime - origin;
    printf("Total [%f seconds]\n\n", timeElapsed);

    /*
    printf("Matrix: [\n");
    for (int i = 0; i < matrix_h; i++) {
        for (int j = 0; j < vector_len; j++)
            printf("%d, " , matrix[i][j]);
        printf("\n");
    }
    printf("] \n");

    

    printf("Input: [");
    for (int j = 0; j < vector_len; j++)
        printf("%d, " , input[j]);
    printf("] \n");
    */

    return client_shares.linear[0];

    // Free matrix
    for (int row = 0; row < matrix_h; row++)
        free(matrix[row]);
    free(matrix);

    // Free vector
    free(input);

    // Free secret shares
    //free(client_shares.linear_ct.inner);
    free(linear_share);

    // Free C++ allocations
    //client_fc_free(&client_shares);
    server_fc_free(&data, enc_matrix, &server_shares);

}

u64 reduce(unsigned __int128 x) {
    return x % PLAINTEXT_MODULUS;
}

u64 mul(u64 x, u64 y) {
    return (u64)(((unsigned __int128)x * y) % PLAINTEXT_MODULUS);
}


void beavers_triples(ClientFHE* cfhe, ServerFHE* sfhe, int num_triples) {
    printf("\nPreprocessing: ");
    float origin = (float)clock()/CLOCKS_PER_SEC;
    
    u64 modulus = PLAINTEXT_MODULUS;

    // Generate and encrypt client's shares of a and b
    u64* client_a = (u64*) malloc(sizeof(u64)*num_triples);
    u64* client_b = (u64*) malloc(sizeof(u64)*num_triples);
    for (int idx = 0; idx < num_triples; idx++) {
        // Only 20 bits so we don't need 128 bit multiplication
        client_a[idx] = rand() % modulus;
        client_b[idx] = rand() % modulus;
    }
    
    ClientTriples client_shares = client_triples_preprocess(cfhe, num_triples, client_a, client_b);
    
    // Generate and encrypt server's shares of a, b, and c
    u64* server_a = (u64*) malloc(sizeof(u64)*num_triples);
    u64* server_b = (u64*) malloc(sizeof(u64)*num_triples);
    u64* server_c = (u64*) malloc(sizeof(u64)*num_triples);
    u64* server_r = (u64*) malloc(sizeof(u64)*num_triples);
    for (int idx = 0; idx < num_triples; idx++) {
        server_a[idx] = rand() % modulus;
        server_b[idx] = rand() % modulus;
        server_c[idx] = rand() % modulus;
        // We first add the modulus so that we don't underflow the u64
        server_r[idx] = ((modulus + mul(server_a[idx], server_b[idx])) - server_c[idx]) % modulus;
    }
    
    ServerTriples server_shares = server_triples_preprocess(sfhe, num_triples, server_a, server_b,
        server_r);

    float endTime = (float)clock()/CLOCKS_PER_SEC;
    float timeElapsed = endTime - origin;
    printf("[%f seconds]\n", timeElapsed);

    printf("Online: ");
    float startTime = (float)clock()/CLOCKS_PER_SEC;

    server_triples_online(sfhe, client_shares.a_ct, client_shares.b_ct, &server_shares);
    
    endTime = (float)clock()/CLOCKS_PER_SEC;
    timeElapsed = endTime - startTime;
    printf("[%f seconds]\n", timeElapsed);

    printf("Post process: ");
    startTime = (float)clock()/CLOCKS_PER_SEC;
    client_triples_decrypt(cfhe, server_shares.c_ct, &client_shares);

    endTime = (float)clock()/CLOCKS_PER_SEC;
    timeElapsed = endTime - startTime;
    printf("[%f seconds]\n", timeElapsed);

    bool correct = true;
    printf("\nA    : [");
    for (int i = 0; i < num_triples; i++) {
        printf("%llu, " , reduce(client_a[i] + server_a[i]));
    }
    printf("] \n");

    printf("B    : [");
    for (int i = 0; i < num_triples; i++) {
        printf("%llu, " , reduce(client_b[i] + server_b[i]));
    }
    printf("] \n");

    printf("C    : [");
    for (int i = 0; i < num_triples; i++) {
        u64 a = client_a[i] + server_a[i];
        u64 b = client_b[i] + server_b[i];
        u64 c = reduce(client_shares.c_share[i] + server_c[i]);
        printf("%llu, " , c);
        correct &= (c == mul(a,b));
    }
    printf("] \n");

    printf("\nCorrect: %d\n", correct);

    timeElapsed = endTime - origin;
    printf("Total [%f seconds]\n\n", timeElapsed);

    free(client_a);
    free(client_b);
    free(server_a);
    free(server_b);
    free(server_c);
    free(server_r);
    client_triples_free(&client_shares);
    server_triples_free(&server_shares);
}

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


int main(int argc, char* argv[]) {
  int bitsize = 32;
  int LEN = 50176;
  int port, party;
  parse_party_and_port(argv, &party, &port);
  NetIO *io = new NetIO(party == ALICE ? nullptr : "127.0.0.1", port);

  setup_semi_honest(io, party);


  cout << "Calculating ReLU of an array length = " << LEN << ", bitsize =" << bitsize << endl;

  if (party != ALICE) {
    u64* temp;
    u64* dense_input = (u64*) malloc( sizeof(u64) * LEN );
    u64* dense_input_2 = (u64*) malloc( sizeof(u64) * LEN / 4 );
    MeanPoolOutput mean_temp;
    temp = ReLU( bitsize, (int64_t*) dense_input, LEN, party);
    temp = ReLU( bitsize, (int64_t*) dense_input, LEN, party);
    for (int chan = 0; chan < 64; chan++) {
        mean_temp = MeanPooling(bitsize, (int64_t*) dense_input, 32, 32, 2, party);
    }
    temp = ReLU( bitsize, (int64_t*) dense_input, LEN, party);
    temp = ReLU( bitsize, (int64_t*) dense_input, LEN, party);
    for (int chan = 0; chan < 3; chan++) {
        mean_temp = MeanPooling(bitsize, (int64_t*) dense_input_2, 16, 16, 2, party);
    }
    return 0;
    temp = ReLU( bitsize, (int64_t*) dense_input, LEN, party);
    temp = ReLU( bitsize, (int64_t*) dense_input, LEN, party);
    temp = ReLU( bitsize, (int64_t*) dense_input, LEN, party);
    free(temp);
    free(dense_input);
    finalize_semi_honest();
    delete io;
    return 0;
  }

  SerialCT key_share;

  printf("Client Keygen: ");
  float startTime = (float)clock()/CLOCKS_PER_SEC;

  ClientFHE cfhe = client_keygen(&key_share);

  float endTime = (float)clock()/CLOCKS_PER_SEC;
  float timeElapsed = endTime - startTime;
  printf("[%f seconds]\n", timeElapsed);
 
  printf("Server Keygen: ");
  startTime = (float)clock()/CLOCKS_PER_SEC;
  
  ServerFHE sfhe = server_keygen(key_share); 

  endTime = (float)clock()/CLOCKS_PER_SEC;
  timeElapsed = endTime - startTime;
  printf("[%f seconds]\n", timeElapsed);

  ConvOutput conv_output;
  MeanPoolOutput meanpool_output;
  u64* flattened;
  u64** unflattened;
  u64* relu_output;
  u64** meanpool_matrix = (u64**) malloc(sizeof(u64*) * 64) ;
  u64* dense_input;
  u64* temp;

  int height = 32;
  int width = 32;
  int channels = 3;


    /*
  conv_output = conv(&cfhe, &sfhe, 28, 28, 3, 3, 1, 64, 1, 0, "multi.conv2d.kernel.txt");
  cout << "CONV OUTPUT \n";
  for (int idx = 0; idx < 150; idx++) {
    cout << conv_output.client_shares.linear[0][idx] << " ";
  }
  cout << "height, width: " << conv_output.output_h << " " << conv_output.output_w << "\n";


  for (int chan = 0; chan < 1; chan++) {
    meanpool_output = MeanPooling(bitsize, (int64_t*) conv_output.client_shares.linear[0], 28, 28, 2, ALICE);
    meanpool_matrix[chan] = meanpool_output.matrix;
  }


  cout << "MEAN POOL OUTPUT \n";
  for (int idx = 0; idx < 150; idx++) {
    printf("%d ", meanpool_matrix[0][idx]);
  }
    return 0;
    */



  // 4 layer MNIST CNN convolutional layers
  /*
  conv_output = conv(&cfhe, &sfhe, 28, 28, 3, 3, 1, 64, 1, 0, "multi.conv2d.kernel.txt");
  conv_output = conv(&cfhe, &sfhe, conv_output.output_h, conv_output.output_w, 3, 3, conv_output.output_chan, conv_output.output_chan, 1, 0, "conv2d_1.kernel.txt", conv_output.client_shares.linear);
  conv_output = conv(&cfhe, &sfhe, conv_output.output_h, conv_output.output_w, 3, 3, conv_output.output_chan, conv_output.output_chan, 1, 0, "conv2d_2.kernel.txt", conv_output.client_shares.linear);
  conv_output = conv(&cfhe, &sfhe, conv_output.output_h, conv_output.output_w, 3, 3, conv_output.output_chan, conv_output.output_chan, 1, 0, "conv2d_3.kernel.txt", conv_output.client_shares.linear);
  int vec_len = conv_output.output_w * conv_output.output_h * conv_output.output_chan;
  dense_input = flatten(conv_output.client_shares.linear, conv_output.output_chan, conv_output.output_h, conv_output.output_w);
  temp = ReLU( bitsize, (int64_t*) dense_input, LEN, party);
  */

 // CIFAR-10 CNN
  // 1) Convolution: input image 3 × 32 × 32, window size 3 × 3, stride (1,1), pad (1, 1), number of output channels 64: R64×1024 ← R64×27·R27×1024.
  conv_output = conv(&cfhe, &sfhe, height, width, 3, 3, channels, 64, 1, 0, "miniONN_cifar_model/conv2d.kernel.txt");
  height = conv_output.output_h;
  width = conv_output.output_w;
  channels = conv_output.output_chan;
  // 2) ReLU Activation: calculates ReLU for each input
  flattened = flatten(conv_output.client_shares.linear, channels, height, width);
  relu_output = ReLU(bitsize, (int64_t*) flattened, LEN, party);
  // 3) Convolution: window size 3 × 3, stride (1, 1), pad (1, 1), number of output channels 64: R64×1024 ← R64×576· R576×1024.
  unflattened = unflatten(relu_output, channels, height, width);
  conv_output = conv(&cfhe, &sfhe, height, width, 3, 3, channels, 64, 1, 0, "miniONN_cifar_model/conv2d_1.kernel.txt", unflattened);
  height = conv_output.output_h;
  width = conv_output.output_w;
  channels = conv_output.output_chan;
  // 4) ReLU Activation: calculates ReLU for each input.
  flattened = flatten(conv_output.client_shares.linear, channels, height, width);
  relu_output = ReLU(bitsize, (int64_t*) flattened, LEN, party);

  // 5) Mean Pooling: window size 1 × 2 × 2, outputs R64×16×16.
  unflattened = unflatten(relu_output, channels, height, width);
  for (int chan = 0; chan < channels; chan++) {
    meanpool_output = MeanPooling(bitsize, (int64_t*) unflattened[chan], height, width, 2, party);
    meanpool_matrix[chan] = meanpool_output.matrix;
  }
  width = meanpool_output.width;
  height = meanpool_output.height;
  // 6) Convolution: window size 3 × 3, stride (1, 1), pad (1, 1), number of output channels 64: R64×256 ← R64×576· R576×256.
  conv_output = conv(&cfhe, &sfhe, height, width, 3, 3, channels, 64, 1, 0, "miniONN_cifar_model/conv2d_2.kernel.txt", meanpool_matrix);
  // 7) ReLU Activation: calculates ReLU for each input.
  flattened = flatten(conv_output.client_shares.linear, channels, height, width);
  relu_output = ReLU(bitsize, (int64_t*) flattened, LEN, party);
  // 8) Convolution: window size 3 × 3, stride (1, 1), pad (1, 1), number of output channels 64: R64×256 ← R64×576· R576×256.
  unflattened = unflatten(relu_output, channels, height, width);
  conv_output = conv(&cfhe, &sfhe, height, width, 3, 3, channels, 64, 1, 0, "miniONN_cifar_model/conv2d_3.kernel.txt", unflattened);
  height = conv_output.output_h;
  width = conv_output.output_w;
  channels = conv_output.output_chan;
  // 9) ReLU Activation: calculates ReLU for each input
  flattened = flatten(conv_output.client_shares.linear, channels, height, width);
  relu_output = ReLU(bitsize, (int64_t*) flattened, LEN, party);
  // 10) Mean Pooling: window size 1 × 2 × 2, outputs R64×16×16.
  unflattened = unflatten(relu_output, channels, height, width);
  for (int chan = 0; chan < 3; chan++) {
    printf("meanpool %d ", chan);
    meanpool_output = MeanPooling(bitsize, (int64_t*) unflattened[chan], height, width, 2, party);
    meanpool_matrix[chan] = meanpool_output.matrix;
  }
  width = meanpool_output.width;
  height = meanpool_output.height;
  return 0;
  // 11) Convolution: window size 3 × 3, stride (1, 1), pad (1, 1), number of output channels 64: R64×64 ← R64×576· R576×64.
  conv_output = conv(&cfhe, &sfhe, height, width, 3, 3, channels, 64, 1, 0, "miniONN_cifar_model/conv2d_4.kernel.txt", meanpool_matrix);
  height = conv_output.output_h;
  width = conv_output.output_w;
  channels = conv_output.output_chan;
  // 12) ReLU Activation: calculates ReLU for each input.
  flattened = flatten(conv_output.client_shares.linear, channels, height, width);
  relu_output = ReLU(bitsize, (int64_t*) flattened, LEN, party);
  // 13) Convolution: window size 1 × 1, stride (1, 1), number of output channels of 64: R64×64 ← R64×64· R64×64.
  unflattened = unflatten(relu_output, channels, height, width);
  conv_output = conv(&cfhe, &sfhe, height, width, 3, 3, channels, 64, 1, 1, "miniONN_cifar_model/conv2d_5.kernel.txt", unflattened);
  height = conv_output.output_h;
  width = conv_output.output_w;
  channels = conv_output.output_chan;
  // 14) ReLU Activation: calculates ReLU for each input.
  flattened = flatten(conv_output.client_shares.linear, channels, height, width);
  relu_output = ReLU(bitsize, (int64_t*) flattened, LEN, party);
  // 15) Convolution: window size 1 × 1, stride (1, 1), number of output channels of 16: R16×64 ← R16×64·R64×64.
  unflattened = unflatten(relu_output, channels, height, width);
  conv_output = conv(&cfhe, &sfhe, height, width, 3, 3, channels, 64, 1, 1, "miniONN_cifar_model/conv2d_6.kernel.txt", unflattened);
  height = conv_output.output_h;
  width = conv_output.output_w;
  channels = conv_output.output_chan;
  // 16) ReLU Activation: calculates ReLU for each input.
  flattened = flatten(conv_output.client_shares.linear, channels, height, width);
  relu_output = ReLU(bitsize, (int64_t*) flattened, LEN, party);
  // 17) Fully Connected Layer: fully connects the incoming 1024 nodes to the outgoing 10 nodes: R10×1 ← R10×1024· R1024×1.
  int vec_len = width * height * channels;
  //fc(&cfhe, &sfhe, vec_len, num_vec, relu_output, "miniONN_cifar_model/dense.kernel.txt");

    return 0;
  cout << " RELU OUTPUT \n";
  for (int i = 0; i < 50; i++) {
    printf("%d ", temp[i]);
  }

  free(dense_input);
  dense_input = temp;

  int num_vec = 10;
  int64_t* output = (int64_t*) malloc(num_vec*sizeof(int64_t));
  for (int j = 0; j < num_vec; j++) {
    output[j] = 0;
  }

  /*
  int split = 4000;
  for (int i = 0; i < 4000; i+=split) {
    printf("min: %d\n", min(vec_len - i, split));
    temp = fc(&cfhe, &sfhe, min(vec_len - i, split), num_vec, dense_input + i, "multi.dense.kernel.txt", i);
    for (int j = 0; j < num_vec; j++) {
        output[j] += (int64_t) temp[j];
        if (output[j] > PLAINTEXT_MODULUS / 2) {
            output[j] -= PLAINTEXT_MODULUS;
        }
    }
  }
  */

  printf("Linear: [");
    for (int idx = 0; idx < num_vec; idx++) {
        printf("%d, " , output[idx]);
    }
    printf("] \n");
  
  
  //beavers_triples(&cfhe, &sfhe, 100);
  
  client_free_keys(&cfhe);
  free_ct(&key_share);
  server_free_keys(&sfhe);


  finalize_semi_honest();
  delete io;
  return 1;
}





