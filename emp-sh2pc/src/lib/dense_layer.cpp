// edited by Dagny Brand and Jeremy Stevens

#include "dense_layer.h"

u64* fc(ClientFHE* cfhe, ServerFHE* sfhe, int vector_len, int matrix_h, u64* dense_input, string weights_filename, int split_index) {
    Metadata data = fc_metadata(cfhe->encoder, vector_len, matrix_h);
   
    // printf("\nClient Preprocessing: ");
    float origin = (float)clock()/CLOCKS_PER_SEC;

    u64* input = (u64*) malloc(sizeof(u64)*vector_len);
    for (int idx = 0; idx < vector_len; idx++)
        //input[idx] = 1;
        input[idx] = dense_input[idx + split_index];

    ClientShares client_shares = client_fc_preprocess(cfhe, &data, input);

    float endTime = (float)clock()/CLOCKS_PER_SEC;
    float timeElapsed = endTime - origin;
    // printf("[%f seconds]\n", timeElapsed);

    // printf("Server Preprocessing: ");
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
    // printf("[%f seconds]\n", timeElapsed);

    // printf("Layer: ");
    startTime = (float)clock()/CLOCKS_PER_SEC;

    server_fc_online(sfhe, &data, client_shares.input_ct, enc_matrix, &server_shares);

    endTime = (float)clock()/CLOCKS_PER_SEC;
    timeElapsed = endTime - startTime;
    // printf("[%f seconds]\n", timeElapsed);

    // This simulates the client receiving the ciphertexts 
    client_shares.linear_ct.inner = (char*) malloc(sizeof(char)*server_shares.linear_ct.size);
    client_shares.linear_ct.size = server_shares.linear_ct.size;
    memcpy(client_shares.linear_ct.inner, server_shares.linear_ct.inner, server_shares.linear_ct.size);

    // printf("Post process: ");
    startTime = (float)clock()/CLOCKS_PER_SEC;

    client_fc_decrypt(cfhe, &data, &client_shares);

    endTime = (float)clock()/CLOCKS_PER_SEC;
    timeElapsed = endTime - startTime;
    // printf("[%f seconds]\n", timeElapsed);

    timeElapsed = endTime - origin;
    // printf("Total [%f seconds]\n\n", timeElapsed);

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