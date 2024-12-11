#include <iostream>
#include <array>
#include <vector>
#include <thread>
#include <cstdio>
#include <unistd.h>
#include <signal.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>


class ProcessManager {
private:
    pid_t process_one = -1;
    pid_t process_zero = -1;
    // Named pipe paths
    const char* one_to_parent = "one_to_parent";
    const char* parent_to_one = "parent_to_one";
    const char* zero_to_parent = "zero_to_parent";
    const char* parent_to_zero = "parent_to_zero";

    void cleanup() {
        if (process_one > 0) kill(process_one, SIGTERM);
        if (process_zero > 0) kill(process_zero, SIGTERM);
        // Remove named pipes
        unlink(one_to_parent);
        unlink(parent_to_one);
        unlink(zero_to_parent);
        unlink(parent_to_zero);
    }

public:
    ProcessManager() {
        // Create named pipes
        if (mkfifo(one_to_parent, 0666) < 0 && errno != EEXIST ||
            mkfifo(parent_to_one, 0666) < 0 && errno != EEXIST ||
            mkfifo(zero_to_parent, 0666) < 0 && errno != EEXIST ||
            mkfifo(parent_to_zero, 0666) < 0 && errno != EEXIST) {
            throw std::runtime_error("Failed to create named pipes");
        }
    }

    void start_processes() {
        // Start one.cpp
        process_one = fork();
        if (process_one == 0) {
            // Child process for one.cpp
            execl("./one", "one", "1", nullptr);
            exit(1);
        }

        // Start zero.py
        process_zero = fork();
        if (process_zero == 0) {
            // Child process for zero.py
            execl("/usr/bin/python3", "python3", "zero.py", "1", nullptr);
            exit(1);
        }

        // Parent process handles communication between processes
        std::thread relay_thread([this]() {
            char buffer[1024];
            // Open named pipes
            int one_read_fd = open(one_to_parent, O_RDONLY);
            int one_write_fd = open(parent_to_one, O_WRONLY);
            int zero_read_fd = open(zero_to_parent, O_RDONLY);
            int zero_write_fd = open(parent_to_zero, O_WRONLY);

            while (true) {
                // Read from one.cpp and write to zero.py
                ssize_t bytes_read = read(one_read_fd, buffer, sizeof(buffer));
                if (bytes_read <= 0) break;
                std::cout << "Relaying from one.cpp to zero.py: " << bytes_read << " bytes" << std::endl;
                write(zero_write_fd, buffer, bytes_read);

                // Read from zero.py and write to one.cpp
                bytes_read = read(zero_read_fd, buffer, sizeof(buffer));
                if (bytes_read <= 0) break;
                std::cout << "Relaying from zero.py to one.cpp: " << bytes_read << " bytes" << std::endl;
                write(one_write_fd, buffer, bytes_read);
            }

            // Close named pipes
            close(one_read_fd);
            close(one_write_fd);
            close(zero_read_fd);
            close(zero_write_fd);
        });

        // Wait for both processes
        int status;
        waitpid(process_one, &status, 0);
        waitpid(process_zero, &status, 0);
        
        relay_thread.join();
        cleanup();
    }

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