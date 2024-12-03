import random
import sys

def main(one_data):
    time_step = one_data[0]
    # p_data = [time_step, p_value, q_value from one.cpp]
    if time_step == 0:
        p_data = [0,random.randint(0, 100), 0 ]
    else:
        p_data = [time_step, random.randint(0, 100), one_data[1]]
    with open("zero.txt", "a") as f:
        f.write(f"{time_step} {p_data[1]} {p_data[2]}\n")
    return p_data

if __name__ == "__main__":

    if len(sys.argv) != 4:
        print("Usage: python zero.py <time_step> <value1> <value2>")
        sys.exit(1)
    
    try:
        # Convert command line arguments to integers
        input_data = [int(arg) for arg in sys.argv[1:]]
        result = main(input_data)
        print(result)
    except ValueError:
        print("Error: All arguments must be integers")
        sys.exit(1)


