#include <iostream>
#include <array>
#include <vector>
#include <string>
#include <cstring>
#include <fstream>
#include <sstream>
#include <thread>
#include <cstdio>
#include <cmath>
#include <iomanip>
#include <chrono>
#include <random>
#include <unistd.h>
#include <signal.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <poll.h>
#include <map>
#include <omp.h> //to use OpenMP API for parallel programming

// #include "model0d.h"
// #include "coupler_auxFun.h"


class ProcessManager {
private:
    pid_t process_one = -1;
    pid_t process_zero = -1;

    // Named pipe paths
    // const char* one_to_parent = "one_to_parent";
    // const char* parent_to_one = "parent_to_one";
    // const char* zero_to_parent = "zero_to_parent";
    // const char* parent_to_zero = "parent_to_zero";
    std::vector<const char*> one_to_parent;
    std::vector<const char*> parent_to_one;
    std::vector<const char*> zero_to_parent;
    std::vector<const char*> parent_to_zero;

    const std::string networkName = "aorticbif";

    // AuxCouplerFun* auxFun;
    int N1d0d;

    const double tolTime = 1e-10;
    const int N = 1024;

    // const char* ODEsolver = "explEul";
    // const char* ODEsolver = "Heun";
    // const char* ODEsolver = "midpoint";
    const char* ODEsolver = "RK4";

    double T0 = 1.1;
    const char* char_T0 = "1.1";
    int nCC = 1;
    const char* char_nCC = "1";

    void cleanup() {
        if (process_one > 0) kill(process_one, SIGTERM);
        if (process_zero > 0) kill(process_zero, SIGTERM);
        
        // Remove named pipes
        // unlink(one_to_parent);
        // unlink(parent_to_one);
        // unlink(zero_to_parent);
        // unlink(parent_to_zero);
        for (size_t i = 0; i < N1d0d; ++i) {
            unlink(one_to_parent[i]);
            unlink(parent_to_one[i]);
            unlink(zero_to_parent[i]);
            unlink(parent_to_zero[i]);
        }

        // Free each dynamically allocated string
        for (auto& pipe : one_to_parent) {
            free((void*)pipe);
            pipe = nullptr;
        }
        for (auto& pipe : parent_to_one) {
            free((void*)pipe);
            pipe = nullptr;
        }
        for (auto& pipe : zero_to_parent) {
            free((void*)pipe);
            pipe = nullptr;
        }
        for (auto& pipe : parent_to_zero) {
            free((void*)pipe);
            pipe = nullptr;
        }
    }

public:
    ProcessManager() {

        std::map<std::string, std::map<std::string, int>> pipe_1d_0d_info{
        { "1", { 
                { "vess1d_idx", 1}, 
                { "vess1d_bc_in0_or_out1", 1},
                { "cellml_idx", 5},  
                { "cellml_bc_in0_or_out1", 0},
                { "port_variable_idx", 17},
                { "port_flow0_or_press1", 0},
                { "R_T_variable_idx", 0}
               }},
        { "2", { 
                { "vess1d_idx", 2}, 
                { "vess1d_bc_in0_or_out1", 1},
                { "cellml_idx", 13}, 
                { "cellml_bc_in0_or_out1", 0},
                { "port_variable_idx", 21},
                { "port_flow0_or_press1", 0},
                { "R_T_variable_idx", 8}
               }}
        };
        
        // auxFun = new AuxCouplerFun();

        // std::string fileJson = networkName+"_coupler1d0d.json";
        // std::string dataJson = auxFun->readJsonFromFile(fileJson);
        // std::cout << "JSON read from file!\n";
        
        // std::map<std::string, std::map<std::string, int>> pipe_1d_0d_info = auxFun->deserializeFromJson(dataJson);
        // std::cout << "JSON string deserialized to map!\n";
        
        N1d0d = pipe_1d_0d_info.size();
        // std::cout << "COUPLER :: number of 1d-0d connections " << N1d0d << std::endl;
        // std::cout << "Loaded JSON data:\n";
        // for (const auto& [category, values] : pipe_1d_0d_info) {
        //     std::cout << category << ":\n";
        //     for (const auto& [key, val] : values) {
        //         std::cout << "  " << key << ": " << val << "\n";
        //     }
        // }

        for (size_t i = 0; i < N1d0d; ++i) {
            int pipeID = i;
            std::string pipePath;
            
            pipePath = "one_to_parent_"+std::to_string(pipeID);
            one_to_parent.push_back(strdup(pipePath.c_str()));

            pipePath = "parent_to_one_"+std::to_string(pipeID);
            parent_to_one.push_back(strdup(pipePath.c_str()));

            pipePath = "zero_to_parent_"+std::to_string(pipeID);
            zero_to_parent.push_back(strdup(pipePath.c_str()));

            pipePath = "parent_to_zero_"+std::to_string(pipeID);
            parent_to_zero.push_back(strdup(pipePath.c_str()));            
        }

        for (size_t i = 0; i < N1d0d; ++i) {
            unlink(one_to_parent[i]);
            unlink(parent_to_one[i]);
            unlink(zero_to_parent[i]);
            unlink(parent_to_zero[i]);
        }

        // Create Named Pipes (FIFOs), instead of "standard" pipes
        // if (mkfifo(one_to_parent, 0666) < 0 && errno != EEXIST ||
        //     mkfifo(parent_to_one, 0666) < 0 && errno != EEXIST ||
        //     mkfifo(zero_to_parent, 0666) < 0 && errno != EEXIST ||
        //     mkfifo(parent_to_zero, 0666) < 0 && errno != EEXIST) {
        //     throw std::runtime_error("Failed to create named pipes");
        // }
        for (size_t i = 0; i < N1d0d; ++i) {
            if (mkfifo(one_to_parent[i], 0666) < 0 && errno != EEXIST ||
                mkfifo(parent_to_one[i], 0666) < 0 && errno != EEXIST ||
                mkfifo(zero_to_parent[i], 0666) < 0 && errno != EEXIST ||
                mkfifo(parent_to_zero[i], 0666) < 0 && errno != EEXIST) {
                throw std::runtime_error("Failed to create named pipes");
            }
            std::cout << "Coupler: Named pipe " << one_to_parent[i] << " created successfully" << std::endl;
            std::cout << "Coupler: Named pipe " << parent_to_one[i] << " created successfully" << std::endl;
            std::cout << "Coupler: Named pipe " << zero_to_parent[i] << " created successfully" << std::endl;
            std::cout << "Coupler: Named pipe " << parent_to_zero[i] << " created successfully" << std::endl;
        }

    }


