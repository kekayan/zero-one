#include <iostream>
#include <string>
#include <random>
#include <fstream>

int main(int argc, char* argv[]) {
    if (argc != 4) {
        std::cerr << "Usage: ./one <time_step> <value1> <value2>" << std::endl;
        return 1;
    }

    try {
        int time_step = std::stoi(argv[1]);
        int value1 = std::stoi(argv[2]);
        int value2 = std::stoi(argv[3]);

        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_int_distribution<> dis(0, 100);

        // Generate data
        std::vector<int> q_data;
        if (time_step == 0) {
            // Initial state: [0, random, 0]
            q_data = {0, dis(gen), 0};
        } else {
            // Subsequent states: [time_step, random, value1]
            q_data = {time_step, dis(gen), value1};
        }

        // write to one.txt
        std::ofstream one_file("one.txt", std::ios::app);
        one_file << q_data[0] << " " << q_data[1] << " " << q_data[2] << std::endl;
        one_file.close();

        std::cout << "[" << q_data[0] << ", " << q_data[1] << ", " << q_data[2] << "]" << std::endl;

        return 0;
    } catch (const std::exception& e) {
        std::cerr << "Error: All arguments must be integers" << std::endl;
        return 1;
    }
} 