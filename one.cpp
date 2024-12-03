#include <iostream>
#include <string>
#include <vector>
#include <chrono>
#include <random>
#include <fstream>
#include <thread>
#include <poll.h>
#include <unistd.h>

int main() {
        
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(0, 100);
    
    // Set up poll for stdin
    struct pollfd fds[1];
    fds[0].fd = STDIN_FILENO;
    fds[0].events = POLLIN;
    
    while (true) {
        try {
            // Check if there's input available
            if (poll(fds, 1, 0) > 0) {
                std::string line;
                if (std::getline(std::cin, line)) {
                    std::cout << "Received: " << line << std::endl;
                    // Log received q_data
                    std::ofstream q_log("q_data_received.txt", std::ios_base::app);
                    q_log << line << std::endl;
                    q_log.close();
                }
            }
            
            // Generate p_data
            auto now = std::chrono::system_clock::now().time_since_epoch().count() / 1000000000.0;
            std::string p_data = std::to_string(now) + " " + 
                                std::to_string(dis(gen)) + " " + 
                                std::to_string(dis(gen)) + " " + 
                                std::to_string(dis(gen));
            
            // Log sent p_data
            std::ofstream p_log("p_data_sent.txt", std::ios_base::app);
            p_log << p_data << std::endl;
            p_log.close();
            
            // Output p_data to stdout
            std::cout << p_data << std::endl;
            std::cout.flush();
            
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
            
        } catch (const std::exception& e) {
            std::cerr << "Error: " << e.what() << std::endl;
            break;
        }
    }
    
    return 0;
} 