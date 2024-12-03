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

int main() {
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(0, 100);
    
    int time_step = 0;
    std::vector<int> prev_data;
    
    struct pollfd fds[1];
    fds[0].fd = STDIN_FILENO;
    fds[0].events = POLLIN;
    
    while (true) {
        try {
            if (time_step > 0) {
                // Wait for input from zero.py for every time step except 0
                bool received_data = false;
                while (!received_data) {
                    if (poll(fds, 1, 100) > 0) {  // Poll with 100ms timeout
                        std::string line;
                        if (std::getline(std::cin, line)) {
                            size_t pos = line.find("[");
                            size_t end_pos = line.find("]");
                            if (pos != std::string::npos && end_pos != std::string::npos) {
                                std::string data_str = line.substr(pos + 1, end_pos - pos - 1);
                                std::stringstream ss(data_str);
                                prev_data.clear();
                                int val;
                                while (ss >> val) {
                                    prev_data.push_back(val);
                                    if (ss.peek() == ',') ss.ignore();
                                }
                                received_data = true;
                            }
                        }
                    }
                }
            }
            
            // Generate new data
            std::string p_data = std::to_string(time_step);
            for (int i = 0; i < 3; i++) {
                int new_val;
                if (time_step == 0) {
                    new_val = dis(gen);  // Random initial values
                } else {
                    new_val = (prev_data[i] + dis(gen)) % 100;
                }
                p_data += " " + std::to_string(new_val);
            }
            
            std::cout << p_data << std::endl;
            std::cout.flush();
            
            time_step++;
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
            
        } catch (const std::exception& e) {
            std::cerr << "Error: " << e.what() << std::endl;
            break;
        }
    }
    return 0;
} 