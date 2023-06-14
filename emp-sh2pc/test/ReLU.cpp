#include <stdio.h>
#include "emp-sh2pc/emp-sh2pc.h"
#include <sys/time.h>
#include <iostream>
#include <inttypes.h>
std::vector<double> iotime_ms;
std::vector<double> ANDgatetime_ms, XORgatetime_ms;
using namespace emp;
using namespace std;

void ReLU(int bitsize, vector<int> inputs_a, int len, int party = 0, unsigned dup_test=10)
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
        vector<int> product_reveal(len);
        for (int j = 0; j < len; ++j){
                product_reveal[j] = product[j].reveal<int>(ALICE);
        }
        gettimeofday(&t_end_reveal, NULL);
        for (int j = 0; j < len; ++j){
                cout<< "Running Party: " <<party<< " ReLU(" << inputs_a[j] << ") = " << product_reveal[j] << endl;
        }

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
}

int main(int argc, char **argv)
{
        int bitsize = 32;
        int LEN = 256;

        // generate circuit for use in malicious library
        if (argc == 4 && strcmp(argv[1], "-m") == 0)
        {
                setup_plain_prot(true, "ReLU.circuit.txt");
                bitsize = stoi(argv[2]);
                LEN = stoi(argv[3]);
                vector<int> inputs(LEN, 0);
                ReLU(bitsize, inputs, LEN, PUBLIC, 1);
                finalize_plain_prot();
                return 0;
        }

        // run computation with semi-honest model
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

        /*
        char fname_a[40];

        sprintf(fname_a, "data/ReLU/%d.1.dat", bitsize);

        ifstream infile_a(fname_a);

        string readinputs_a;
        vector<int> inputs_a;

        if (infile_a.is_open())
        {
                for (int i = 0; i < LEN; i++)
                {
                        getline(infile_a, readinputs_a);
                        int read_temp = stoi(readinputs_a);
                        inputs_a.push_back(read_temp);
                }
                
                infile_a.close();
        }
        */
        vector<int> inputs_a;
        for (int i = 0; i < LEN; i++) {
                inputs_a.push_back( 1 << (3*i) );
        }

        ReLU(bitsize, inputs_a, LEN, party);

        delete io;
}
