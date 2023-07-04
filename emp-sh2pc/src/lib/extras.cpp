#include "extras.h"

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
        printf("%lu, " , reduce(client_a[i] + server_a[i]));
    }
    printf("] \n");

    printf("B    : [");
    for (int i = 0; i < num_triples; i++) {
        printf("%lu, " , reduce(client_b[i] + server_b[i]));
    }
    printf("] \n");

    printf("C    : [");
    for (int i = 0; i < num_triples; i++) {
        u64 a = client_a[i] + server_a[i];
        u64 b = client_b[i] + server_b[i];
        u64 c = reduce(client_shares.c_share[i] + server_c[i]);
        printf("%lu, " , c);
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