    void start_processes() {
        // Start 1D solver main1D.py
        process_one = fork();
        if (process_one == 0) { // Child process for main1D.py
            //execl("/usr/bin/python3", "python3", "./solver1D/main1D.py", "1", nullptr);
            //execl("/usr/bin/python", "python", "./solver1D/main1D.py", "1", nullptr);
            //execl("/home/bghi639/anaconda3/bin/python", "python", "./solver1D/main1D.py", "1", nullptr);
            //execl("/hpc/bghi639/anaconda3/bin/python", "python", "./solver1D/main1D.py", "1", nullptr);
            //execl("/hpc/bghi639/anaconda3/bin/python", "python", "main1D.py", "1", nullptr);
            
            // execl("/hpc/bghi639/anaconda3/bin/python", "python", "./solver1D/main1D.py", "1", ODEsolver, char_T0, char_nCC, nullptr);
            execl("./one", "one", "1", nullptr);

            exit(1);
        }
        
        // Start 0D solver main0D.cpp
        process_zero = fork();
        if (process_zero == 0) { // Child process for main0D.cpp
            //execl("./solver0D/main0D", "main0D", "1", nullptr);
            //execl("./main0D", "main0D", "1", nullptr);

            // execl("./solver0D/main0D", "main0D", "1", ODEsolver, char_T0, char_nCC, nullptr);
            execl("/usr/bin/python3", "python3", "zero.py", "1", nullptr);

            exit(1);
        }

        
        // Parent process handles communication between processes
        // The three processes (zero, one & coupler) are running "in parallel" 
        std::thread relay_thread([this]() {

            double dt = 0.;
            double timeGlob = 0.;
            double tEndGlob = nCC*T0;
            // double dtSample = 1e-3;
            // double timeSample = dtSample;
            
            // char one_buffer[N];
            // char zero_buffer[N];
            char **one_buffer;
            char **zero_buffer;
            one_buffer = new char*[N1d0d];
            zero_buffer = new char*[N1d0d];
            // #pragma omp parallel for
            for (size_t i = 0; i < N1d0d; ++i) {
                one_buffer[i] = new char[N];
                zero_buffer[i] = new char[N];
            }
            
            int count = 0;
            
            // Open named pipes
            // int one_read_fd = open(one_to_parent, O_RDONLY);
            // int one_write_fd = open(parent_to_one, O_WRONLY);
            // int zero_read_fd = open(zero_to_parent, O_RDONLY);
            // int zero_write_fd = open(parent_to_zero, O_WRONLY);
            int one_read_fd[N1d0d]; 
            int one_write_fd[N1d0d];
            int zero_read_fd[N1d0d];
            int zero_write_fd[N1d0d];
            // #pragma omp parallel for
            for (size_t i = 0; i < N1d0d; ++i) {
                one_read_fd[i] = open(one_to_parent[i], O_RDONLY);
                one_write_fd[i] = open(parent_to_one[i], O_WRONLY);
                zero_read_fd[i] = open(zero_to_parent[i], O_RDONLY);
                zero_write_fd[i] = open(parent_to_zero[i], O_WRONLY);
            }

            // std::cout << "DBG 000 \n";
            
            while (true) {
                // #pragma omp parallel for
                bool all_pipes_closed = true;
                for (size_t i = 0; i < N1d0d; ++i) {
                    // std::cout << "Thread " << omp_get_thread_num() << " is processing index " << i << std::endl; // Test!!!
                    // Read from main0D.cpp
                    ssize_t N_zero = read(zero_read_fd[i], zero_buffer[i], N);
                    if (N_zero <= 0) {
                        std::cerr << "Pipe " << i << " :: Zero read failed or returned 0 bytes" << std::endl;
                        continue;  // Skip this pipe but continue with others
                    }
                    all_pipes_closed = false;
                    std::cout << "Pipe " << i << " :: Relaying from main0D.cpp to main1D.py : " << N_zero << " bytes" << std::endl;
                    
                    // Write to main1D.cpp
                    write(one_write_fd[i], zero_buffer[i], N_zero);
                }
                
                for (size_t i = 0; i < N1d0d; ++i) {
                    // Read from main1D.cpp
                    ssize_t N_one = read(one_read_fd[i], one_buffer[i], N);
                    if (N_one <= 0) {
                        std::cerr << "Pipe " << i << " :: One read failed or returned 0 bytes" << std::endl;
                        continue;  // Skip this pipe but continue with others
                    }
                    all_pipes_closed = false;
                    std::cout << "Pipe " << i << " :: Relaying from main1D.py to main0D.cpp : " << N_one << " bytes" << std::endl;
                    
                    // Write to main0D.cpp
                    write(zero_write_fd[i], one_buffer[i], N_one);
                }

                if (all_pipes_closed) {
                    std::cout << "All pipes closed, relay thread exiting..." << std::endl;
                    break;
                }
                
                count++;

                // if (ODEsolver=="explEul"){
                //     std::cout << "Coupling :: timeGlob : " << std::setprecision(8) << timeGlob << " || dtGlob : " << dt << std::endl;
                //     timeGlob +=dt;
                //     timeGlob = std::round(timeGlob / tolTime) * tolTime;
                //     // if (abs(timeGlob-timeSample)<=tolTime) {
                //     //     timeSample +=dtSample;
                //     // }
                // } else if (ODEsolver=="Heun" || ODEsolver=="midpoint"){
                //     if (count % 2 == 0){
                //         std::cout << "Coupling :: timeGlob : " << std::setprecision(8) << timeGlob << " || dtGlob : " << dt << std::endl;
                //         timeGlob +=dt;
                //         timeGlob = std::round(timeGlob / tolTime) * tolTime;
                //         // if (abs(timeGlob-timeSample)<=tolTime) {
                //         //     timeSample +=dtSample;
                //         // }
                //     }
                // } else if (ODEsolver=="RK4"){
                //     if (count % 4 == 0){
                //         std::cout << "Coupling :: timeGlob : " << std::setprecision(8) << timeGlob << " || dtGlob : " << dt << std::endl;
                //         timeGlob +=dt;
                //         timeGlob = std::round(timeGlob / tolTime) * tolTime;
                //         // if (abs(timeGlob-timeSample)<=tolTime) {
                //         //     timeSample +=dtSample;
                //         // }
                //     }
                // }

                // if (abs(timeGlob-tEndGlob)<=tolTime) { // HERE !!!!!!!!!!!!!!!!!! BUT WHY?!?!?!?!?!?!?!?
                //     run_coupler = false;
                // }
                std::cout<< "hello world" << std::endl;
            }

            std::cout << "### Coupler :: Stop execution! ###" << std::endl;
            std::cout << "Global final time : " << timeGlob << std::endl;
            std::cout << "Number of 1D-0D communications : " << count << std::endl;

            // Close named pipes
            for (size_t i = 0; i < N1d0d; ++i) {
                close(one_read_fd[i]);
                close(one_write_fd[i]);
                close(zero_read_fd[i]);
                close(zero_write_fd[i]);
            }

            // #pragma omp parallel for
            for (size_t i = 0; i < N1d0d; ++i) {
                delete[] one_buffer[i];
                delete[] zero_buffer[i];
            }
            delete[] one_buffer;
            delete[] zero_buffer;

        });

        std::cout << "Waiting for child processes to terminate..." << std::endl;
        int one_status, zero_status;
        waitpid(process_one, &one_status, 0);
        std::cout << "Process one terminated with status " << one_status << std::endl;
        waitpid(process_zero, &zero_status, 0);
        std::cout << "Process zero terminated with status " << zero_status << std::endl;


        std::cout << "Waiting for relay thread to join..." << std::endl;
        relay_thread.join();
        
        std::cout << "Both processes terminated successfully with status codes: " << one_status << " " << zero_status << std::endl;
        
        cleanup();
       
    }

    ~ProcessManager() {
        cleanup();
    }
};

int main() {
    // omp_set_num_threads(4);

    try {
        ProcessManager manager;
        manager.start_processes();
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }
    return 0;
}