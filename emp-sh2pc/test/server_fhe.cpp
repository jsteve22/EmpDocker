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

#include "read_txt.h"
#include "ReLU.h"
#include "mean_pooling.h"
#include "flattens.h"
//#include "conv_layer.h"
//#include "dense_layer.h"
#include "extras.h"
#include "helper_functions.h"
#include "print_outputs.h"

typedef uint64_t u64;
std::vector<double> iotime_ms;
std::vector<double> ANDgatetime_ms, XORgatetime_ms;
using namespace emp;
using namespace std;

extern int ciphertexts;
extern int result_ciphertexts;
extern int rot_count;
extern int multiplications;
extern int additions;
extern int subtractions;

struct ConvOutput {
    Metadata data;
    int output_h;
    int output_w;
    int output_chan;
    ClientShares client_shares;
};

ClientShares conv_pre(ClientFHE* cfhe, Metadata data, u64** input_data) {
  // client side
  printf("\nClient Preprocessing: ");
  float origin = (float)clock()/CLOCKS_PER_SEC;

  u64** input = (u64**) malloc(sizeof(u64*)*data.inp_chans);
  for (int chan = 0; chan < data.inp_chans; chan++) {
    input[chan] = (u64*) malloc(sizeof(u64)*data.image_size);
  }

  for (int chan = 0; chan < data.inp_chans; chan++) {
    for (int i = 0; i < data.image_h * data.image_w; i++) {
      input[chan][i] = input_data[chan][i] + PLAINTEXT_MODULUS;
    }  
  }
  for (int i = 0; i < data.inp_chans; i++) {
    free(input_data[i]);
  }
  free(input_data);

  ClientShares client_shares = client_conv_preprocess(cfhe, &data, input);
  float endTime = (float)clock()/CLOCKS_PER_SEC;
  float timeElapsed = endTime - origin;
  printf("Conv PreProcessing [%f seconds]\n", timeElapsed);

  // Free image
  for (int chan = 0; chan < data.inp_chans; chan++) {
    free(input[chan]);
  }
  free(input);

  return client_shares;
  // outputs -> client_shares
}

ClientShares conv_server(ServerFHE* sfhe, Metadata data, string weights_filename, ClientShares client_shares) {
  // both parties need data
  float origin = (float)clock()/CLOCKS_PER_SEC;
  float endTime;
  float timeElapsed;

  vector<vector<vector<vector<int> > > > data4;
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
      linear_share[chan][idx] = 300;
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
  printf("Conv processing time [%f seconds]\n", timeElapsed);

  // This simulates the client receiving the ciphertexts 
  client_shares.linear_ct.inner = (char*) malloc(sizeof(char)*server_shares.linear_ct.size);
  client_shares.linear_ct.size = server_shares.linear_ct.size;
  memcpy(client_shares.linear_ct.inner, server_shares.linear_ct.inner, server_shares.linear_ct.size);

  // Free filters
  for (int out_c = 0; out_c < data.out_chans; out_c++) {
    for (int inp_c = 0; inp_c < data.inp_chans; inp_c++)
      free(filters[out_c][inp_c]);
    free(filters[out_c]);
  }
  free(filters);
  
  // Free secret shares
  for (int chan = 0; chan < data.out_chans; chan++) {
    free(linear_share[chan]);
  }
  free(linear_share);

  server_conv_free(&data, masks, &server_shares);
  return client_shares;
}

ConvOutput conv_post(ClientFHE* cfhe, Metadata data, ClientShares client_shares) {
  float origin = (float)clock()/CLOCKS_PER_SEC;
  float endTime;
  float timeElapsed;
  float startTime;
  printf("Post process: ");
  startTime = (float)clock()/CLOCKS_PER_SEC;

  client_conv_decrypt(cfhe, &data, &client_shares);

  endTime = (float)clock()/CLOCKS_PER_SEC;
  timeElapsed = endTime - startTime;
  printf("[%f seconds]\n", timeElapsed);

  timeElapsed = endTime - origin;
  printf("Total [%f seconds]\n\n", timeElapsed);

  ConvOutput conv_output;
  conv_output.data = data;
  conv_output.output_h = data.output_h;
  conv_output.output_w = data.output_w;
  conv_output.output_chan = data.out_chans;
  conv_output.client_shares = client_shares;
  
  // Free C++ allocations
  //free(client_shares.linear_ct.inner);
  //client_conv_free(&data, &client_shares);
  return conv_output;
}



