#include <iostream>
#include <fstream>
#include <string>
#include <sstream>
#include <vector>
#include <cmath>
#include <iomanip>
#include <random>
#include <thread>
#include <mutex>
#include <Windows.h>
#include <functional>
#include <queue>
#include <condition_variable>
#include <chrono>
#include <windows.h>
#include "fast.h"


using namespace std;
std::mutex mtx;


std::vector<std::vector<double>> networkn;
std::vector<std::vector<double>> networkb;
std::vector<std::vector<std::vector<std::vector<double>>>> network;
std::vector<std::vector<std::vector<double>>> training_data;
int batchSize;

double MSETotal;
vector<vector<vector<vector<double>>>> networkgs(2);
bool gate;
double preErr;

int reportI = 0;


//��������ģ��Ȩ�غ�ƫ�ñ��浽�ļ�
void saveNetwork(const std::vector<std::vector<std::vector<std::vector<double>>>>& network, const std::string& filename) {
    std::ofstream file(filename, std::ios::binary);
    if (file.is_open()) {
        //��ȡ��ά vector ��ά����Ϣ
        std::size_t dim1 = network.size();
        std::size_t dim2 = network[0].size();
        std::size_t dim3 = network[0][0].size();
        std::size_t dim4 = network[0][0][0].size();

        //д��ά����Ϣ
        file.write(reinterpret_cast<const char*>(&dim1), sizeof(dim1));
        file.write(reinterpret_cast<const char*>(&dim2), sizeof(dim2));
        file.write(reinterpret_cast<const char*>(&dim3), sizeof(dim3));
        file.write(reinterpret_cast<const char*>(&dim4), sizeof(dim4));

        //д�����ݣ������ƣ�
        for (const auto& vec1 : network) {
            for (const auto& vec2 : vec1) {
                for (const auto& vec3 : vec2) {
                    file.write(reinterpret_cast<const char*>(vec3.data()), dim4 * sizeof(double));
                }
            }
        }

        file.close();
        std::cout << "Network saved to " << filename << std::endl;
    }
    else {
        std::cerr << "Unable to open file: " << filename << std::endl;
    }
}

//�Ӷ��������ݶ�ȡȨ��ƫ�ò���
std::vector<std::vector<std::vector<std::vector<double>>>> loadNetwork(const std::string& filename) {
    std::vector<std::vector<std::vector<std::vector<double>>>> network;

    std::ifstream file(filename, std::ios::binary);
    if (file.is_open()) {
        //��ȡά����Ϣ
        std::size_t dim1, dim2, dim3, dim4;
        file.read(reinterpret_cast<char*>(&dim1), sizeof(dim1));
        file.read(reinterpret_cast<char*>(&dim2), sizeof(dim2));
        file.read(reinterpret_cast<char*>(&dim3), sizeof(dim3));
        file.read(reinterpret_cast<char*>(&dim4), sizeof(dim4));

        //���� vector ��С
        network.resize(dim1);
        for (auto& vec1 : network) {
            vec1.resize(dim2);
            for (auto& vec2 : vec1) {
                vec2.resize(dim3);
                for (auto& vec3 : vec2) {
                    vec3.resize(dim4);
                }
            }
        }

        //��ȡ����
        for (auto& vec1 : network) {
            for (auto& vec2 : vec1) {
                for (auto& vec3 : vec2) {
                    file.read(reinterpret_cast<char*>(vec3.data()), dim4 * sizeof(double));
                }
            }
        }

        file.close();
        std::cout << "Network loaded from " << filename << std::endl;
    }
    else {
        std::cerr << "Unable to open file: " << filename << std::endl;
    }

    return network;
}


vector<double> predict(vector<double> dt) {
    //��ǰ���������ز�
    vector<vector<vector<vector<double>>>> networkg = network;
    for (int m = 0; m < networkn[0].size(); m++) {
        //��͡�wx+b
        networkb[0][m] = dot_product(network[0][m][0].size(), network[0][m][0].data(), dt.data()) + network[0][m][1][0];
        //����
        if (networkb[0][m] >= 0) {
            networkn[0][m] = networkb[0][m];
        }
        else {
            networkn[0][m] = exp(networkb[0][m]) - 1;
        }
    }

    //��ǰ�����������
    for (int n = 0; n < networkn[1].size(); n++) {
        networkb[1][n] = dot_product(network[1][n][0].size(), network[1][n][0].data(), networkn[0].data()) + network[1][n][1][0];//δ����ֵ
    }

    //softmax���������
    double SSum = 0;//Softmax�ķ�ĸ��ÿ������ֵ���ã�
    for (int i = 0; i < networkb[1].size(); i++) {
        SSum += exp(networkb[1][i]);
    }
    for (int i = 0; i < networkb[1].size(); i++) {
        networkn[1][i] += exp(networkb[1][i]) / SSum;//����ÿһ������ֵ
    }

    cout << networkn[1][0] << endl;
    return networkn[1];
}


