#include <iostream>
#include <string>
#include <vector>
#include <chrono>
#include <random>
#include <fstream>
#include <thread>
#include <poll.h>
#include <unistd.h>
#include <sstream>
#include <sys/stat.h>
#include <sys/types.h>
#include <errno.h>
#include <cstring>  

std::random_device rd;
std::mt19937 gen(rd());
std::uniform_int_distribution<> dis(0, 100);

std::vector<int> solve_step(int time_step, int some_data_initialised, int parent_data) {
    int random_data = dis(gen);
    std::vector<int> one_data = {time_step, random_data, some_data_initialised * random_data, parent_data};
    return one_data;
}

int main(int argc, char* argv[]) {
    bool is_coupled = false;
    if (argc > 1 && std::string(argv[1]) == "1") {
        is_coupled = true;
    }

    int time_step = 0;
    std::vector<int> prev_data;
    int some_data = 2;
    
    std::ofstream write_pipe;
    std::ifstream read_pipe;

    if (is_coupled) {
        // Open write pipe 
        std::cout << "Opening write pipe..." << std::endl;
        write_pipe.open("one_to_parent");
        if (!write_pipe.is_open()) {
            std::cerr << "Failed to open write pipe" << std::endl;
            return 1;
        }
        std::cout << "Write pipe opened successfully" << std::endl;

        // open read pipe
        std::cout << "Opening read pipe..." << std::endl;
        read_pipe.open("parent_to_one");
        if (!read_pipe.is_open()) {
            std::cerr << "Failed to open read pipe" << std::endl;
            return 1;
        }
        std::cout << "Read pipe opened successfully" << std::endl;
    }

    while (true) {
        try {
            std::vector<int> one_data; 
            
            if (!is_coupled) {
                int parent_data = 0;
                one_data = solve_step(time_step, some_data, parent_data);
            }
            else {
                std::cout << "Coupled" << std::endl;
                if (time_step == 0) {
                    int parent_data = 0;
                    one_data = solve_step(time_step, some_data, parent_data);
                } else {
                    std::vector<int> parent_data(4);
                    read_pipe.read(reinterpret_cast<char*>(parent_data.data()), 
                                 parent_data.size() * sizeof(int));
                    int parent_time_step = parent_data[0];
                    if (parent_time_step+1 != time_step) {
                        std::cerr << "Parent time step is not equal to time step" << std::endl;
                        std::this_thread::sleep_for(std::chrono::milliseconds(100));
                        continue;
                    }
                    int zero_data = parent_data[1];  
                    one_data = solve_step(time_step, some_data, zero_data); 
                }

                write_pipe.write(reinterpret_cast<const char*>(one_data.data()), 
                               one_data.size() * sizeof(int));
                write_pipe.flush();
            }
            std::cout << "One data: " << one_data[0] << " " << one_data[1] << " " << one_data[2] << " " << one_data[3] << std::endl;
            prev_data = one_data;
            time_step++;
            std::this_thread::sleep_for(std::chrono::milliseconds(100));

        } catch (const std::exception& e) {
            std::cerr << "Error: " << e.what() << std::endl;
            break;
        }
    }
    return 0;
} 