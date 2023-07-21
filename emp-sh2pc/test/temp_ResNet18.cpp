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


int main(int argc, char* argv[]) {
  // TODO: Rewrite scale_down function to take in 
  // single pointer instead of double pointer
  int SCALE = 256;
  int bitsize = 32;
  int LEN = 32*32*64;
  int port, party;
  parse_party_and_port(argv, &party, &port);
  party = ALICE;
  NetIO *io = new NetIO(party == ALICE ? nullptr : "127.0.0.1", port);

  setup_semi_honest(io, party);

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

  // io->send_data(&(key_share.size), sizeof(uint64_t));
  // io->send_data(key_share.inner, sizeof(char) * key_share.size);
  send_serial(io, &key_share); 

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
  u64** meanpool_matrix = (u64**) malloc(sizeof(u64*) * 64) ;
  u64** maxpool_matrix = (u64**) malloc(sizeof(u64*) * 64) ;
  u64** saved_input = (u64**) malloc(sizeof(u64*) * 64) ;
  u64** saved_output = (u64**) malloc(sizeof(u64*) * 64) ;
  u64** res_output;
  u64* dense_input;
  u64* batch_output;
  u64* temp;
  ClientShares client_shares;

  int height = 32;
  int width = 32;
  int channels = 3;


  string model_directory("../models/resnet18/");
  string input_image("../models/cifar_image.txt");

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
        input[chan][idx] = image_data[chan][i][j];
        idx++;
      }
    }  
  }

 // ResNet18
   clear_files();
   padded_output = pad(input, channels, height, width, 3);
  // 0) Initial Convolution block
  data = conv_metadata(cfhe.encoder, height, width, 7, 7, channels, 64, 2, 2, 1);
  client_shares = conv_pre(&cfhe, data, padded_output.matrix);

  io->send_data(&data, sizeof(Metadata));
  send_clientshare(io, &client_shares);
  recv_clientshare(io, &client_shares);
  // client_shares = conv_server(&sfhe, data, model_directory + "conv2d.kernel.txt", client_shares);

  // 0.1) Convolution
  conv_output = conv_post(&cfhe, data, client_shares);
  height = conv_output.output_h;
  width = conv_output.output_w;
  channels = conv_output.output_chan;
  cout << "conv2d Done\n";
  cout << "channels: " << channels << endl;
  cout << endl;
  linear_layer_centerlift(conv_output.client_shares.linear, channels, height*width);
  //print_3D_output(conv_output.client_shares.linear, height, width, channels, "./output_files/1_conv_output.txt");
  /*
  for (int i = 0; i < 100; i++) {
      printf("%ld ", (int64_t)conv_output.client_shares.linear[i]);
  } printf("\n");
  */

  // 0.2) Batch Norm
  flattened = flatten(conv_output.client_shares.linear, channels, height, width);
  batch_output = batchnorm_gc(bitsize, (int64_t*) flattened, height, width, channels, party)
  cout << "Batch Norm Done\n";
  cout << endl;

  // 0.3) ReLU
  relu_output = ReLU(bitsize, (int64_t*) batch_output, height*width*channels, party);
  scale_down(relu_output, channels*height*width, SCALE);
  cout << "ReLU Done\n";
  cout << endl;
  //print_1D_output(relu_output, height*width*channels, "./output_files/2_relu_output.txt");

  // 0.4) Max Pool with padding
  unflattened = unflatten(relu_output, channels, height, width);
  padded_output = pad(unflatten, channels, height, width, 1);
  height = padded_output.height;
  width = padded_output.width;
  for (int chan = 0; chan < channels; chan++) {
    maxpool_output = MaxPooling(bitsize, (int64_t*) padded_output.matrix[chan], height, width, 3, 2, party);
    maxpool_matrix[chan] = maxpool_output.matrix;
  }
  input = maxpool_matrix;
  width = maxpool_output.width;
  height = maxpool_output.height;
  cout << "maxpooling_1 Done\n";
  cout << endl;

  // 1) Layer 1: 2 Basic Blocks with residual
  // 1.0) 1st basic block
  // 1.0.1) Convolution
  saved_input = input;
  data = conv_metadata(cfhe.encoder, height, width, 3, 3, channels, 64, 1, 1, 0);
  client_shares = conv_pre(&cfhe, data, input);

  io->send_data(&data, sizeof(Metadata));
  send_clientshare(io, &client_shares);
  recv_clientshare(io, &client_shares);
  // client_shares = conv_server(&sfhe, data, model_directory + "conv2d_1.kernel.txt", client_shares);

  conv_output = conv_post(&cfhe, data, client_shares);
  height = conv_output.output_h;
  width = conv_output.output_w;
  channels = conv_output.output_chan;
  cout << "conv2d_1 Done\n";
  cout << endl;
  linear_layer_centerlift(conv_output.client_shares.linear, channels, height*width);
  //print_3D_output(conv_output.client_shares.linear, height, width, channels, "./output_files/3_conv_output.txt");

  // 1.0.2) Batch Norm
  flattened = flatten(conv_output.client_shares.linear, channels, height, width);
  batch_output = batchnorm_gc(bitsize, flattened, height, width, channels, party);

  // 1.0.3) ReLU
  relu_output = ReLU(bitsize, batch_output, height*width*channels, party);

  // 1.0.4) Convolution
  unflattened = unflatten(relu_output, channels, width, height);
  data = conv_metadata(cfhe.encoder, height, width, 3, 3, channels, 64, 1, 1, 0);
  client_shares = conv_pre(&cfhe, data, unflattened);

  io->send_data(&data, sizeof(Metadata));
  send_clientshare(io, &client_shares);
  recv_clientshare(io, &client_shares);
  // client_shares = conv_server(&sfhe, data, model_directory + "conv2d_1.kernel.txt", client_shares);

  conv_output = conv_post(&cfhe, data, client_shares);
  height = conv_output.output_h;
  width = conv_output.output_w;
  channels = conv_output.output_chan;
  cout << "conv2d_1 Done\n";
  cout << endl;
  linear_layer_centerlift(conv_output.client_shares.linear, channels, height*width);

  // 1.0.5) Batch Norm
  flattened = flatten(conv_output.client_shares.linear, channels, height, width);
  batch_output = batchnorm_gc(bitsize, flattened, height, width, channels, party);

  // 1.0.6) Residual
  res_output = residual(saved_input, batch_output, channels, height, width);

  // 1.0.7) ReLU
  flattened = flatten(res_output, channels, height, width);
  relu_output = ReLU(bitsize, flattened, height*width*channels, party)

  // 1.1) 2nd basic block
  // 1.1.1) Convolution
  input = relu_output;
  saved_input = input;
  data = conv_metadata(cfhe.encoder, height, width, 3, 3, channels, 64, 1, 1, 0);
  client_shares = conv_pre(&cfhe, data, input);

  io->send_data(&data, sizeof(Metadata));
  send_clientshare(io, &client_shares);
  recv_clientshare(io, &client_shares);
  // client_shares = conv_server(&sfhe, data, model_directory + "conv2d_1.kernel.txt", client_shares);

  conv_output = conv_post(&cfhe, data, client_shares);
  height = conv_output.output_h;
  width = conv_output.output_w;
  channels = conv_output.output_chan;
  cout << "conv2d_1 Done\n";
  cout << endl;
  linear_layer_centerlift(conv_output.client_shares.linear, channels, height*width);
  //print_3D_output(conv_output.client_shares.linear, height, width, channels, "./output_files/3_conv_output.txt");

  // 1.1.2) Batch Norm
  flattened = flatten(conv_output.client_shares.linear, channels, height, width);
  batch_output = batchnorm_gc(bitsize, flattened, height, width, channels, party);

  // 1.1.3) ReLU
  relu_output = ReLU(bitsize, batch_output, height*width*channels, party);

  // 1.1.4) Convolution
  unflattened = unflatten(relu_output, channels, width, height);
  data = conv_metadata(cfhe.encoder, height, width, 3, 3, channels, 64, 1, 1, 0);
  client_shares = conv_pre(&cfhe, data, unflattened);

  io->send_data(&data, sizeof(Metadata));
  send_clientshare(io, &client_shares);
  recv_clientshare(io, &client_shares);
  // client_shares = conv_server(&sfhe, data, model_directory + "conv2d_1.kernel.txt", client_shares);

  conv_output = conv_post(&cfhe, data, client_shares);
  height = conv_output.output_h;
  width = conv_output.output_w;
  channels = conv_output.output_chan;
  cout << "conv2d_1 Done\n";
  cout << endl;
  linear_layer_centerlift(conv_output.client_shares.linear, channels, height*width);

  // 1.1.5) Batch Norm
  flattened = flatten(conv_output.client_shares.linear, channels, height, width);
  batch_output = batchnorm_gc(bitsize, flattened, height, width, channels, party);

  // 1.1.6) Residual
  res_output = residual(saved_input, batch_output, channels, height, width);

  // 1.1.7) ReLU
  flattened = flatten(res_output, channels, height, width);
  relu_output = ReLU(bitsize, flattened, height*width*channels, party)

  // 2) Layer 2: 2 Basic Blocks with downsampling
  // 2.0) 1st basic block
  // 2.0.1) Convolution
  input = relu_output;
  saved_input = input;
  data = conv_metadata(cfhe.encoder, height, width, 3, 3, channels, 64, 2, 2, 0);
  client_shares = conv_pre(&cfhe, data, input);

  io->send_data(&data, sizeof(Metadata));
  send_clientshare(io, &client_shares);
  recv_clientshare(io, &client_shares);
  // client_shares = conv_server(&sfhe, data, model_directory + "conv2d_1.kernel.txt", client_shares);

  conv_output = conv_post(&cfhe, data, client_shares);
  height = conv_output.output_h;
  width = conv_output.output_w;
  channels = conv_output.output_chan;
  cout << "conv2d_1 Done\n";
  cout << endl;
  linear_layer_centerlift(conv_output.client_shares.linear, channels, height*width);
  //print_3D_output(conv_output.client_shares.linear, height, width, channels, "./output_files/3_conv_output.txt");

  // 2.0.2) Batch Norm
  flattened = flatten(conv_output.client_shares.linear, channels, height, width);
  batch_output = batchnorm_gc(bitsize, flattened, height, width, channels, party);

  // 2.0.3) ReLU
  relu_output = ReLU(bitsize, batch_output, height*width*channels, party);

  // 2.0.4) Convolution
  unflattened = unflatten(relu_output, channels, width, height);
  data = conv_metadata(cfhe.encoder, height, width, 3, 3, channels, 64, 1, 1, 0);
  client_shares = conv_pre(&cfhe, data, unflattened);

  io->send_data(&data, sizeof(Metadata));
  send_clientshare(io, &client_shares);
  recv_clientshare(io, &client_shares);
  // client_shares = conv_server(&sfhe, data, model_directory + "conv2d_1.kernel.txt", client_shares);

  conv_output = conv_post(&cfhe, data, client_shares);
  height = conv_output.output_h;
  width = conv_output.output_w;
  channels = conv_output.output_chan;
  cout << "conv2d_1 Done\n";
  cout << endl;
  linear_layer_centerlift(conv_output.client_shares.linear, channels, height*width);

  // 2.0.5) Batch Norm
  flattened = flatten(conv_output.client_shares.linear, channels, height, width);
  batch_output = batchnorm_gc(bitsize, flattened, height, width, channels, party);
  unflattened = unflatten(batch_output, channels, height, width);
  saved_output = unflattened;

  // 2.0.6) Downsampling
  // 2.0.6.1) Convolution
  data = conv_metadata(cfhe.encoder, height, width, 3, 3, channels, 64, 2, 2, 0);
  client_shares = conv_pre(&cfhe, data, saved_input);

  io->send_data(&data, sizeof(Metadata));
  send_clientshare(io, &client_shares);
  recv_clientshare(io, &client_shares);
  // client_shares = conv_server(&sfhe, data, model_directory + "conv2d_1.kernel.txt", client_shares);

  conv_output = conv_post(&cfhe, data, client_shares);
  height = conv_output.output_h;
  width = conv_output.output_w;
  channels = conv_output.output_chan;
  cout << "conv2d_1 Done\n";
  cout << endl;
  linear_layer_centerlift(conv_output.client_shares.linear, channels, height*width);

  // 2.0.6.2) Batch Norm
  flattened = flatten(conv_output.client_shares.linear, channels, height, width);
  batch_output = batchnorm_gc(bitsize, flattened, height, width, channels, party);
  unflattened = unflatten(batch_output, channels, height, width);
  

  // 2.0.7) Residual
  res_output = residual(batch_output, saved_output, channels, height, width);

  // 2.0.8) ReLU
  flattened = flatten(res_output, channels, height, width);
  relu_output = ReLU(bitsize, flattened, height*width*channels, party);

  // 2.1) 2nd basic block
  // 2.1.1) Convolution
  input = relu_output;
  saved_input = input;
  data = conv_metadata(cfhe.encoder, height, width, 3, 3, channels, 64, 1, 1, 0);
  client_shares = conv_pre(&cfhe, data, input);

  io->send_data(&data, sizeof(Metadata));
  send_clientshare(io, &client_shares);
  recv_clientshare(io, &client_shares);
  // client_shares = conv_server(&sfhe, data, model_directory + "conv2d_1.kernel.txt", client_shares);

  conv_output = conv_post(&cfhe, data, client_shares);
  height = conv_output.output_h;
  width = conv_output.output_w;
  channels = conv_output.output_chan;
  cout << "conv2d_1 Done\n";
  cout << endl;
  linear_layer_centerlift(conv_output.client_shares.linear, channels, height*width);
  //print_3D_output(conv_output.client_shares.linear, height, width, channels, "./output_files/3_conv_output.txt");

  // 2.1.2) Batch Norm
  flattened = flatten(conv_output.client_shares.linear, channels, height, width);
  batch_output = batchnorm_gc(bitsize, flattened, height, width, channels, party);

  // 2.1.3) ReLU
  relu_output = ReLU(bitsize, batch_output, height*width*channels, party);

  // 2.1.4) Convolution
  unflattened = unflatten(relu_output, channels, width, height);
  data = conv_metadata(cfhe.encoder, height, width, 3, 3, channels, 64, 1, 1, 0);
  client_shares = conv_pre(&cfhe, data, unflattened);

  io->send_data(&data, sizeof(Metadata));
  send_clientshare(io, &client_shares);
  recv_clientshare(io, &client_shares);
  // client_shares = conv_server(&sfhe, data, model_directory + "conv2d_1.kernel.txt", client_shares);

  conv_output = conv_post(&cfhe, data, client_shares);
  height = conv_output.output_h;
  width = conv_output.output_w;
  channels = conv_output.output_chan;
  cout << "conv2d_1 Done\n";
  cout << endl;
  linear_layer_centerlift(conv_output.client_shares.linear, channels, height*width);

  // 2.1.5) Batch Norm
  flattened = flatten(conv_output.client_shares.linear, channels, height, width);
  batch_output = batchnorm_gc(bitsize, flattened, height, width, channels, party);
  unflattened = unflatten(batch_output, channels, height, width);
  saved_output = unflattened;
  
  // 2.1.6) Residual
  res_output = residual(saved_input, saved_output, channels, height, width);

  // 2.1.7) ReLU
  flattened = flatten(res_output, channels, height, width);
  relu_output = ReLU(bitsize, flattened, height*width*channels, party)

  // 3) Layer 3: 2 Basic Blocks with downsampling
  // 3.0) 1st basic block
  // 3.0.1) Convolution
  input = relu_output;
  saved_input = input;
  data = conv_metadata(cfhe.encoder, height, width, 3, 3, channels, 64, 2, 2, 0);
  client_shares = conv_pre(&cfhe, data, input);

  io->send_data(&data, sizeof(Metadata));
  send_clientshare(io, &client_shares);
  recv_clientshare(io, &client_shares);
  // client_shares = conv_server(&sfhe, data, model_directory + "conv2d_1.kernel.txt", client_shares);

  conv_output = conv_post(&cfhe, data, client_shares);
  height = conv_output.output_h;
  width = conv_output.output_w;
  channels = conv_output.output_chan;
  cout << "conv2d_1 Done\n";
  cout << endl;
  linear_layer_centerlift(conv_output.client_shares.linear, channels, height*width);
  //print_3D_output(conv_output.client_shares.linear, height, width, channels, "./output_files/3_conv_output.txt");

  // 3.0.2) Batch Norm
  flattened = flatten(conv_output.client_shares.linear, channels, height, width);
  batch_output = batchnorm_gc(bitsize, flattened, height, width, channels, party);

  // 3.0.3) ReLU
  relu_output = ReLU(bitsize, batch_output, height*width*channels, party);

  // 3.0.4) Convolution
  unflattened = unflatten(relu_output, channels, width, height);
  data = conv_metadata(cfhe.encoder, height, width, 3, 3, channels, 64, 1, 1, 0);
  client_shares = conv_pre(&cfhe, data, unflattened);

  io->send_data(&data, sizeof(Metadata));
  send_clientshare(io, &client_shares);
  recv_clientshare(io, &client_shares);
  // client_shares = conv_server(&sfhe, data, model_directory + "conv2d_1.kernel.txt", client_shares);

  conv_output = conv_post(&cfhe, data, client_shares);
  height = conv_output.output_h;
  width = conv_output.output_w;
  channels = conv_output.output_chan;
  cout << "conv2d_1 Done\n";
  cout << endl;
  linear_layer_centerlift(conv_output.client_shares.linear, channels, height*width);

  // 3.0.5) Batch Norm
  flattened = flatten(conv_output.client_shares.linear, channels, height, width);
  batch_output = batchnorm_gc(bitsize, flattened, height, width, channels, party);
  unflattened = unflatten(batch_output, channels, height, width);
  saved_output = unflattened;

  // 3.0.6) Downsampling
  // 3.0.6.1) Convolution
  data = conv_metadata(cfhe.encoder, height, width, 3, 3, channels, 64, 2, 2, 0);
  client_shares = conv_pre(&cfhe, data, saved_input);

  io->send_data(&data, sizeof(Metadata));
  send_clientshare(io, &client_shares);
  recv_clientshare(io, &client_shares);
  // client_shares = conv_server(&sfhe, data, model_directory + "conv2d_1.kernel.txt", client_shares);

  conv_output = conv_post(&cfhe, data, client_shares);
  height = conv_output.output_h;
  width = conv_output.output_w;
  channels = conv_output.output_chan;
  cout << "conv2d_1 Done\n";
  cout << endl;
  linear_layer_centerlift(conv_output.client_shares.linear, channels, height*width);

  // 3.0.6.2) Batch Norm
  flattened = flatten(conv_output.client_shares.linear, channels, height, width);
  batch_output = batchnorm_gc(bitsize, flattened, height, width, channels, party);
  unflattened = unflatten(batch_output, channels, height, width);
  

  // 3.0.7) Residual
  res_output = residual(batch_output, saved_output, channels, height, width);

  // 3.0.8) ReLU
  flattened = flatten(res_output, channels, height, width);
  relu_output = ReLU(bitsize, flattened, height*width*channels, party);

  // 3.1) 2nd basic block
  // 3.1.1) Convolution
  input = relu_output;
  saved_input = input;
  data = conv_metadata(cfhe.encoder, height, width, 3, 3, channels, 64, 1, 1, 0);
  client_shares = conv_pre(&cfhe, data, input);

  io->send_data(&data, sizeof(Metadata));
  send_clientshare(io, &client_shares);
  recv_clientshare(io, &client_shares);
  // client_shares = conv_server(&sfhe, data, model_directory + "conv2d_1.kernel.txt", client_shares);

  conv_output = conv_post(&cfhe, data, client_shares);
  height = conv_output.output_h;
  width = conv_output.output_w;
  channels = conv_output.output_chan;
  cout << "conv2d_1 Done\n";
  cout << endl;
  linear_layer_centerlift(conv_output.client_shares.linear, channels, height*width);
  //print_3D_output(conv_output.client_shares.linear, height, width, channels, "./output_files/3_conv_output.txt");

  // 3.1.2) Batch Norm
  flattened = flatten(conv_output.client_shares.linear, channels, height, width);
  batch_output = batchnorm_gc(bitsize, flattened, height, width, channels, party);

  // 3.1.3) ReLU
  relu_output = ReLU(bitsize, batch_output, height*width*channels, party);

  // 3.1.4) Convolution
  unflattened = unflatten(relu_output, channels, width, height);
  data = conv_metadata(cfhe.encoder, height, width, 3, 3, channels, 64, 1, 1, 0);
  client_shares = conv_pre(&cfhe, data, unflattened);

  io->send_data(&data, sizeof(Metadata));
  send_clientshare(io, &client_shares);
  recv_clientshare(io, &client_shares);
  // client_shares = conv_server(&sfhe, data, model_directory + "conv2d_1.kernel.txt", client_shares);

  conv_output = conv_post(&cfhe, data, client_shares);
  height = conv_output.output_h;
  width = conv_output.output_w;
  channels = conv_output.output_chan;
  cout << "conv2d_1 Done\n";
  cout << endl;
  linear_layer_centerlift(conv_output.client_shares.linear, channels, height*width);

  // 3.1.5) Batch Norm
  flattened = flatten(conv_output.client_shares.linear, channels, height, width);
  batch_output = batchnorm_gc(bitsize, flattened, height, width, channels, party);
  unflattened = unflatten(batch_output, channels, height, width);
  saved_output = unflattened;
  
  // 3.1.6) Residual
  res_output = residual(saved_input, saved_output, channels, height, width);

  // 3.1.7) ReLU
  flattened = flatten(res_output, channels, height, width);
  relu_output = ReLU(bitsize, flattened, height*width*channels, party)

  // 4) Layer 4: 2 Basic Blocks with downsampling
  // 4.0) 1st basic block
  // 4.0.1) Convolution
  input = relu_output;
  saved_input = input;
  data = conv_metadata(cfhe.encoder, height, width, 3, 3, channels, 64, 2, 2, 0);
  client_shares = conv_pre(&cfhe, data, input);

  io->send_data(&data, sizeof(Metadata));
  send_clientshare(io, &client_shares);
  recv_clientshare(io, &client_shares);
  // client_shares = conv_server(&sfhe, data, model_directory + "conv2d_1.kernel.txt", client_shares);

  conv_output = conv_post(&cfhe, data, client_shares);
  height = conv_output.output_h;
  width = conv_output.output_w;
  channels = conv_output.output_chan;
  cout << "conv2d_1 Done\n";
  cout << endl;
  linear_layer_centerlift(conv_output.client_shares.linear, channels, height*width);
  //print_3D_output(conv_output.client_shares.linear, height, width, channels, "./output_files/3_conv_output.txt");

  // 4.0.2) Batch Norm
  flattened = flatten(conv_output.client_shares.linear, channels, height, width);
  batch_output = batchnorm_gc(bitsize, flattened, height, width, channels, party);

  // 4.0.3) ReLU
  relu_output = ReLU(bitsize, batch_output, height*width*channels, party);

  // 4.0.4) Convolution
  unflattened = unflatten(relu_output, channels, width, height);
  data = conv_metadata(cfhe.encoder, height, width, 3, 3, channels, 64, 1, 1, 0);
  client_shares = conv_pre(&cfhe, data, unflattened);

  io->send_data(&data, sizeof(Metadata));
  send_clientshare(io, &client_shares);
  recv_clientshare(io, &client_shares);
  // client_shares = conv_server(&sfhe, data, model_directory + "conv2d_1.kernel.txt", client_shares);

  conv_output = conv_post(&cfhe, data, client_shares);
  height = conv_output.output_h;
  width = conv_output.output_w;
  channels = conv_output.output_chan;
  cout << "conv2d_1 Done\n";
  cout << endl;
  linear_layer_centerlift(conv_output.client_shares.linear, channels, height*width);

  // 4.0.5) Batch Norm
  flattened = flatten(conv_output.client_shares.linear, channels, height, width);
  batch_output = batchnorm_gc(bitsize, flattened, height, width, channels, party);
  unflattened = unflatten(batch_output, channels, height, width);
  saved_output = unflattened;

  // 4.0.6) Downsampling
  // 4.0.6.1) Convolution
  data = conv_metadata(cfhe.encoder, height, width, 3, 3, channels, 64, 2, 2, 0);
  client_shares = conv_pre(&cfhe, data, saved_input);

  io->send_data(&data, sizeof(Metadata));
  send_clientshare(io, &client_shares);
  recv_clientshare(io, &client_shares);
  // client_shares = conv_server(&sfhe, data, model_directory + "conv2d_1.kernel.txt", client_shares);

  conv_output = conv_post(&cfhe, data, client_shares);
  height = conv_output.output_h;
  width = conv_output.output_w;
  channels = conv_output.output_chan;
  cout << "conv2d_1 Done\n";
  cout << endl;
  linear_layer_centerlift(conv_output.client_shares.linear, channels, height*width);

  // 4.0.6.2) Batch Norm
  flattened = flatten(conv_output.client_shares.linear, channels, height, width);
  batch_output = batchnorm_gc(bitsize, flattened, height, width, channels, party);
  unflattened = unflatten(batch_output, channels, height, width);
  

  // 4.0.7) Residual
  res_output = residual(batch_output, saved_output, channels, height, width);

  // 4.0.8) ReLU
  flattened = flatten(res_output, channels, height, width);
  relu_output = ReLU(bitsize, flattened, height*width*channels, party);

  // 4.1) 2nd basic block
  // 4.1.1) Convolution
  input = relu_output;
  saved_input = input;
  data = conv_metadata(cfhe.encoder, height, width, 3, 3, channels, 64, 1, 1, 0);
  client_shares = conv_pre(&cfhe, data, input);

  io->send_data(&data, sizeof(Metadata));
  send_clientshare(io, &client_shares);
  recv_clientshare(io, &client_shares);
  // client_shares = conv_server(&sfhe, data, model_directory + "conv2d_1.kernel.txt", client_shares);

  conv_output = conv_post(&cfhe, data, client_shares);
  height = conv_output.output_h;
  width = conv_output.output_w;
  channels = conv_output.output_chan;
  cout << "conv2d_1 Done\n";
  cout << endl;
  linear_layer_centerlift(conv_output.client_shares.linear, channels, height*width);
  //print_3D_output(conv_output.client_shares.linear, height, width, channels, "./output_files/3_conv_output.txt");

  // 4.1.2) Batch Norm
  flattened = flatten(conv_output.client_shares.linear, channels, height, width);
  batch_output = batchnorm_gc(bitsize, flattened, height, width, channels, party);

  // 4.1.3) ReLU
  relu_output = ReLU(bitsize, batch_output, height*width*channels, party);

  // 4.1.4) Convolution
  unflattened = unflatten(relu_output, channels, width, height);
  data = conv_metadata(cfhe.encoder, height, width, 3, 3, channels, 64, 1, 1, 0);
  client_shares = conv_pre(&cfhe, data, unflattened);

  io->send_data(&data, sizeof(Metadata));
  send_clientshare(io, &client_shares);
  recv_clientshare(io, &client_shares);
  // client_shares = conv_server(&sfhe, data, model_directory + "conv2d_1.kernel.txt", client_shares);

  conv_output = conv_post(&cfhe, data, client_shares);
  height = conv_output.output_h;
  width = conv_output.output_w;
  channels = conv_output.output_chan;
  cout << "conv2d_1 Done\n";
  cout << endl;
  linear_layer_centerlift(conv_output.client_shares.linear, channels, height*width);

  // 4.1.5) Batch Norm
  flattened = flatten(conv_output.client_shares.linear, channels, height, width);
  batch_output = batchnorm_gc(bitsize, flattened, height, width, channels, party);
  unflattened = unflatten(batch_output, channels, height, width);
  saved_output = unflattened;
  
  // 4.1.6) Residual
  res_output = residual(saved_input, saved_output, channels, height, width);

  // 4.1.7) ReLU
  flattened = flatten(res_output, channels, height, width);
  relu_output = ReLU(bitsize, flattened, height*width*channels, party)

   // 5) Mean Pool with padding
  unflattened = unflatten(relu_output, channels, height, width);
  for (int chan = 0; chan < channels; chan++) {
    meanpool_output = MeanPooling(bitsize, (int64_t*) padded_output.matrix[chan], height, width, 3, 2, party);
    meanpool_matrix[chan] = meanpool_output.matrix;
  }
  input = meanpool_matrix;
  width = meanpool_output.width;
  height = meanpool_output.height;
  cout << "meanpooling_1 Done\n";
  cout << endl;

  // 6) Fully Connected Layer
  int vec_len = width * height * channels;

  int num_vec = 10;
  u64* output = (u64*) malloc(num_vec*sizeof(u64));
  for (int j = 0; j < num_vec; j++) {
    output[j] = 0;
  }

  
  io->send_data(&vec_len, sizeof(int));
  int split = 4000;
  for (int i = 0; i < vec_len; i+=split) {
    printf("min: %d\n", min(vec_len - i, split));

    data = fc_metadata(cfhe.encoder, min(vec_len - i, split), num_vec);


    client_shares = fc_pre(&cfhe, data, meanpool_matrix, i);

    io->send_data(&data, sizeof(Metadata));
    send_clientshare(io, &client_shares);
    recv_clientshare(io, &client_shares);

    temp = fc_post(&cfhe, data, client_shares, i);

    // temp = fc(&cfhe, &sfhe, min(vec_len - i, split), num_vec, relu_output, "./miniONN_cifar_model/dense.kernel.txt", i);
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