u64* fc(ClientFHE* cfhe, ServerFHE* sfhe, int vector_len, int matrix_h, u64* dense_input, string weights_filename, int split_index=0) {
    Metadata data = fc_metadata(cfhe->encoder, vector_len, matrix_h);
   
    printf("\nClient Preprocessing: ");
    float origin = (float)clock()/CLOCKS_PER_SEC;

    u64* input = (u64*) malloc(sizeof(u64)*vector_len);
    for (int idx = 0; idx < vector_len; idx++)
        input[idx] = dense_input[idx + split_index];

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

ClientShares fc_pre(ClientFHE* cfhe, Metadata data, u64* dense_input, int split_index=0) {
  
  printf("\nClient Preprocessing: ");
  float origin = (float)clock()/CLOCKS_PER_SEC;
  int vector_len = data.image_size;
  int matrix_h = data.filter_h;

  u64* input = (u64*) malloc(sizeof(u64)*vector_len);
  for (int idx = 0; idx < vector_len; idx++)
    input[idx] = dense_input[idx + split_index];

  ClientShares client_shares = client_fc_preprocess(cfhe, &data, input);

  float endTime = (float)clock()/CLOCKS_PER_SEC;
  float timeElapsed = endTime - origin;
  printf("[%f seconds]\n", timeElapsed);

  return client_shares;
}

ClientShares fc_server(ServerFHE* sfhe, Metadata data, string weights_filename, ClientShares client_shares, int split_index=0) {

  float endTime;
  float timeElapsed;
  int vector_len = data.image_size;
  int matrix_h = data.filter_h;
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

  // Free matrix
  for (int row = 0; row < matrix_h; row++)
    free(matrix[row]);
  free(matrix);
  free(split_matrix);

  // Free secret shares
  //free(client_shares.linear_ct.inner);
  free(linear_share);

  return client_shares;
}

u64* fc_post(ClientFHE* cfhe, Metadata data, ClientShares client_shares, int split_index=0) {

  float endTime;
  float timeElapsed;
  float startTime;

  printf("Post process: ");
  startTime = (float)clock()/CLOCKS_PER_SEC;

  client_fc_decrypt(cfhe, &data, &client_shares);

  endTime = (float)clock()/CLOCKS_PER_SEC;
  timeElapsed = endTime - startTime;
  printf("[%f seconds]\n", timeElapsed);

  return client_shares.linear[0];
}

void recv_serial(NetIO *io, SerialCT *serial) {
  io->recv_data(&(serial->size), sizeof(uint64_t));
  serial->inner = (char*) malloc( sizeof(char) * serial->size );
  io->recv_data(serial->inner, sizeof(char) * serial->size);
}

void send_serial(NetIO *io, SerialCT *serial) {
  io->send_data(&(serial->size), sizeof(uint64_t));
  io->send_data(serial->inner, sizeof(char) * serial->size);
}

void recv_clientshare(NetIO *io, ClientShares *client_shares) {
  recv_serial(io, &(client_shares->input_ct));
}

void send_clientshare(NetIO *io, ClientShares *client_shares) {
  send_serial(io, &(client_shares->input_ct));
  send_serial(io, &(client_shares->linear_ct));
}

int main(int argc, char* argv[]) {
  // TODO: Rewrite scale_down function to take in 
  // single pointer instead of double pointer
  int SCALE = 256;
  int bitsize = 32;
  int LEN = 32*32*64;
  int port, party;
  parse_party_and_port(argv, &party, &port);
  party = BOB;
  NetIO *io = new NetIO(party == ALICE ? nullptr : "127.0.0.1", port);

  setup_semi_honest(io, party);

  cout << "Calculating ReLU of an array length = " << LEN << ", bitsize =" << bitsize << endl;

  // get key_share
  SerialCT key_share;
  recv_serial(io, &key_share);
  // io->recv_data(&(key_share.size), sizeof(uint64_t));
  // key_share.inner = (char*) malloc( sizeof(char) * key_share.size);
  // io->recv_data(key_share.inner, sizeof(char) * key_share.size);
  ServerFHE sfhe = server_keygen(key_share); 

  /*
  printf("Server Keygen: ");
  startTime = (float)clock()/CLOCKS_PER_SEC;
  
  endTime = (float)clock()/CLOCKS_PER_SEC;
  timeElapsed = endTime - startTime;
  printf("[%f seconds]\n", timeElapsed);
  */
  string model_directory("../models/miniONN_cifar_model/");

  u64* temp;
  u64* dense_input = (u64*) malloc( sizeof(u64) * LEN );
  u64* dense_input_2 = (u64*) malloc( sizeof(u64) * LEN / 4 );
  for (int i = 0; i < LEN; i++) {
      dense_input[i] = 300;
  }

  MeanPoolOutput mean_temp;
  Metadata data;
  ClientShares client_shares;

  io->recv_data(&data, sizeof(Metadata));
  recv_clientshare(io, &client_shares);
  client_shares = conv_server(&sfhe, data, model_directory + "conv2d.kernel.txt", client_shares);
  send_clientshare(io, &client_shares);
  temp = ReLU( bitsize, (int64_t*) dense_input, 32*32*64, party);

  io->recv_data(&data, sizeof(Metadata));
  recv_clientshare(io, &client_shares);
  client_shares = conv_server(&sfhe, data, model_directory + "conv2d_1.kernel.txt", client_shares);
  send_clientshare(io, &client_shares);
  temp = ReLU( bitsize, (int64_t*) dense_input, 32*32*64, party);

  for (int chan = 0; chan < 64; chan++) {
      mean_temp = MeanPooling(bitsize, (int64_t*) dense_input, 32, 32, 2, party);
  }

  io->recv_data(&data, sizeof(Metadata));
  recv_clientshare(io, &client_shares);
  client_shares = conv_server(&sfhe, data, model_directory + "conv2d_2.kernel.txt", client_shares);
  send_clientshare(io, &client_shares);
  temp = ReLU( bitsize, (int64_t*) dense_input, 16*16*64, party);

  io->recv_data(&data, sizeof(Metadata));
  recv_clientshare(io, &client_shares);
  client_shares = conv_server(&sfhe, data, model_directory + "conv2d_3.kernel.txt", client_shares);
  send_clientshare(io, &client_shares);
  temp = ReLU( bitsize, (int64_t*) dense_input, 16*16*64, party);

  for (int chan = 0; chan < 64; chan++) {
      mean_temp = MeanPooling(bitsize, (int64_t*) dense_input_2, 16, 16, 2, party);
  }

  io->recv_data(&data, sizeof(Metadata));
  recv_clientshare(io, &client_shares);
  client_shares = conv_server(&sfhe, data, model_directory + "conv2d_4.kernel.txt", client_shares);
  send_clientshare(io, &client_shares);
  temp = ReLU( bitsize, (int64_t*) dense_input, 8*8*64, party); // ReLU 4

  io->recv_data(&data, sizeof(Metadata));
  recv_clientshare(io, &client_shares);
  client_shares = conv_server(&sfhe, data, model_directory + "conv2d_5.kernel.txt", client_shares);
  send_clientshare(io, &client_shares);
  temp = ReLU( bitsize, (int64_t*) dense_input, 8*8*64, party); // ReLU 5

  io->recv_data(&data, sizeof(Metadata));
  recv_clientshare(io, &client_shares);
  client_shares = conv_server(&sfhe, data, model_directory + "conv2d_6.kernel.txt", client_shares);
  send_clientshare(io, &client_shares);
  temp = ReLU( bitsize, (int64_t*) dense_input, 8*8*16, party); // ReLU 6

  int vec_len;
  int split = 4000;
  io->recv_data(&vec_len, sizeof(int));

  for (int i = 0; i < vec_len; i+=split) {

    io->recv_data(&data, sizeof(Metadata));

    recv_clientshare(io, &client_shares);
    client_shares = fc_server(&sfhe, data, model_directory + "dense.kernel.txt", client_shares, i);
    send_clientshare(io, &client_shares);
  }

  free(temp);
  free(dense_input);
  finalize_semi_honest();

  cout << endl;
  // cout << "ciphertexts: " << ciphertexts << endl;
  // cout << "result_ciphertexts: " << result_ciphertexts << endl;
  cout << "server rot_count: " << rot_count << endl;
  cout << "server multiplications: " << multiplications << endl;
  cout << "server additions: " << additions << endl;

  delete io;
  return 0;

  /*
  // 17) Fully Connected Layer: fully connects the incoming 1024 nodes to the outgoing 10 nodes: R10×1 ← R10×1024· R1024×1.
  int vec_len = width * height * channels;

  int num_vec = 10;
  u64* output = (u64*) malloc(num_vec*sizeof(u64));
  for (int j = 0; j < num_vec; j++) {
    output[j] = 0;
  }

  
  int split = 4000;
  for (int i = 0; i < vec_len; i+=split) {
    printf("min: %d\n", min(vec_len - i, split));

    data = fc_metadata(cfhe.encoder, min(vec_len - i, split), num_vec);

    client_shares = fc_pre(&cfhe, data, relu_output, i);
    client_shares = fc_server(&sfhe, data, "./miniONN_cifar_model/dense.kernel.txt", client_shares, i);
    temp = fc_post(&cfhe, data, client_shares, i);

    // temp = fc(&cfhe, &sfhe, min(vec_len - i, split), num_vec, relu_output, "./miniONN_cifar_model/dense.kernel.txt", i);
    for (int j = 0; j < num_vec; j++) {
        output[j] += temp[j];
        if (output[j] > PLAINTEXT_MODULUS / 2) {
            output[j] -= PLAINTEXT_MODULUS;
        }
    }
  }
  */
}





