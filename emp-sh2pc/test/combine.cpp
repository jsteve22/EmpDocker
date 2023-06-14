/*
 *  Example of using Delphi Offline's C interface
 *
 *  Created on: June 10, 2019
 *      Author: ryanleh
 */
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

typedef uint64_t u64;
std::vector<double> iotime_ms;
std::vector<double> ANDgatetime_ms, XORgatetime_ms;
using namespace emp;
using namespace std;

void ReLU(int bitsize, vector<int> inputs_a, int len, int party = 0, unsigned dup_test=10)
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
                        ifThenElse(&product[j].bits[0], &a[j].bits[0], &zero_int[j].bits[0], bitsize, a[j].geq(zero_int[j]));
                }
        }
        gettimeofday(&t_end_run, NULL);
        vector<int> product_reveal(len);
        for (int j = 0; j < len; ++j){
                product_reveal[j] = product[j].reveal<int>(ALICE);
        }
        gettimeofday(&t_end_reveal, NULL);
        for (int j = 0; j < len; ++j){
                cout<< "Running Party: " <<party<< " ReLU(" << inputs_a[j] << ") = " << product_reveal[j] << endl;
        }

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
}

void conv(ClientFHE* cfhe, ServerFHE* sfhe, int image_h, int image_w, int filter_h, int filter_w,
    int inp_chans, int out_chans, int stride, bool pad_valid) {
    Metadata data = conv_metadata(cfhe->encoder, image_h, image_w, filter_h, filter_w, inp_chans, 
        out_chans, stride, stride, pad_valid);
   
    printf("\nClient Preprocessing: ");
    float origin = (float)clock()/CLOCKS_PER_SEC;

    u64** input = (u64**) malloc(sizeof(u64*)*data.inp_chans);
    for (int chan = 0; chan < data.inp_chans; chan++) {
        input[chan] = (u64*) malloc(sizeof(u64)*data.image_size);
        for (int idx = 0; idx < data.image_size; idx++)
            input[chan][idx] = idx;
    }

    for (int chan = 0; chan < data.inp_chans; chan++) {
        int idx = 0;
        for (int row = 0; row < data.image_h; row++) {
            printf(" [");
            int col = 0;
            for (; col < data.image_w-1; col++) {
                printf("%d, " , input[chan][row*data.output_w + col]);
            }
            printf("%d ]\n" , input[chan][row*data.output_w + col]);
        }
        printf("\n");
    }

    ClientShares client_shares = client_conv_preprocess(cfhe, &data, input);

    float endTime = (float)clock()/CLOCKS_PER_SEC;
    float timeElapsed = endTime - origin;
    printf("[%f seconds]\n", timeElapsed);


    printf("Server Preprocessing: ");
    float startTime = (float)clock()/CLOCKS_PER_SEC;

    // Server creates filter
    u64*** filters = (u64***) malloc(sizeof(u64**)*data.out_chans);
    for (int out_c = 0; out_c < data.out_chans; out_c++) {
        filters[out_c] = (u64**) malloc(sizeof(u64*)*data.inp_chans);
        for (int inp_c = 0; inp_c < data.inp_chans; inp_c++) {
            filters[out_c][inp_c] = (u64*) malloc(sizeof(u64)*data.filter_size);
            for (int idx = 0; idx < data.filter_size; idx++)
                filters[out_c][inp_c][idx] = 1;
        }
    }

    // Server creates additive secret share
    uint64_t** linear_share = (uint64_t**) malloc(sizeof(uint64_t*)*data.out_chans);
    
    for (int chan = 0; chan < data.out_chans; chan++) {
        linear_share[chan] = (uint64_t*) malloc(sizeof(uint64_t)*data.output_h*data.output_w);
        for (int idx = 0; idx < data.output_h*data.output_w; idx++) {
            // TODO: Adjust these for testing
            linear_share[chan][idx] = 4;
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
    
    // Free C++ allocations
    free(client_shares.linear_ct.inner);
    client_conv_free(&data, &client_shares);
    server_conv_free(&data, masks, &server_shares);
}

void fc(ClientFHE* cfhe, ServerFHE* sfhe, int vector_len, int matrix_h) {
    Metadata data = fc_metadata(cfhe->encoder, vector_len, matrix_h);
   
    printf("\nClient Preprocessing: ");
    float origin = (float)clock()/CLOCKS_PER_SEC;

    u64* input = (u64*) malloc(sizeof(u64)*vector_len);
    for (int idx = 0; idx < vector_len; idx++)
        input[idx] = 1;

    ClientShares client_shares = client_fc_preprocess(cfhe, &data, input);

    float endTime = (float)clock()/CLOCKS_PER_SEC;
    float timeElapsed = endTime - origin;
    printf("[%f seconds]\n", timeElapsed);

    printf("Server Preprocessing: ");
    float startTime = (float)clock()/CLOCKS_PER_SEC;

    u64** matrix = (u64**) malloc(sizeof(u64*)*matrix_h);
    for (int ct = 0; ct < matrix_h; ct++) {
        matrix[ct] = (u64*) malloc(sizeof(u64)*vector_len);
        for (int idx = 0; idx < vector_len; idx++)
            matrix[ct][idx] = ct*vector_len + idx;
    }

    uint64_t* linear_share = (uint64_t*) malloc(sizeof(uint64_t)*matrix_h);
    for (int idx = 0; idx < matrix_h; idx++) {
        linear_share[idx] = 0;
    }

    char** enc_matrix = server_fc_preprocess(sfhe, &data, matrix); 
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

    printf("Linear: [");
    for (int idx = 0; idx < matrix_h; idx++) {
        printf("%d, " , client_shares.linear[0][idx]);
    }
    printf("] \n");

    // Free matrix
    for (int row = 0; row < matrix_h; row++)
        free(matrix[row]);
    free(matrix);

    // Free vector
    free(input);

    // Free secret shares
    free(client_shares.linear_ct.inner);
    free(linear_share);

    // Free C++ allocations
    client_fc_free(&client_shares);
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


int main(int argc, char* argv[]) {
  int bitsize = 32;
  int LEN = 256;
  int port, party;
  parse_party_and_port(argv, &party, &port);
  NetIO *io = new NetIO(party == ALICE ? nullptr : "127.0.0.1", port);

  setup_semi_honest(io, party);

  if (argc != 5)
  {
          cout << "Usage: ./ReLU <party> <port> <bitsize> <length>" << endl
                << "where <value> are secret shares of the three inputs"
                << endl;
          delete io;
          return 1;
  }
  bitsize = atoi(argv[3]);
  LEN = atoi(argv[4]);
  cout << "Calculating ReLU of an array length = " << LEN << ", bitsize =" << bitsize << endl;


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

  //conv(&cfhe, &sfhe, 5, 5, 3, 3, 2, 2, 1, 0);
  //conv(&cfhe, &sfhe, 32, 32, 3, 3, 16, 16, 1, 0);
  //conv(&cfhe, &sfhe, 16, 16, 3, 3, 32, 32, 1, 1);
  //conv(&cfhe, &sfhe, 8, 8, 3, 3, 64, 64, 1, 1);
  
  fc(&cfhe, &sfhe, 25, 10);
  
  //beavers_triples(&cfhe, &sfhe, 100);
  
  client_free_keys(&cfhe);
  free_ct(&key_share);
  server_free_keys(&sfhe);

	int ia = 0;
	int ib = 0;

	if (party == ALICE) ia = 10;
	if (party == BOB) ib = 4;

	Integer a(32, ia, ALICE);
	Integer b(32, ib, BOB);

	Integer res = a - b;
	if (party == ALICE) {
		cout << "Alice: " << ia << endl << flush;
	} else if (party == BOB) {
		cout << "Bob: " << ib << endl << flush;
	}
	cout << "\t" << res.reveal<int>(PUBLIC) << endl << flush;

	finalize_semi_honest();
  delete io;
  return 1;
}