void report(vector<vector<vector<vector<vector<double>>>>> result) {
    MSETotal += result[1][0][0][0][0];
    reportI++;

    for (int p = 0; p < networkn[1].size(); p++) {
        add_arrays(network[1][p][0].size(), result[0][1][p][0].data(), networkgs[1][p][0].data());
    }
    for (int p = 0; p < networkn[1].size(); p++) {
        networkgs[1][p][1][0] += result[0][1][p][1][0];
    }
    for (int p = 0; p < networkn[0].size(); p++) {
        add_arrays(network[0][p][0].size(), result[0][0][p][0].data(), networkgs[0][p][0].data());
    }
    for (int p = 0; p < networkn[0].size(); p++) {
        networkgs[0][p][1][0] += result[0][0][p][1][0];
    }

    if (reportI == batchSize) {
        for (int p = 0; p < networkn[1].size(); p++) {
            add_arrays(network[1][p][0].size(), networkgs[1][p][0].data(), network[1][p][0].data());
        }
        for (int p = 0; p < networkn[1].size(); p++) {
            network[1][p][1][0] += networkgs[1][p][1][0];
        }
        for (int p = 0; p < networkn[0].size(); p++) {
            add_arrays(network[0][p][0].size(), networkgs[0][p][0].data(), network[0][p][0].data());
        }
        for (int p = 0; p < networkn[0].size(); p++) {
            network[0][p][1][0] += networkgs[0][p][1][0];
        }


        for (int p = 0; p < networkn[1].size(); p++) {
            scale_product(networkgs[1][p][0].size(), networkgs[1][p][0].data(), 0);
        }
        for (int p = 0; p < networkn[1].size(); p++) {
            networkgs[1][p][1][0] = 0;
        }
        for (int p = 0; p < networkn[0].size(); p++) {
            scale_product(networkgs[0][p][0].size(), networkgs[0][p][0].data(), 0);
        }
        for (int p = 0; p < networkn[0].size(); p++) {
            networkgs[0][p][1][0] = 0;
        }


        preErr = MSETotal;
        MSETotal = 0;
        reportI = 0;
        gate = true;
    }
}


int trainNet(vector<vector<double>> dt, vector<vector<vector<vector<double>>>> network, vector<vector<double>> networkn, vector<vector<double>> networkb, double rate, std::function<void(vector<vector<vector<vector<vector<double>>>>> result)> callback) {
    //��ǰ���������ز�
    vector<vector<vector<vector<double>>>> networkg = network;
    for (int m = 0; m < networkn[0].size(); m++) {
        //��͡�wx+b
        networkb[0][m] = dot_product(network[0][m][0].size(), network[0][m][0].data(), dt[0].data()) + network[0][m][1][0];
        //����
        if (networkb[0][m] >= 0) {
            networkn[0][m] = networkb[0][m];
        }
        else {
            networkn[0][m] = exp(networkb[0][m]) - 1;
        }
    }

    //��ǰ�����������
    for (int n = 0; n < networkn[1].size(); n++) {
        networkb[1][n] = dot_product(network[1][n][0].size(), network[1][n][0].data(), networkn[0].data()) + network[1][n][1][0];//δ����ֵ
    }

    //softmax���������
    double SSum = 0;//Softmax�ķ�ĸ��ÿ������ֵ���ã�
    for (int i = 0; i < networkb[1].size(); i++) {
        SSum += exp(networkb[1][i]);
    }
    for (int i = 0; i < networkb[1].size(); i++) {
        networkn[1][i] += exp(networkb[1][i]) / SSum;//����ÿһ������ֵ
    }



    double MSError = 0;
    //�������ֵ��MSE���ѵ��Ч��
    for (int l = 0; l < networkn[1].size(); l++) {
        MSError += ((dt[1][l] - networkn[1][l]) * (dt[1][l] - networkn[1][l]));
    }

    //Ϊÿ�������Ԫ�ֱ����ѧϰ�ʳ�����ʧֵ���������Ԫδ����ֵ��ƫ�����������������
    std::vector<double> rMEdN;
    for (int l = 0; l < networkn[1].size(); l++) {
        rMEdN.push_back(-rate * (networkn[1][l] - dt[1][l]));//networkn[1][l] - dt[1][l] ��Softmax+����ཻ���ؽ���󵼣�ʹ�ý�������Ϊ�����ݶ�ʱ����ʧ��������ʹ��MSE�����ڷ���ͳ����������ѵ�����̣�
    }

    //���������Ȩ��
    for (int p = 0; p < networkn[1].size(); p++) {//��p�������Ԫ
        networkg[1][p][0] = networkn[0];
        scale_product(networkg[1][p][0].size(), networkg[1][p][0].data(), rMEdN[p]);
    }

    //���������ƫ��
    for (int p = 0; p < networkn[1].size(); p++) {//��p�������Ԫ
        networkg[1][p][1][0] = rMEdN[p];
    }

    //�������ز�
    for (int p = 0; p < networkn[0].size(); p++) {//��p��������Ԫ
        //����ÿ�������Ԫ����ʧ�Դ�������Ԫ��ƫ��������ֱ����ͣ�Ҳ������ƽ����׼ȷ��˵Ӧ����ͣ���������Ͳ��ٸı������ˣ���⼴�ɣ�
        double averagenN = 0;
        for (int s = 0; s < network[1].size(); s++) {
            averagenN += rMEdN[s] * network[1][s][0][p];//��s�������Ԫ���ݶ� ���� �����ŵ�s�������Ԫ��Ȩ��
        }
        if (networkb[0][p] >= 0) {
            networkg[0][p][1][0] = averagenN;
        }
        else {
            networkg[0][p][1][0] = averagenN * exp(networkb[0][p]);
        }

        networkg[0][p][0] = dt[0];
        scale_product(network[0][p][0].size(), networkg[0][p][0].data(), networkg[0][p][1][0]);
    }


    mtx.lock();
    callback({ networkg , {{{{MSError}}}} });
    mtx.unlock();
    return 0;
}


