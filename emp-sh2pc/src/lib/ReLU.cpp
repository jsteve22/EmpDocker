// edited by Dagny Brand and Jeremy Stevens

#include "ReLU.h"

u64* ReLU(int bitsize, int64_t* inputs_a, int len, int party, unsigned dup_test)
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
                        // ifThenElse function in file emp-tool/emp-tool/circuits/integer.hpp
                        // ifThenElse(Bit *dest, const Bit *tsrc, const Bit *fsrc, int size, Bit cond)
                        ifThenElse(&product[j].bits[0], &a[j].bits[0], &zero_int[j].bits[0], bitsize, a[j].geq(zero_int[j]));
                }
        }
        gettimeofday(&t_end_run, NULL);
        u64* product_reveal = (u64*) malloc( (sizeof(u64) * len) );
        for (int j = 0; j < len; ++j){
                product_reveal[j] = product[j].reveal<int>(ALICE);
        }
        gettimeofday(&t_end_reveal, NULL);
        /*
        for (int j = 0; j < len; ++j){
                cout<< "Running Party: " <<party<< " ReLU(" << inputs_a[j] << ") = " << product_reveal[j] << endl;
        }
        */

        /*
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
        */

        return product_reveal;
}