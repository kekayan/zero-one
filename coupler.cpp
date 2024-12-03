#include <iostream>
#include <array>
#include <vector>
#include <thread>
#include <cstdio>
#include <unistd.h>
#include <signal.h>
#include <sys/wait.h>
#include <chrono>


class ProcessManager {
private:
    pid_t process_one = -1;
    pid_t process_zero = -1;
    // separate pipes for each process to communicate with parent
    int one_to_parent[2];    // one.cpp -> coupler
    int parent_to_one[2];    // coupler -> one.cpp
    int zero_to_parent[2];   // zero.py -> coupler
    int parent_to_zero[2];   // coupler -> zero.py
    int current_time_step = 0;
    int prev_one_value = 0;
    int prev_zero_value = 0;

    void cleanup() {
        if (process_one > 0) kill(process_one, SIGTERM);
        if (process_zero > 0) kill(process_zero, SIGTERM);
        // close all pipe ends
        close(one_to_parent[0]);
        close(one_to_parent[1]);
        close(parent_to_one[0]);
        close(parent_to_one[1]);
        close(zero_to_parent[0]);
        close(zero_to_parent[1]);
        close(parent_to_zero[0]);
        close(parent_to_zero[1]);
    }

public:
    ProcessManager() {
        // create all pipes
        if (pipe(one_to_parent) < 0 || pipe(parent_to_one) < 0 ||
            pipe(zero_to_parent) < 0 || pipe(parent_to_zero) < 0) {
            throw std::runtime_error("Failed to create pipes");
        }
    }

    void start_processes() {
        auto parse_output = [](const std::string& output) -> int {
            size_t comma = output.find(',');
            if (comma != std::string::npos) {
                size_t second_comma = output.find(',', comma + 1);
                if (second_comma != std::string::npos) {
                    std::string value = output.substr(comma + 1, second_comma - comma - 1);
                    return std::stoi(value);
                }
            }
            return 0;
        };

        while (true) {  // main loop for spawning processes
            // start one.cpp
            process_one = fork();
            if (process_one == 0) {
                // child process for one.cpp
                dup2(parent_to_one[0], STDIN_FILENO);
                dup2(one_to_parent[1], STDOUT_FILENO);
                
                // close unused pipe ends
                close(one_to_parent[0]);
                close(parent_to_one[1]);
                close(zero_to_parent[0]);
                close(zero_to_parent[1]);
                close(parent_to_zero[0]);
                close(parent_to_zero[1]);
                
                char time_step_str[16], val1_str[16], val2_str[16];
                snprintf(time_step_str, sizeof(time_step_str), "%d", current_time_step);
                snprintf(val1_str, sizeof(val1_str), "%d", prev_zero_value);
                snprintf(val2_str, sizeof(val2_str), "%d", 0);
                
                execl("./one", "one", time_step_str, val1_str, val2_str, nullptr);
                exit(1);
            }

            // Start zero.py
            process_zero = fork();
            if (process_zero == 0) {
                // Child process for zero.py
                dup2(parent_to_zero[0], STDIN_FILENO);
                dup2(zero_to_parent[1], STDOUT_FILENO);
                
                // Close unused pipe ends
                close(zero_to_parent[0]);
                close(parent_to_zero[1]);
                close(one_to_parent[0]);
                close(one_to_parent[1]);
                close(parent_to_one[0]);
                close(parent_to_one[1]);
                
                char time_step_str[16], val1_str[16], val2_str[16];
                snprintf(time_step_str, sizeof(time_step_str), "%d", current_time_step);
                snprintf(val1_str, sizeof(val1_str), "%d", prev_one_value);
                snprintf(val2_str, sizeof(val2_str), "%d", 0);
                
                execl("/usr/bin/python3", "python3", "zero.py", time_step_str, val1_str, val2_str, nullptr);
                exit(1);
            }

            // parent process: read outputs and update state
            char buffer[1024];
            
            // read from one.cpp
            ssize_t bytes_read = read(one_to_parent[0], buffer, sizeof(buffer));
            if (bytes_read > 0) {
                std::string one_output(buffer, bytes_read);
                std::cout << "Read from one.cpp: " << one_output << std::endl;
                prev_one_value = parse_output(one_output);
            }

            // read from zero.py
            bytes_read = read(zero_to_parent[0], buffer, sizeof(buffer));
            if (bytes_read > 0) {
                std::string zero_output(buffer, bytes_read);
                std::cout << "Read from zero.py: " << zero_output << std::endl;
                prev_zero_value = parse_output(zero_output);
            }

            // wait for both processes to complete
            waitpid(process_one, nullptr, 0);
            waitpid(process_zero, nullptr, 0);

            // reset process IDs
            process_one = -1;
            process_zero = -1;

            // increment time step
            current_time_step++;

            // small delay between iterations to prevent CPU overload
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
    }

    // destructor
    ~ProcessManager() {
        cleanup();
    }
};

int main() {
    try {
        ProcessManager manager;
        manager.start_processes();
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }
    return 0;
} 