void train(vector<vector<vector<double>>> dt, double rate, double aim) {
    std::cout << "Gradient loss function: Cross Entropy" << std::endl;
    int i = 0;
    while (true) {
        i++;
        double err = 0;
        //�ݶ��½����ÿ��ѵ�����ݽ��и����Ż�����
        auto start = std::chrono::high_resolution_clock::now();
        for (int c = 0; c < dt.size() / batchSize; c++) {
            while (true) {
                if (gate == true) {
                    err += preErr;
                    gate = false;
                    for (int w = 0; w < batchSize; w++) {
                        thread worker(trainNet, training_data[c + w], network, networkn, networkb, rate, report);
                        worker.detach();
                    }
                    break;
                }
            }
        }
        auto end = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double> duration = end - start;
        cout << "spent time: " << duration.count() << "s" << endl;

        if (i % 1 == 0) {
            rate *= 1.1;
        }
        if (i % 5 == 0) {
            rate *= 1.1;
        }

        //�ж���ʧֵ�Ƿ�����Ҫ��С�ڵ���Ŀ����ʧֵ
        if (err <= aim) {
            std::cout << ">>> finished " << dt.size() * i << " steps (" << i << " rounds) gradient descent in " << std::endl;
            break;
        }
        else {
            std::cout << "Round: " << i << "  Training: " << dt.size() * i << "  MSE: " << err << " rate: " << rate << std::endl;
        }
    }
}


//�������ÿ����ʼȨ�غ�ƫ��
std::vector<double> generateVector(int length) {
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_real_distribution<double> dis(-0.5, 0.5);
    std::vector<double> result(length);
    for (int i = 0; i <= length - 1; i++) {
        result[i] = dis(gen);
    }
    return result;
}

//��ȡ�Ҷ��ı����ݲ�����Ϊvector
vector<double> getData(std::string path) {
    std::ifstream file(path);
    if (!file.is_open()) {
        std::cout << "Failed" << std::endl;
        exit(1);
    }

    std::string content;
    std::getline(file, content);
    file.close();

    std::vector<double> numbers;
    std::stringstream ss(content);
    std::string token;

    while (std::getline(ss, token, ',')) {
        double number = std::stof(token);
        numbers.push_back(number);
    }

    return numbers;
}


