// edited by Dagny Brand and Jeremy Stevens

#include "conv_layer.h"

ConvOutput conv(ClientFHE* cfhe, ServerFHE* sfhe, int image_h, int image_w, int filter_h, int filter_w,
    int inp_chans, int out_chans, int stride, bool pad_valid, string weights_filename, u64** input_data) {
    Metadata data = conv_metadata(cfhe->encoder, image_h, image_w, filter_h, filter_w, inp_chans, 
        out_chans, stride, stride, pad_valid);
    string filename;
    int SCALE = 256;
   
    // printf("\nClient Preprocessing: ");
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
    printf("Conv PreProcessing [%f seconds]\n", timeElapsed);

    vector<vector<vector<vector<int> > > > data4;
    //filename = "conv2d.kernel.txt";
    data4 = read_weights_4(weights_filename);
    int width = data4[0][0].size();
    int height = data4[0][0][0].size();

    // printf("Server Preprocessing: ");
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
    // printf("[%f seconds]\n", timeElapsed);

    // printf("Convolution: ");
    startTime = (float)clock()/CLOCKS_PER_SEC;

    server_conv_online(sfhe, &data, client_shares.input_ct, masks, &server_shares);

    endTime = (float)clock()/CLOCKS_PER_SEC;
    timeElapsed = endTime - startTime;
    printf("Conv processing time [%f seconds]\n", timeElapsed);

    // printf("Post process: ");
    startTime = (float)clock()/CLOCKS_PER_SEC;

    // This simulates the client receiving the ciphertexts 
    client_shares.linear_ct.inner = (char*) malloc(sizeof(char)*server_shares.linear_ct.size);
    client_shares.linear_ct.size = server_shares.linear_ct.size;
    memcpy(client_shares.linear_ct.inner, server_shares.linear_ct.inner, server_shares.linear_ct.size);
    client_conv_decrypt(cfhe, &data, &client_shares);

    endTime = (float)clock()/CLOCKS_PER_SEC;
    timeElapsed = endTime - startTime;
    // printf("[%f seconds]\n", timeElapsed);

    timeElapsed = endTime - origin;
    // printf("Total [%f seconds]\n\n", timeElapsed);

    
    
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
    server_conv_free(&data, masks, &server_shares);
    return conv_output;
}