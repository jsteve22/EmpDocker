// written by Dagny Brand and Jeremy Stevens

#include "read_weights.h"

template <typename T> T read_weights(string file) {
    ifstream fp;
    string line;

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

        vector<float> data1;
        vector<vector<float> > data2;
        vector<vector<vector<vector<float> > > > data4;

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
                        data1.push_back(stof(num));
                        num = "";
                    }
                    j++;
                }
            }
            return data1;
        }

        if (dims.size() == 2) {
            //
            string line;
            getline(fp, line);
            string num = "";

            int k = 0;
            for (int i = 0; i < dims[0]; i++) {
                vector<float> empty;
                data2.push_back(empty);
                for (int j = 0; i < dims[1]; i++) {
                    while (line[k] != '\0') {
                        if (line[k] != ' ') {
                            num += line[k];
                        }
                        else {
                            data2[i].push_back(stof(num));
                            num = "";
                        }
                        k++;
                    }
                }
            }
            return data2;
        }



        if (dims.size() == 4) {
            string line;
            getline(fp, line);
            string num = "";

            int m = 0;
            for (int i = 0; i < dims[0]; i++) {
                vector<vector<vector<float> > > empty3;
                data4.push_back(empty3);
                for (int j = 0; j < dims[1]; j++) {
                    vector<vector<float> > empty2;
                    data4[i].push_back(empty2);
                    for (int k = 0; k < dims[2]; k++) {
                        vector<float> empty1;
                        data4[i][j].push_back(empty1);
                        for (int l = 0; l < dims[3]; l++) {
                            while (line[m] != '\0') {
                                if (line[m] != ' ') {
                                    num += line[m];
                                }
                                else {
                                    data4[i][j][k].push_back(stof(num));
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
}