int main() {
    gate = true;
    MSETotal = 0;
    batchSize = 10;
    networkn = {
        {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0},
        {0,0,0,0,0,0,0,0,0,0}
    };
    networkb = {
        {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0},
        {0,0,0,0,0,0,0,0,0,0}
    };
    network = {
        {
            {generateVector(784), {0}},
            {generateVector(784), {0}},
            {generateVector(784), {0}},
            {generateVector(784), {0}},
            {generateVector(784), {0}},
            {generateVector(784), {0}},
            {generateVector(784), {0}},
            {generateVector(784), {0}},
            {generateVector(784), {0}},
            {generateVector(784), {0}},
            {generateVector(784), {0}},
            {generateVector(784), {0}},
            {generateVector(784), {0}},
            {generateVector(784), {0}},
            {generateVector(784), {0}},
            {generateVector(784), {0}},
            {generateVector(784), {0}},
            {generateVector(784), {0}},
            {generateVector(784), {0}},
            {generateVector(784), {0}},
            {generateVector(784), {0}},
            {generateVector(784), {0}},
            {generateVector(784), {0}},
            {generateVector(784), {0}},
            {generateVector(784), {0}},
            {generateVector(784), {0}},
            {generateVector(784), {0}},
            {generateVector(784), {0}},
            {generateVector(784), {0}},
            {generateVector(784), {0}},
            {generateVector(784), {0}},
            {generateVector(784), {0}},
            {generateVector(784), {0}},
            {generateVector(784), {0}},
            {generateVector(784), {0}},
            {generateVector(784), {0}},
            {generateVector(784), {0}},
            {generateVector(784), {0}},
            {generateVector(784), {0}},
            {generateVector(784), {0}},
            {generateVector(784), {0}},
            {generateVector(784), {0}},
            {generateVector(784), {0}},
            {generateVector(784), {0}},
            {generateVector(784), {0}},
            {generateVector(784), {0}},
            {generateVector(784), {0}},
            {generateVector(784), {0}},
            {generateVector(784), {0}},
            {generateVector(784), {0}},
            {generateVector(784), {0}},
            {generateVector(784), {0}},
            {generateVector(784), {0}},
            {generateVector(784), {0}},
            {generateVector(784), {0}},
            {generateVector(784), {0}},
            {generateVector(784), {0}},
            {generateVector(784), {0}},
            {generateVector(784), {0}},
            {generateVector(784), {0}},
        },
        {
            {generateVector(30), {0}},
            {generateVector(30), {0}},
            {generateVector(30), {0}},
            {generateVector(30), {0}},
            {generateVector(30), {0}},
            {generateVector(30), {0}},
            {generateVector(30), {0}},
            {generateVector(30), {0}},
            {generateVector(30), {0}},
            {generateVector(30), {0}}
        }
    };

    networkgs = network;
    for (int p = 0; p <= networkn[1].size() - 1; p++) {
        scale_product(networkgs[1][p][0].size(), networkgs[1][p][0].data(), 0);
    }
    for (int p = 0; p <= networkn[1].size() - 1; p++) {
        networkgs[1][p][1][0] = 0;
    }
    for (int p = 0; p <= networkn[0].size() - 1; p++) {
        scale_product(networkgs[0][p][0].size(), networkgs[0][p][0].data(), 0);
    }
    for (int p = 0; p <= networkn[0].size() - 1; p++) {
        networkgs[0][p][1][0] = 0;
    }

    double rate = 0.001;//ѧϰ��
    double aim = 1e-5;//Ŀ����ʧֵ
    cout << "2/3 Load Training Data" << endl;
    for (int dt1 = 0; dt1 <= 100; dt1++) {
        training_data.push_back({ getData("./data/0/" + to_string(dt1) + ".txt"), {1,0,0,0,0,0,0,0,0,0} });
        training_data.push_back({ getData("./data/1/" + to_string(dt1) + ".txt"), {0,1,0,0,0,0,0,0,0,0} });
        training_data.push_back({ getData("./data/2/" + to_string(dt1) + ".txt"), {0,0,1,0,0,0,0,0,0,0} });
        training_data.push_back({ getData("./data/3/" + to_string(dt1) + ".txt"), {0,0,0,1,0,0,0,0,0,0} });
        training_data.push_back({ getData("./data/4/" + to_string(dt1) + ".txt"), {0,0,0,0,1,0,0,0,0,0} });
        training_data.push_back({ getData("./data/5/" + to_string(dt1) + ".txt"), {0,0,0,0,0,1,0,0,0,0} });
        training_data.push_back({ getData("./data/6/" + to_string(dt1) + ".txt"), {0,0,0,0,0,0,1,0,0,0} });
        training_data.push_back({ getData("./data/7/" + to_string(dt1) + ".txt"), {0,0,0,0,0,0,0,1,0,0} });
        training_data.push_back({ getData("./data/8/" + to_string(dt1) + ".txt"), {0,0,0,0,0,0,0,0,1,0} });
        training_data.push_back({ getData("./data/9/" + to_string(dt1) + ".txt"), {0,0,0,0,0,0,0,0,0,1} });
    }
    cout << "3/3 Ready" << endl;
    train(training_data, rate, aim);
}