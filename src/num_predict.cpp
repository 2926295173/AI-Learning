#include <iostream>
#include <fstream>
#include <string>
#include <sstream>
#include <vector>
#include <cmath>
#include <iomanip>
#include <random>

using namespace std;

//�������-0.5��0.5����Ϊÿ����ʼȨ�غ�ƫ��
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
    } else {
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


//��������-��ӡ����
void pv(const std::vector<double>& vec) {
    std::cout << "[";
    for (std::size_t i = 0; i < vec.size(); ++i) {
        std::cout << std::fixed << std::setprecision(2) << vec[i];
        if (i != vec.size() - 1) {
            std::cout << ", ";
        }
    }
    std::cout << "]" << std::endl;
}

//ȫ�ֱ���
std::vector<std::vector<std::vector<std::vector<double>>>> network;//ģ��Ȩ�غ�ƫ��
std::vector<std::vector<double>> networkn;//���м���ֵ
std::vector<std::vector<double>> networkb;//���м���ǰ��wx+b��ֵ
double rate;//ѧϰ��
double aim;//Ŀ����ʧֵ

//һ�����ز���Ԫ
vector<double> neuron(std::vector<double> w, std::vector<double> x, double b) {
    //�����-ELU
    auto elu = [](double x) {
        if (x >= 0) {
            return x;
        }
        else {
            return 1.0 * (exp(x) - 1);
        }
    };

    //��͡�wx+b
    auto sigma = [&w, &x, b]() {
        double sum = 0;
        for (int i = 0; i < w.size(); ++i) {
            sum += w[i] * x[i];
        }
        return sum + b;
    };

    double sum = sigma();//����ǰ��ֵ

    return { sum,elu(sum) };
}

//һ���������Ԫ
double sneuron(std::vector<double> w, std::vector<double> x, double b) {
    //��͡�wx+b
    auto sigma = [&w, &x, b]() {
        double sum = 0;
        for (int i = 0; i < x.size(); ++i) {
            sum += w[i] * x[i];
        }
        return sum + b;
    };

    return sigma();
}

//ELU������ĵ���
double elu_derivative(double x) {
    if (x >= 0) {
        return 1;
    } else {
        return 1.0 * exp(x);
    }
}

//�����������ͳ��ģ�����ѵ�����
double MSE(double out, double out_hat) {
    return (out - out_hat) * (out - out_hat);
}

//Softmax��Ϊ����㼤���
vector<double> softmax(std::vector<double> output, double sum) {
    std::vector<double> yhat(output.size(), 0);//��������м���ֵ
    for (int i = 0; i <= output.size()-1; i++) {
        yhat[i] += exp(output[i]) / sum;//����ÿһ������ֵ
    }
    return yhat;
}


//Ԥ��-��ǰ����
vector<double> predict(vector<double> content) {
    //��ǰ���������ز�
    for (int m = 0; m <= networkn[0].size() - 1; m++) {
        auto r0 = neuron(network[0][m][0], content, network[0][m][1][0]);
        networkb[0][m] = r0[0];//δ����ֵ
        networkn[0][m] = r0[1];//����ֵ
    }

    //��ǰ�����������
    for (int n = 0; n <= networkn[1].size() - 1; n++) {
        auto r1 = sneuron(network[1][n][0], networkn[0], network[1][n][1][0]);
        networkb[1][n] = r1;//δ����ֵ
    }
    
    //softmax���������
    double sum = 0;//Softmax�ķ�ĸ��ÿ������ֵ���ã�
    for (int i = 0; i <= networkb[1].size()-1; i++) {
        sum += exp(networkb[1][i]);
    }
    networkn[1] = softmax(networkb[1], sum);//����ÿ�������Ԫ����ֵ

    return networkn[1];
}


//ѵ��-���򴫲�-����ݶ��½�
double trainNet(vector<vector<double>> dt) {
    std::vector<double> out_hat = predict(dt[0]);//Ԥ��-ǰ��
    double MSError = 0;
    //�������ֵ��MSE���ѵ��Ч��
    for (int l = 0; l <= out_hat.size() - 1; l++) {
        MSError += MSE(dt[1][l], out_hat[l]);
    }

    //Ϊÿ�������Ԫ�ֱ����ѧϰ�ʳ�����ʧֵ���������Ԫδ����ֵ��ƫ�����������������
    std::vector<double> rMEdN;
    for (int l = 0; l <= out_hat.size() - 1; l++) {
        rMEdN.push_back(rate * (out_hat[l] - dt[1][l]));//out_hat[l] - dt[1][l] ��Softmax+����ཻ���ؽ���󵼣�ʹ�ý�������Ϊ�����ݶ�ʱ����ʧ��������ʹ��MSE�����ڷ���ͳ����������ѵ�����̣�
    }

    //���������Ȩ��
    for (int p = 0; p <= networkn[1].size() - 1; p++) {//��p�������Ԫ
        for (int q = 0; q <= network[1][p][0].size() - 1; q++) {//��q��Ȩ��
            network[1][p][0][q] -= rMEdN[p] * networkn[0][q];
        }
    }

    //���������ƫ��
    for (int p = 0; p <= networkn[1].size() - 1; p++) {//��p�������Ԫ
        network[1][p][1][0] -= rMEdN[p];
    }

    //�������ز�Ȩ��
    for (int p = 0; p <= networkn[0].size() - 1; p++) {//��p��������Ԫ
        for (int q = 0; q <= network[0][p][0].size() - 1; q++) {//��q��Ȩ��
            //����ÿ�������Ԫ����ʧ�Դ�������Ԫ��ƫ��������ֱ����ͣ�Ҳ������ƽ����׼ȷ��˵Ӧ����ͣ���������Ͳ��ٸı������ˣ���⼴�ɣ�
            double averagenN = 0;
            for (int s = 0; s <= network[1].size() - 1; s++) {
                averagenN += rMEdN[s] * network[1][s][0][p];//��s�������Ԫ���ݶ� ���� �����ŵ�s�������Ԫ��Ȩ��
            }
            network[0][p][0][q] -= averagenN * elu_derivative(networkb[0][p]) * dt[0][q];
        }
    }

    //�������ز�ƫ�ã�ͬ��
    for (int p = 0; p <= networkn[0].size() - 1; p++) {
        double averagenN = 0;
        for (int s = 0; s <= network[1].size() - 1; s++) {
            averagenN += rMEdN[s] * network[1][s][0][p];
        }
        network[0][p][1][0] -= averagenN * elu_derivative(networkb[0][p]);
    }

    return MSError;//����ͳ�����
}

void train(vector<vector<vector<double>>> dt) {
    std::cout << "Gradient loss function: Cross Entropy" << std::endl;
    int i = 0;
    while (true) {
        i++;
        double err = 0;
        //�ݶ��½����ÿ��ѵ�����ݽ��и����Ż�����
        for (int c = 0; c <= dt.size() - 1; c++) {
            double preErr = trainNet(dt[c]);//�ݶ��½�һ��
            err += preErr;
        }
        
        if (i % 1 == 0) {
            rate *= 1.1;
        }
        if (i % 5 == 0) {
            rate *= 1.1;
        }

        //�ж���ʧֵ�Ƿ�����Ҫ��С�ڵ���Ŀ����ʧֵ
        if (err <= aim) {
            std::cout << "Training completed with err <= " << aim << " (" << err << ")" << std::endl;
            std::cout << ">>> finished " << dt.size() * i << " steps (" << i << " rounds) gradient descent in " << /*elapsed + */"ms <<<" << std::endl;
            break;
        }
        else {
            std::cout << "Round: " << i << "  Training: " << dt.size() * i << "  MSE: " << err << " rate: " << rate << std::endl;
        }
    }
}


int main() {
    networkn = {
        {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0},
        {0,0,0,0,0,0,0,0,0,0}
    };
    networkb = {
        {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0},
        {0,0,0,0,0,0,0,0,0,0}
    };
    
    /*ѵ����ɺ�Ԥ��ʱʹ�����´���
    network = loadNetwork("./num_predict.bin");
    //���´�������Ԥ��������ݼ�
    for (int dt0 = 0; dt0 <= 9; dt0++) {
        cout << "----------" << dt0 << "----------" << endl;
        for (int dt1 = 0; dt1 <= 10; dt1++) {
            pv(predict(getData("./data/testing/" + to_string(dt0) + "/" + to_string(dt1) + ".txt")));
        }
    }
    //���´�������Ԥ������д������
    pv(predict(getData("./your_data_path.txt")));
    */

    //ѵ��������ʹ�����´��룬Ԥ��ʱɾ�����´���
    cout << "1/3 Generate Vector" << endl;
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
    rate = 0.0015;//ѧϰ��
    aim = 1;//Ŀ����ʧֵ
    std::vector<std::vector<std::vector<double>>> training_data;
    cout << "2/3 Load Training Data" << endl;
    for(int dt1=0;dt1<=100;dt1++){
        training_data.push_back({getData("./data/0/"+to_string(dt1)+".txt"), {1,0,0,0,0,0,0,0,0,0}});
        training_data.push_back({getData("./data/1/"+to_string(dt1)+".txt"), {0,1,0,0,0,0,0,0,0,0}});
        training_data.push_back({getData("./data/2/"+to_string(dt1)+".txt"), {0,0,1,0,0,0,0,0,0,0}});
        training_data.push_back({getData("./data/3/"+to_string(dt1)+".txt"), {0,0,0,1,0,0,0,0,0,0}});
        training_data.push_back({getData("./data/4/"+to_string(dt1)+".txt"), {0,0,0,0,1,0,0,0,0,0}});
        training_data.push_back({getData("./data/5/"+to_string(dt1)+".txt"), {0,0,0,0,0,1,0,0,0,0}});
        training_data.push_back({getData("./data/6/"+to_string(dt1)+".txt"), {0,0,0,0,0,0,1,0,0,0}});
        training_data.push_back({getData("./data/7/"+to_string(dt1)+".txt"), {0,0,0,0,0,0,0,1,0,0}});
        training_data.push_back({getData("./data/8/"+to_string(dt1)+".txt"), {0,0,0,0,0,0,0,0,1,0}});
        training_data.push_back({getData("./data/9/"+to_string(dt1)+".txt"), {0,0,0,0,0,0,0,0,0,1}});
    }
    cout << "3/3 Ready" << endl;
    train(training_data);
    saveNetwork(network, "./num_predict.bin");

    return 0;
}
