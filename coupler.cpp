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
    int pipe_one_to_zero[2];  // one.cpp -> zero.py
    int pipe_zero_to_one[2];  // zero.py -> one.cpp

    void cleanup() {
        if (process_one > 0) kill(process_one, SIGTERM);
        if (process_zero > 0) kill(process_zero, SIGTERM);
        close(pipe_one_to_zero[0]);
        close(pipe_one_to_zero[1]);
        close(pipe_zero_to_one[0]);
        close(pipe_zero_to_one[1]);
    }
// constructor: sets up pipes
public:
    ProcessManager() {
        if (pipe(pipe_one_to_zero) < 0 || pipe(pipe_zero_to_one) < 0) {
            throw std::runtime_error("Failed to create pipes");
        }
    }

    ~ProcessManager() {
        cleanup();
    }

    void start_processes() {
        // Start one.cpp
        process_one = fork();
        if (process_one == 0) {
            // Child process for one.cpp
            dup2(pipe_one_to_zero[1], STDOUT_FILENO); 
            dup2(pipe_zero_to_one[0], STDIN_FILENO);
            
            // Close unused pipe ends
            close(pipe_one_to_zero[0]); // read end
            close(pipe_zero_to_one[1]); // write end
            
            execl("./one", "one", nullptr);
            exit(1);
        }

        // Start zero.py
        process_zero = fork();
        if (process_zero == 0) {
            // Child process for zero.py
            dup2(pipe_one_to_zero[0], STDIN_FILENO);
            dup2(pipe_zero_to_one[1], STDOUT_FILENO);
            
            // Close unused pipe ends
            close(pipe_one_to_zero[1]); // write end
            close(pipe_zero_to_one[0]); // read end
            
            execl("/usr/bin/python3", "python3", "zero.py", nullptr);
            exit(1);
        }

        // Close unused pipe ends in parent
        close(pipe_one_to_zero[0]);
        close(pipe_one_to_zero[1]);
        close(pipe_zero_to_one[0]);
        close(pipe_zero_to_one[1]);

        // Wait for both processes
        int status;
        waitpid(process_one, &status, 0);
        waitpid(process_zero, &status, 0);
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