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
#include "max_pooling.h"
#include "batchnorm_gc.h"
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

ConvOutput conv(ClientFHE* cfhe, ServerFHE* sfhe, int image_h, int image_w, int filter_h, int filter_w,
    int inp_chans, int out_chans, int stride, bool pad_valid, string weights_filename, u64** input_data=NULL) {
    // both parties need data
    Metadata data = conv_metadata(cfhe->encoder, image_h, image_w, filter_h, filter_w, inp_chans, 
        out_chans, stride, stride, pad_valid);
    string filename;
   
    // client side
    printf("\nClient Preprocessing: ");
    float origin = (float)clock()/CLOCKS_PER_SEC;

    u64** input = (u64**) malloc(sizeof(u64*)*data.inp_chans);
    for (int chan = 0; chan < data.inp_chans; chan++) {
        input[chan] = (u64*) malloc(sizeof(u64)*data.image_size);
    }

    if (input_data == NULL) {
        vector<vector<vector<int> > > image_data;
        filename = "cifar_image.txt";
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

    ClientShares client_shares = client_conv_preprocess(cfhe, &data, input);

    float endTime = (float)clock()/CLOCKS_PER_SEC;
    float timeElapsed = endTime - origin;
    printf("Conv PreProcessing [%f seconds]\n", timeElapsed);

    // outputs -> client_shares

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
    server_conv_free(&data, masks, &server_shares);
    return conv_output;
}

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

void test_batchnorm(int party) {
  u64 width = 16;
  u64 height = 16;
  u64 channels = 64;
  u64 vec_size = width*height*channels;
  int bitsize = 32;

  u64 *vector = (u64*) malloc( sizeof(u64) * vec_size );

  for (int i = 0; i < vec_size; i++) {
    vector[i] = 1;
    if (party == BOB) {
      vector[i] = 0;
    }
  }

  u64 *results; 
  if (party == ALICE) {
    results = batchnorm_gc(bitsize, (int64_t*) vector, height, width, channels, party);
  } else {
    results = batchnorm_gc(bitsize, (int64_t*) vector, height, width, channels, party, "../models/int_resnet18/bn1");
  }

  if (party == ALICE) {
    for (int i = 0; i < vec_size; i+=(width*height)) {
      cout << (int64_t) results[i] << " ";
      cout << (int64_t) results[i+1] << " ";
      cout << (int64_t) results[i+2] << " ";
      cout << endl;
    }
    cout << endl;
  }
  return;
}

// ConvOutput conv(ClientFHE* cfhe, ServerFHE* sfhe, int image_h, int image_w, int filter_h, int filter_w,
//     int inp_chans, int out_chans, int stride, bool pad_valid, string weights_filename, u64** input_data=NULL) {
// struct ConvOutput ResnetBlockLayer(ClientFHE* cfhe, ServerFHE* sfhe, string weights_dir, u64** input_data, bool downsample=false, int first_stride=1) {

// }

int main(int argc, char* argv[]) {
  // TODO: Rewrite scale_down function to take in 
  // single pointer instead of double pointer
  int SCALE = 256*256;
  int bitsize = 32;
  int LEN = 32*32*64;
  int port, party;
  parse_party_and_port(argv, &party, &port);
  NetIO *io = new NetIO(party == ALICE ? nullptr : "127.0.0.1", port);

  setup_semi_honest(io, party);

  cout << "Calculating ReLU of an array length = " << LEN << ", bitsize =" << bitsize << endl;

  if (party != ALICE) {
    delete io;
    return 0;
    u64* temp;
    u64* dense_input = (u64*) malloc( sizeof(u64) * LEN );
    u64* dense_input_2 = (u64*) malloc( sizeof(u64) * LEN / 4 );
    for (int i = 0; i < LEN; i++) {
        dense_input[i] = 300;
    }

    MeanPoolOutput mean_temp;

    temp = ReLU( bitsize, (int64_t*) dense_input, 32*32*64, party);
    temp = ReLU( bitsize, (int64_t*) dense_input, 32*32*64, party);

    for (int chan = 0; chan < 64; chan++) {
        mean_temp = MeanPooling(bitsize, (int64_t*) dense_input, 32, 32, 2, party);
    }

    temp = ReLU( bitsize, (int64_t*) dense_input, 16*16*64, party);
    temp = ReLU( bitsize, (int64_t*) dense_input, 16*16*64, party);

    for (int chan = 0; chan < 64; chan++) {
        mean_temp = MeanPooling(bitsize, (int64_t*) dense_input_2, 16, 16, 2, party);
    }

    temp = ReLU( bitsize, (int64_t*) dense_input, 8*8*64, party); // ReLU 4
    temp = ReLU( bitsize, (int64_t*) dense_input, 8*8*64, party); // ReLU 5
    temp = ReLU( bitsize, (int64_t*) dense_input, 8*8*16, party); // ReLU 6
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
  MaxPoolOutput maxpool_output;
  PaddedOutput padded_output;
  Metadata data;
  meanpool_output.height = 0;
  meanpool_output.width = 0;
  u64* flattened;
  u64** unflattened;
  u64* relu_output;
  u64** meanpool_matrix = (u64**) malloc(sizeof(u64*) * 64);
  u64** maxpool_matrix = (u64**) malloc(sizeof(u64*) * 64);
  u64** saved_input = (u64**) malloc(sizeof(u64*) * 64);
  u64** saved_output = (u64**) malloc(sizeof(u64*) * 64);
  u64** res_output;
  u64* dense_input;
  u64* batch_output;
  u64* temp;
  ClientShares client_shares;

  int height = 64;
  int width = 64;
  int channels = 3;

  string model_directory("../models/int_resnet18/");
  string input_image("../models/resnet_image.txt");

  vector<vector<vector<int> > > image_data;
  image_data = read_image(input_image);
  int image_height = image_data[0].size();
  int image_width = image_data[0][0].size();
  u64 **input = (u64**) malloc( sizeof(u64*) * channels );

  for (int chan = 0; chan < channels; chan++) {
    int idx = 0;
    input[chan] = (u64*) malloc( sizeof(u64) * image_height * image_width );
    for (int i = 0; i < image_height; i++) {
      for (int j = 0; j < image_width; j++) {
        input[chan][idx] = image_data[chan][i][j] * 256;
        idx++;
      }
    }  
  }

  // ResNet18
  clear_files();
  padded_output = pad(input, channels, height, width, 3);
  // 0) Initial Convolution block
  cout << "padded width: " << padded_output.width << endl;
  cout << "padded height: " << padded_output.height << endl;
  cout << "channels: " << channels << endl;
  // for (int i = 0; i < padded_output.width*padded_output.height; i++) {
  //     printf("%ld ", (int64_t)padded_output.matrix[0][i]);
  // } printf("\n");
  // printf("---------------------------------\n");
  data = conv_metadata(cfhe.encoder, padded_output.height, padded_output.width, 7, 7, channels, 64, 2, 2, 1);
  client_shares = conv_pre(&cfhe, data, padded_output.matrix);
  client_shares = conv_server(&sfhe, data, model_directory + "conv1.weight.txt", client_shares);
  conv_output = conv_post(&cfhe, data, client_shares);
  height = conv_output.output_h;
  width = conv_output.output_w;
  channels = conv_output.output_chan;
  cout << "conv2d Done\n";
  cout << "channels: " << channels << endl;
  cout << endl;
  linear_layer_centerlift(conv_output.client_shares.linear, channels, height*width);
  for (int i = 0; i < 100; i++) {
      printf("%ld ", (int64_t)conv_output.client_shares.linear[i]);
  } printf("\n");

  client_free_keys(&cfhe);
  free_ct(&key_share);
  server_free_keys(&sfhe);

  cout << endl;
  // cout << "ciphertexts: " << ciphertexts << endl;
  // cout << "result_ciphertexts: " << result_ciphertexts << endl;
  cout << "rot_count: " << rot_count << endl;
  cout << "multiplications: " << multiplications << endl;
  cout << "additions: " << additions << endl;
  // cout << "subtractions: " << subtractions << endl;

  finalize_semi_honest();
  delete io;
  return 0;
}




