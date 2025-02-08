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
#include <algorithm>

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

    std::vector<int> time_steps;
    std::vector<std::vector<int>> prev_data;
    int some_data = 2;
    
    std::vector<std::ofstream> write_pipes;
    std::vector<std::ifstream> read_pipes;

    int num_pipes = 2;

    time_steps.resize(num_pipes, 0);

    if (is_coupled) {
        
        // Open write pipes
        
        for (int i = 0; i < num_pipes; i++) {
            write_pipes.emplace_back(std::ofstream());
            std::string write_pipe_name = "one_to_parent_" + std::to_string(i);
            write_pipes[i].open(write_pipe_name);
            if (!write_pipes[i].is_open()) {
                std::cerr << "One: Failed to open write pipe " << write_pipe_name << std::endl;
                return 1;
            }
            std::cout << "One: Write pipe " << write_pipe_name << " opened successfully" << std::endl;

            // Open read pipes
            
            read_pipes.emplace_back(std::ifstream());
            std::string read_pipe_name = "parent_to_one_" + std::to_string(i);
            read_pipes[i].open(read_pipe_name);
            if (!read_pipes[i].is_open()) {
                std::cerr << "One: Failed to open read pipe " << read_pipe_name << std::endl;
                return 1;
            }
            std::cout << "One: Read pipe " << read_pipe_name << " opened successfully" << std::endl;
        }

    }

    while (true) {
        if (std::all_of(time_steps.begin(), time_steps.end(), [](int t) { return t == 10; })){
            std::cout << "One: Breaking..." << std::endl;
            break;
        }
        try {
            std::vector<std::vector<int>> one_data_array;
            
            if (!is_coupled) {
                int parent_data = 0;
                one_data_array.push_back(solve_step(time_steps[0], some_data, parent_data));
            }
            else {
                
                if (std::all_of(time_steps.begin(), time_steps.end(), [](int t) { return t == 0; })) {
                    std::cout << "One: Coupled" << std::endl;
                    int parent_data = 0;
                    for (size_t i = 0; i < num_pipes; i++) {
                        one_data_array.push_back(solve_step(time_steps[i], some_data, parent_data));
                        time_steps[i]++;    
                    }
                } else {
                    // Read from all read pipes
                    std::vector<std::vector<int>> parent_data_array;
                    bool retry_timestep = false;
                    
                    for (size_t i = 0; i < read_pipes.size(); i++) {
                        std::vector<int> parent_data(4);
                        read_pipes[i].read(reinterpret_cast<char*>(parent_data.data()), 
                                     parent_data.size() * sizeof(int));
                        parent_data_array.push_back(parent_data);
                        
                        int parent_time_step = parent_data[0];
                        if (parent_time_step + 1 != time_steps[i]) {
                            std::cerr << "One: Parent time step " << parent_time_step 
                                    << " is not equal to expected time step " << time_steps[i] 
                                    << " for pipe " << i << std::endl;
                            retry_timestep = true;
                        }
                    }
                    
                    if (retry_timestep) {
                        std::this_thread::sleep_for(std::chrono::milliseconds(100));
                        continue;   
                    }
                    
                    // Process data from each pipe
                    for (size_t i = 0; i < parent_data_array.size(); i++) {
                        int zero_data = parent_data_array[i][1];
                        one_data_array.push_back(solve_step(time_steps[i], some_data, zero_data));
                        time_steps[i]++;    
                    }
                }
            }

            // Print data from all pipes
            for (size_t i = 0; i < one_data_array.size(); i++) {
                std::cout << "One: data " << i << ": " 
                         << one_data_array[i][0] << " " 
                         << one_data_array[i][1] << " " 
                         << one_data_array[i][2] << " " 
                         << one_data_array[i][3] << std::endl;
            }

            prev_data = one_data_array;     

            if (is_coupled) {
                // Write to all write pipes
                for (size_t i = 0; i < write_pipes.size(); i++) {
                    if (i >= one_data_array.size()) {
                        std::cerr << "One: Missing data for pipe " << i << std::endl;
                        continue;
                    }
                    try {
                        const char* data = reinterpret_cast<const char*>(one_data_array[i].data());
                        std::streamsize size = one_data_array[i].size() * sizeof(int);
                        write_pipes[i].write(data, size);
                        write_pipes[i].flush();
                        
                        if (!write_pipes[i].good()) {
                            throw std::runtime_error("Write operation failed");
                        }
                        std::cout << "One: Successfully wrote " << size << " bytes to pipe " << i << std::endl;
                    }
                    catch (const std::exception& e) {
                        std::cerr << "One: Failed to write to pipe " << i << ": " << e.what() << std::endl;
                        std::this_thread::sleep_for(std::chrono::milliseconds(100));
                        continue;
                    }
                }
            }

            std::this_thread::sleep_for(std::chrono::milliseconds(100));

        } catch (const std::exception& e) {
            std::cerr << "One: " << e.what() << std::endl;
            break;
        }
    }
    std::cout << "One: Exiting..." << std::endl;
    return 0;
} 