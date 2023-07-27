// written by Dagny Brand and Jeremy Stevens

#include "max_pooling.h"

MaxPoolOutput MaxPooling(int bitsize, int64_t* inputs_a, int height, int width, int window_size, int stride, int party, unsigned dup_test) {
        std::vector<double> iotime_ms;

        MaxPoolOutput output_struct;
        vector<vector<Integer> > output(height/window_size, vector<Integer>(width/window_size, Integer(bitsize, 0, PUBLIC))); // Integer product(bitsize, 0, PUBLIC);
        // vector<Integer> zero_int(len, Integer(bitsize, 0, PUBLIC)); // Integer zero_int(bitsize, 0, PUBLIC);
        /*
        if ( height == 16 ) {
            cout << "Test with height == 16\n";
        }
        */

        struct timeval t_start_a, t_end_a, t_end_run, t_end_reveal;
        vector<vector<Integer> > a(height, vector<Integer>(width));
        gettimeofday(&t_start_a, NULL);
        int idx = 0;
        for (int i = 0; i < height; i++) {
            for (int j = 0; j < width; ++j){
                a[i][j] = Integer(bitsize, party == ALICE ? inputs_a[idx] : 0, ALICE);
                idx++;
            }
        }


        gettimeofday(&t_end_a, NULL);
        int out_i = 0;
        int out_j = 0;
        for(unsigned m = 0; m < dup_test; ++m){ // 10 duplicate test
            for (int i = 0; i < height - window_size + 1; i+=stride) {
                for (int j = 0; j < width - window_size + 1; j+=stride){
                    Integer max = Integer(bitsize, 0, PUBLIC);
                    for (int k = 0; k < window_size; k++) {
                        for (int l = 0; l < window_size; l++) {
                            // max = a[i + k][j + l];
                            ifThenElse(&max.bits[0], &a[i+k][j+l].bits[0], &max.bits[0], bitsize, a[i+k][j+k].geq(max));
                        }
                    }
                    output[out_i][out_j] = max;
                    out_j++;
                }
                out_j = 0;
                out_i++;
            }
        }

        gettimeofday(&t_end_run, NULL);
        u64* product_reveal = (u64*) malloc( (sizeof(u64) * height * width) );
        idx = 0;
        for (int i = 0; i < height / window_size; ++i) {
            for (int j = 0; j < width / window_size; ++j) {
                product_reveal[idx] = output[i][j].reveal<int>(ALICE);
                idx++;
            }
        }

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

        output_struct.matrix = product_reveal;
        output_struct.height = height / window_size;
        output_struct.width = width / window_size;

        return output_struct;
}