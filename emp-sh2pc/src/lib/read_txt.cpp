// written by Dagny Brand and Jeremy Stevens

#include "read_txt.h"

vector<vector<vector<int> > > read_image(string file) {
    ifstream fp;
    string line;
    vector<vector<vector<int> > > image_data;

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

        string line;
        getline(fp, line);
        string num = "";

        int m = 0;
        for (int i = 0; i < dims[0]; i++) {
            vector<vector<int> > empty2;
            image_data.push_back(empty2);
            for (int j = 0; j < dims[1]; j++) {
                vector<int> empty1;
                image_data[i].push_back(empty1);
                for (int k = 0; k < dims[2]; k++) {
                    while (line[m] != '\0') {
                        if (line[m] != ' ') {
                            num += line[m];
                        }
                        else {
                            image_data[i][j].push_back(stoi(num));
                            num = "";
                            m++;
                            break;
                        }
                        m++;
                    }
                }
            }
        }
        fp.close();
        return image_data;
    }
    cout << "file reading error\n";
    return image_data;
}

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
            fp.close();
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
            fp.close();
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
            fp.close();
            return data4;
        }
    }
    cout << "file reading error\n";
    return data4;
}
