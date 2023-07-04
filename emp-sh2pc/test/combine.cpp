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
#include "conv_layer.h"
#include "dense_layer.h"
#include "extras.h"

typedef uint64_t u64;
std::vector<double> iotime_ms;
std::vector<double> ANDgatetime_ms, XORgatetime_ms;
using namespace emp;
using namespace std;

int main(int argc, char* argv[]) {
  int SCALE = 256;
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

    for (int chan = 0; chan < 64; chan++) {
        mean_temp = MeanPooling(bitsize, (int64_t*) dense_input_2, 16, 16, 2, party);
    }

    temp = ReLU( bitsize, (int64_t*) dense_input, LEN, party); // ReLU 4
    temp = ReLU( bitsize, (int64_t*) dense_input, 8*8*64, party); // ReLU 5
    temp = ReLU( bitsize, (int64_t*) dense_input, LEN, party); // ReLU 6
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
  meanpool_output.height = 0;
  meanpool_output.width = 0;
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
  conv_output = conv(&cfhe, &sfhe, height, width, 3, 3, channels, 64, 1, 0, "./miniONN_cifar_model/conv2d.kernel.txt");
  height = conv_output.output_h;
  width = conv_output.output_w;
  channels = conv_output.output_chan;
  cout << "conv2d Done\n";
  cout << "channels: " << channels << endl;

  // 2) ReLU Activation: calculates ReLU for each input
  flattened = flatten(conv_output.client_shares.linear, channels, height, width);
  relu_output = ReLU(bitsize, (int64_t*) flattened, LEN, party);
  cout << "ReLU Done\n";
  for (int i = 0; i < 50; i++) {
    printf("%d ", (int)relu_output[i]);
  } printf("\n");

  // 3) Convolution: window size 3 × 3, stride (1, 1), pad (1, 1), number of output channels 64: R64×1024 ← R64×576· R576×1024.
  unflattened = unflatten(relu_output, channels, height, width);
  cout << "After unflatten\n";
  for (int i = 0; i < 50; i++) {
    printf("%d ", (int)unflattened[0][i]);
  } printf("\n");
  // TODO: EXAMINE INPUT CHANNELS 
  // channels = 32;
  conv_output = conv(&cfhe, &sfhe, height, width, 3, 3, channels, 64, 1, 0, "./miniONN_cifar_model/conv2d_1.kernel.txt", unflattened);
  // height = conv_output.output_h;
  // width = conv_output.output_w;
  // channels = conv_output.output_chan;
  cout << "conv2d_1 Done\n";
  for (int i = 0; i < 50; i++) {
    printf("%d ", (int)conv_output.client_shares.linear[0][i]);
  } printf("\n");

  // 4) ReLU Activation: calculates ReLU for each input.
  flattened = flatten(conv_output.client_shares.linear, channels, height, width);
  relu_output = ReLU(bitsize, (int64_t*) flattened, LEN, party);
  cout << "ReLU_1 Done\n";
  for (int i = 0; i < 50; i++) {
    printf("%d ", (int)relu_output[i]);
  } printf("\n");

  // 5) Mean Pooling: window size 1 × 2 × 2, outputs R64×16×16.
  unflattened = unflatten(relu_output, channels, height, width);
  for (int chan = 0; chan < channels; chan++) {
    meanpool_output = MeanPooling(bitsize, (int64_t*) unflattened[chan], height, width, 2, party);
    meanpool_matrix[chan] = meanpool_output.matrix;
  }
  width = meanpool_output.width;
  height = meanpool_output.height;
  cout << "meanpooling Done\n";
  for (int i = 0; i < 50; i++) {
    printf("%d ", (int)meanpool_matrix[0][i]);
  } printf("\n");

  // 6) Convolution: window size 3 × 3, stride (1, 1), pad (1, 1), number of output channels 64: R64×256 ← R64×576· R576×256.
  conv_output = conv(&cfhe, &sfhe, height, width, 3, 3, channels, 64, 1, 0, "./miniONN_cifar_model/conv2d_2.kernel.txt", meanpool_matrix);
  cout << "conv2d_2 Done\n";

  // 7) ReLU Activation: calculates ReLU for each input.
  flattened = flatten(conv_output.client_shares.linear, channels, height, width);
  relu_output = ReLU(bitsize, (int64_t*) flattened, LEN, party);
  cout << "ReLU_2 Done\n";

  // 8) Convolution: window size 3 × 3, stride (1, 1), pad (1, 1), number of output channels 64: R64×256 ← R64×576· R576×256.
  unflattened = unflatten(relu_output, channels, height, width);
  conv_output = conv(&cfhe, &sfhe, height, width, 3, 3, channels, 64, 1, 0, "./miniONN_cifar_model/conv2d_3.kernel.txt", unflattened);
  height = conv_output.output_h;
  width = conv_output.output_w;
  channels = conv_output.output_chan;
  cout << "conv2d_3 Done\n";

  // 9) ReLU Activation: calculates ReLU for each input
  flattened = flatten(conv_output.client_shares.linear, channels, height, width);
  relu_output = ReLU(bitsize, (int64_t*) flattened, LEN, party);
  cout << "ReLU_3 Done\n";

  meanpool_matrix = (u64**) malloc(sizeof(u64*) * 64) ;
  // 10) Mean Pooling: window size 1 × 2 × 2, outputs R64×16×16.
  unflattened = unflatten(relu_output, channels, height, width);
  for (int chan = 0; chan < channels; chan++) {
    meanpool_output = MeanPooling(bitsize, (int64_t*) unflattened[chan], height, width, 2, party);
    meanpool_matrix[chan] = meanpool_output.matrix;
  }
  width = meanpool_output.width;
  height = meanpool_output.height;
  cout << "meanpooling_1 Done\n";

  // 11) Convolution: window size 3 × 3, stride (1, 1), pad (1, 1), number of output channels 64: R64×64 ← R64×576· R576×64.
  conv_output = conv(&cfhe, &sfhe, height, width, 3, 3, channels, 64, 1, 0, "./miniONN_cifar_model/conv2d_4.kernel.txt", meanpool_matrix);
  height = conv_output.output_h;
  width = conv_output.output_w;
  channels = conv_output.output_chan;
  cout << "conv2d_4 Done\n";

  // 12) ReLU Activation: calculates ReLU for each input.
  flattened = flatten(conv_output.client_shares.linear, channels, height, width);
  relu_output = ReLU(bitsize, (int64_t*) flattened, LEN, party);
  cout << "ReLU_4 Done\n";

  // 13) Convolution: window size 1 × 1, stride (1, 1), number of output channels of 64: R64×64 ← R64×64· R64×64.
  unflattened = unflatten(relu_output, channels, height, width);
  conv_output = conv(&cfhe, &sfhe, height, width, 1, 1, channels, 64, 1, 1, "./miniONN_cifar_model/conv2d_5.kernel.txt", unflattened);
  height = conv_output.output_h;
  width = conv_output.output_w;
  channels = conv_output.output_chan;
  cout << "conv2d_5 Done\n";
  cout << "(height, width): (" << height << ", " << width << ")\n"; 
  cout << "channels: " << channels << endl;

  // 14) ReLU Activation: calculates ReLU for each input.
  flattened = flatten(conv_output.client_shares.linear, channels, height, width);
  relu_output = ReLU(bitsize, (int64_t*) flattened, height*width*channels, party);
  cout << "ReLU_5 Done\n";

  // 15) Convolution: window size 1 × 1, stride (1, 1), number of output channels of 16: R16×64 ← R16×64·R64×64.
  unflattened = unflatten(relu_output, channels, height, width);
  conv_output = conv(&cfhe, &sfhe, height, width, 1, 1, channels, 16, 1, 1, "./miniONN_cifar_model/conv2d_6.kernel.txt", unflattened);
  height = conv_output.output_h;
  width = conv_output.output_w;
  channels = conv_output.output_chan;
  cout << "conv2d_6 Done\n";

  // 16) ReLU Activation: calculates ReLU for each input.
  flattened = flatten(conv_output.client_shares.linear, channels, height, width);
  relu_output = ReLU(bitsize, (int64_t*) flattened, LEN, party);
  cout << "ReLU_6 Done\n";

  // 17) Fully Connected Layer: fully connects the incoming 1024 nodes to the outgoing 10 nodes: R10×1 ← R10×1024· R1024×1.
  int vec_len = width * height * channels;
  //fc(&cfhe, &sfhe, vec_len, num_vec, relu_output, "miniONN_cifar_model/dense.kernel.txt");

  cout << " RELU OUTPUT \n";
  for (int i = 0; i < 50; i++) {
    printf("%d ", (int)relu_output[i]);
  }

  //free(dense_input);
  // dense_input = temp;

  int num_vec = 10;
  // int64_t* output = (int64_t*) malloc(num_vec*sizeof(int64_t));
  u64* output = (u64*) malloc(num_vec*sizeof(u64));
  for (int j = 0; j < num_vec; j++) {
    output[j] = 0;
  }

  
  int split = 4000;
  for (int i = 0; i < vec_len; i+=split) {
    temp = fc(&cfhe, &sfhe, min(vec_len - i, split), num_vec, relu_output, "miniONN_cifar_model/dense.kernel.txt", i);
    for (int j = 0; j < num_vec; j++) {
        output[j] += temp[j];
        if (output[j] > PLAINTEXT_MODULUS / 2) {
            output[j] -= PLAINTEXT_MODULUS;
        }
    }
  }
  

  printf("Linear: [");
    for (int idx = 0; idx < num_vec; idx++) {
        printf("%d, " , (int)output[idx]);
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





