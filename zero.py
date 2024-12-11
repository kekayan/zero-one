import sys
import time
import random
import os
import numpy as np

def solve_step(time_step, some_data_initialised, parent_data):
    random_data = random.randint(0, 100)
    zero_data = np.array([time_step, random_data, some_data_initialised * random_data, parent_data], dtype=np.int32)
    return zero_data

def main():
    is_coupled = len(sys.argv) > 1 and sys.argv[1] == "1"
    time_step = 0
    prev_data = None
    some_data = 2

    write_pipe = None
    read_pipe = None

    if is_coupled:
        try:
            print("Opening write pipe...")
            # Create the pipe if it doesn't exist
            if not os.path.exists("zero_to_parent"):
                os.mkfifo("zero_to_parent")
            if not os.path.exists("parent_to_zero"):
                os.mkfifo("parent_to_zero")
                
            # Open pipes with explicit buffering
            write_pipe = open("zero_to_parent", "wb", buffering=0)
            print("Write pipe opened successfully")

            print("Opening read pipe...")
            read_pipe = open("parent_to_zero", "rb", buffering=0)
            print("Read pipe opened successfully")
        except Exception as e:
            print(f"Failed to open pipes: {e}", file=sys.stderr)
            if write_pipe:
                write_pipe.close()
            if read_pipe:
                read_pipe.close()
            return 1

    while True:
        try:
            if not is_coupled:
                parent_data = 0
                zero_data = solve_step(time_step, some_data, parent_data)
            else:
                print("Coupled")
                if time_step == 0:
                    parent_data = 0
                    zero_data = solve_step(time_step, some_data, parent_data)
                else:
                    # Define constants for data structure
                    INT_SIZE = 4  # size of int32 in bytes
                    DATA_LENGTH = 4  # number of integers in the array
                    
                    # Read data as numpy array
                    parent_data_bytes = read_pipe.read(INT_SIZE * DATA_LENGTH)
                    if len(parent_data_bytes) != INT_SIZE * DATA_LENGTH:
                        print(f"Error: Expected {INT_SIZE * DATA_LENGTH} bytes but got {len(parent_data_bytes)}", file=sys.stderr)
                        continue
                        
                    parent_data = np.frombuffer(parent_data_bytes, dtype=np.int32)
                    
                    parent_time_step = parent_data[0]
                    if parent_time_step+1 != time_step:
                        print("Parent time step is not equal to time step", file=sys.stderr)
                        time.sleep(0.1)
                        continue
                    
                    one_data = parent_data[1]
                    zero_data = solve_step(time_step, some_data, one_data)

                # Write the numpy array as binary
                write_pipe.write(zero_data.tobytes())
                write_pipe.flush()

            print(f"Zero data: {' '.join(map(str, zero_data))}")
            prev_data = zero_data
            time_step += 1
            time.sleep(0.1)

        except Exception as e:
            print(f"Error: {e}", file=sys.stderr)
            break

    if write_pipe:
        write_pipe.close()
    if read_pipe:
        read_pipe.close()

if __name__ == "__main__":
    main()
