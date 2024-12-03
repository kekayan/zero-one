#include <iostream>
#include <array>
#include <vector>
#include <thread>
#include <cstdio>
#include <unistd.h>
#include <signal.h>
#include <sys/wait.h>


class ProcessManager {
private:
    pid_t process_one = -1;
    pid_t process_zero = -1;
    // Separate pipes for each process to communicate with parent
    int one_to_parent[2];    // one.cpp -> coupler
    int parent_to_one[2];    // coupler -> one.cpp
    int zero_to_parent[2];   // zero.py -> coupler
    int parent_to_zero[2];   // coupler -> zero.py

    void cleanup() {
        if (process_one > 0) kill(process_one, SIGTERM);
        if (process_zero > 0) kill(process_zero, SIGTERM);
        // Close all pipe ends
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
        // Create all pipes
        if (pipe(one_to_parent) < 0 || pipe(parent_to_one) < 0 ||
            pipe(zero_to_parent) < 0 || pipe(parent_to_zero) < 0) {
            throw std::runtime_error("Failed to create pipes");
        }
    }

    void start_processes() {
        // Start one.cpp
        process_one = fork();
        if (process_one == 0) {
            // Child process for one.cpp
            dup2(parent_to_one[0], STDIN_FILENO);  // Read from parent
            dup2(one_to_parent[1], STDOUT_FILENO); // Write to parent
            
            // Close unused pipe ends
            close(one_to_parent[0]);
            close(parent_to_one[1]);
            close(zero_to_parent[0]);
            close(zero_to_parent[1]);
            close(parent_to_zero[0]);
            close(parent_to_zero[1]);
            
            execl("./one", "one", nullptr);
            exit(1);
        }

        // Start zero.py
        process_zero = fork();
        if (process_zero == 0) {
            // Child process for zero.py
            dup2(parent_to_zero[0], STDIN_FILENO);  // Read from parent
            dup2(zero_to_parent[1], STDOUT_FILENO); // Write to parent
            
            // Close unused pipe ends
            close(zero_to_parent[0]);
            close(parent_to_zero[1]);
            close(one_to_parent[0]);
            close(one_to_parent[1]);
            close(parent_to_one[0]);
            close(parent_to_one[1]);
            
            execl("/usr/bin/python3", "python3", "zero.py", nullptr);
            exit(1);
        }

        // Parent process handles communication between processes
        std::thread relay_thread([this]() {
            char buffer[1024];
            while (true) {
                // Read from one.cpp and write to zero.py
                ssize_t bytes_read = read(one_to_parent[0], buffer, sizeof(buffer));
                if (bytes_read <= 0) break;
                std::cout << "Read from one.cpp: " << std::string(buffer, bytes_read) << std::endl;
                write(parent_to_zero[1], buffer, bytes_read);

                // Read from zero.py and write to one.cpp
                bytes_read = read(zero_to_parent[0], buffer, sizeof(buffer));
                if (bytes_read <= 0) break;
                std::cout << "Read from zero.py: " << std::string(buffer, bytes_read) << std::endl;
                write(parent_to_one[1], buffer, bytes_read);
            }
        });

        // Wait for both processes
        int status;
        waitpid(process_one, &status, 0);
        waitpid(process_zero, &status, 0);
        
        relay_thread.join();
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