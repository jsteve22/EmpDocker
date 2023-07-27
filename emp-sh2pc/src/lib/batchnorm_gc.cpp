// written by Dagny Brand and Jeremy Stevens

#include "batchnorm_gc.h"

u64* batchnorm_gc(int bitsize, int64_t* inputs_a, int height, int width, int channels, int party, string weights_path, unsigned dup_test) {
  std::vector<double> iotime_ms;

  cout << "party: " << party << endl;
  
  vector<int> biases(channels);
  vector<int> means(channels); 
  vector<int> vars(channels);  
  vector<int> weights(channels);

  if (party == BOB) {
    biases  = read_weights_1(weights_path + ".bias.txt");
    means   = read_weights_1(weights_path + ".running_mean.txt");
    vars    = read_weights_1(weights_path + ".running_var.txt");
    weights = read_weights_1(weights_path + ".weight.txt");
  }

  vector<Integer> output(height*width*channels, Integer(bitsize, 0, PUBLIC)); // Integer product(bitsize, 0, PUBLIC);

  int vec_size = width*height*channels;
  struct timeval t_start_a, t_end_a, t_end_run, t_end_reveal;
  vector<Integer > a(vec_size);
  gettimeofday(&t_start_a, NULL);
  for (int i = 0; i < vec_size; i++) {
    a[i] = Integer(bitsize, party == ALICE ? inputs_a[i] : 0, ALICE);
  }

  cout << "filled a in batchnorm: " << party << endl;

  Integer bias, mean, var, weight;
  int idx;

  gettimeofday(&t_end_a, NULL);
  for(unsigned m = 0; m < dup_test; ++m){ // 10 duplicate test
    for (int i = 0; i < vec_size; i++) {
      idx = i / (width*height);
      
      if (i == 0) {
        cout << biases[idx] << " ";
        cout << means[idx] << " ";
        cout << (int)sqrt((double)vars[idx]) << " ";
        cout << weights[idx] << "\n";
      }

      Integer bias   = Integer(bitsize, party==BOB ? biases[idx] : 0, BOB);
      Integer mean   = Integer(bitsize, party==BOB ? means[idx] : 0, BOB);
      Integer var    = Integer(bitsize, party==BOB ? ((vars[idx]) ? vars[idx] : 1) : 0, BOB);
      Integer weight = Integer(bitsize, party==BOB ? weights[idx] : 0, BOB);

      // output[i] = ((a[i]-mean)/var)*weight + bias;
      output[i] = (a[i]-mean);
      output[i] = output[i] * weight;
      output[i] = output[i] / var;
      output[i] = output[i] + bias;
    }
  }

  cout << "computed output in batchnorm: " << party << endl;

  gettimeofday(&t_end_run, NULL);
  u64* product_reveal = (u64*) malloc( (sizeof(u64) * vec_size) );
  for (int i = 0; i < vec_size; ++i) {
    product_reveal[i] = output[i].reveal<int>(ALICE);
  }

  cout << "product reveal in batch norm: " << party << endl;

  gettimeofday(&t_end_reveal, NULL);

  /*
  double mseconds_a = 1000 * (t_end_a.tv_sec - t_start_a.tv_sec) + (t_end_a.tv_usec - t_start_a.tv_usec) / 1000.0;
  double mseconds_run = (1000 * (t_end_run.tv_sec - t_end_a.tv_sec) + (t_end_run.tv_usec - t_end_a.tv_usec) / 1000.0);
  double mseconds_reveal = 1000 * (t_end_reveal.tv_sec - t_end_run.tv_sec) + (t_end_reveal.tv_usec - t_end_run.tv_usec) / 1000.0;
  double io_ms = 0;
  for (size_t i = 0; i < iotime_ms.size(); i++)
  {
    io_ms += iotime_ms[i];
  }

  printf("Running Party: %d\n", party);
  printf("%d %d Time of init a:            %f ms\n", party, getpid(), mseconds_a);
  printf("%d %d Time of run circuit:       %f ms\n", party, getpid(), mseconds_run);
  printf("%d %d Time of REAL RUN circuit:  %f ms\n", party, getpid(), mseconds_run-io_ms);
  printf("%d %d Time of reveal the output: %f ms\n", party, getpid(), mseconds_reveal);
  */

  cout << product_reveal[0] << " " << party << endl;

  return product_reveal;
}