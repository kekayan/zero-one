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
    time_steps = [0] * 2  
    prev_data = []  
    some_data = 2
    num_pipes = 2 

    write_pipes = []
    read_pipes = []

    if is_coupled:
        try:
           
            # Create and open write pipes
            for i in range(num_pipes):
                pipe_name = f"zero_to_parent_{i}"
                if not os.path.exists(pipe_name):
                    os.mkfifo(pipe_name)
                write_pipe = open(pipe_name, "wb", buffering=0)
                write_pipes.append(write_pipe)
                print(f"Write pipe {pipe_name} opened successfully")

                
                # Create and open read pipes
                pipe_name = f"parent_to_zero_{i}"
                if not os.path.exists(pipe_name):
                    os.mkfifo(pipe_name)
                read_pipe = open(pipe_name, "rb", buffering=0)
                read_pipes.append(read_pipe)
                print(f"Read pipe {pipe_name} opened successfully")

        except Exception as e:
            print(f"Zero: Failed to open pipes: {e}", file=sys.stderr)
            for pipe in write_pipes + read_pipes:
                pipe.close()
            return 1

    while True:
        if all(t >= 10 for t in time_steps):
            print("Zero: breaking")
            break
        try:
            zero_data_array = [] 

            if not is_coupled:
                parent_data = 0
                zero_data_array.append(solve_step(time_steps[0], some_data, parent_data))
            else:
               
                if all(t == 0 for t in time_steps):
                    print("Zero: Coupled, time_step 0")
                    # Initialize data for all pipes at time_step 0
                    parent_data = 0
                    for i in range(num_pipes):
                        zero_data_array.append(solve_step(time_steps[i], some_data, parent_data))
                        time_steps[i] += 1  
                else:
                    INT_SIZE = 4  
                    DATA_LENGTH = 4  
                    
                    # Read from all pipes
                    parent_data_array = []
                    retry_timestep = False
                    
                    for i, read_pipe in enumerate(read_pipes):
                        parent_data_bytes = read_pipe.read(INT_SIZE * DATA_LENGTH)
                        if len(parent_data_bytes) != INT_SIZE * DATA_LENGTH:
                            print(f"Zero: Expected {INT_SIZE * DATA_LENGTH} bytes but got {len(parent_data_bytes)}", 
                                  file=sys.stderr)
                            retry_timestep = True
                            break
                        
                        parent_data = np.frombuffer(parent_data_bytes, dtype=np.int32)
                        parent_data_array.append(parent_data)
                        
                        parent_time_step = parent_data[0]
                        if parent_time_step + 1 != time_steps[i]:
                            print(f"Zero: Parent time step {parent_time_step} is not equal to expected time step {time_steps[i]} for pipe {i}", file=sys.stderr)
                            retry_timestep = True
                    
                    if retry_timestep:
                        time.sleep(0.1)
                        continue  # Retry the entire time step
                    
                    # Process data from each pipe
                    for i, parent_data in enumerate(parent_data_array):
                        one_data = parent_data[1]
                        zero_data = solve_step(time_steps[i], some_data, one_data)
                        zero_data_array.append(zero_data)
                        time_steps[i] += 1  

                # Write to all pipes
                if len(zero_data_array) != len(write_pipes):
                    print(f"Zero: Warning - Data mismatch. Have {len(zero_data_array)} data items for {len(write_pipes)} pipes", file=sys.stderr)
                
                retry_timestep = False
                for i, write_pipe in enumerate(write_pipes):
                    if i < len(zero_data_array):
                        try:
                            # Ensure data is contiguous and in correct byte order
                            data = zero_data_array[i].astype(np.int32)
                            if not data.flags['C_CONTIGUOUS']:
                                data = np.ascontiguousarray(data)
                            data_bytes = data.tobytes('C')  
                            bytes_written = write_pipe.write(data_bytes)
                            write_pipe.flush()
                            print(f"Zero: Successfully wrote {bytes_written} bytes to pipe {i}")
                            print(f"Zero: data {i}: {' '.join(map(str, zero_data_array[i]))}")
                        except IOError as e:
                            print(f"Zero: Failed to write to pipe {i}: {e}", file=sys.stderr)
                            retry_timestep = True
                            break
                    else:
                        print(f"Zero: Skipping write to pipe {i} due to missing data", file=sys.stderr)
                
                if retry_timestep:
                    time.sleep(0.1)
                    continue

            prev_data = zero_data_array
            time.sleep(0.1)

        except Exception as e:
            print(f"Zero: {e}", file=sys.stderr)
            continue

    # Cleanup
    for pipe in write_pipes + read_pipes:
        pipe.close()

if __name__ == "__main__":
    main()
    print("Zero: Exiting main")
