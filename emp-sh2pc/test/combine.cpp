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
            return data4;
        }
    }
    cout << "file reading error\n";
    return data4;
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
    

    ClientShares client_shares = client_conv_preprocess(cfhe, &data, input);

    float endTime = (float)clock()/CLOCKS_PER_SEC;
    float timeElapsed = endTime - origin;
    printf("[%f seconds]\n", timeElapsed);

    vector<vector<vector<vector<int> > > > data4;
    string filename = "conv2d.kernel.txt";
    data4 = read_weights_4(filename);

    printf("Server Preprocessing: ");
    float startTime = (float)clock()/CLOCKS_PER_SEC;

    // Server creates filter
    u64*** filters = (u64***) malloc(sizeof(u64**)*data.out_chans);
    for (int out_c = 0; out_c < data.out_chans; out_c++) {
        filters[out_c] = (u64**) malloc(sizeof(u64*)*data.inp_chans);
        for (int inp_c = 0; inp_c < data.inp_chans; inp_c++) {
            filters[out_c][inp_c] = (u64*) malloc(sizeof(u64)*data.filter_size);
            int idx = 0;
                for (int channels = 0; channels < data4[0].size(); channels++) {
                    for (int width = 0; width < data4[0][0].size(); width++) {
                        for (int height = 0; height < data4[0][0][0].size(); height++) {
                            filters[out_c][inp_c][idx] = data4[out_c][channels][width][height] + PLAINTEXT_MODULUS;
                            idx++;
                        }
                    }
                }
        }
    }

    /*
    cout << "data4: " << data4[3][0][1][2] << "\n";
    cout << "filters: " << filters[3][0][5] << "\n";
    for (int fil = 0; fil < 1; fil++) {
        for (int idx = 0; idx < 9; idx++) {
            cout << filters[fil][0][idx] << "\n";
        }
    }
    */

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

    vector<vector<int> > data2;
    string filename = "dense.kernel.txt";
    data2 = read_weights_2(filename);

    u64** matrix = (u64**) malloc(sizeof(u64*)*matrix_h);
    for (int ct = 0; ct < matrix_h; ct++) {
        matrix[ct] = (u64*) malloc(sizeof(u64)*vector_len);
        // cout << data2[0].size() << "\n";
        for (int vecs = 0; vecs < vector_len; vecs++) {
            //cout << "ct: " << ct << "\n";
            //cout << "vecs: " << vecs << "\n";
            matrix[ct][vecs] = data2[ct][vecs] + PLAINTEXT_MODULUS;
        }
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

  //conv(&cfhe, &sfhe, 5, 5, 3, 3, 1, 4, 1, 1);
  //conv(&cfhe, &sfhe, 32, 32, 3, 3, 16, 16, 1, 0);
  //conv(&cfhe, &sfhe, 16, 16, 3, 3, 32, 32, 1, 1);
  //conv(&cfhe, &sfhe, 8, 8, 3, 3, 64, 64, 1, 1);
  
  fc(&cfhe, &sfhe, 2704, 10);
  
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





