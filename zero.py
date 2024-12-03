import sys
import time
import json
import select
import random

poll_obj = select.poll()
poll_obj.register(sys.stdin, select.POLLIN)

time_step = 0
prev_data = None

while True:
    try:
        if time_step > 0:
            # Wait for input from one.cpp for every time step except 0
            received_data = False
            while not received_data:
                if poll_obj.poll(100):  # Poll with 100ms timeout
                    p_data = sys.stdin.readline().strip()
                    if not p_data:
                        break
                    
                    values = [int(x) for x in p_data.split()]
                    if len(values) >= 4:  # time_step + 3 values
                        prev_data = values[1:]
                        received_data = True
        
        # Generate new data
        if time_step == 0:
            new_data = [random.randint(0, 100) for _ in range(3)]  # Random initial values
        else:
            new_data = [(x + random.randint(0, 100)) % 100 for x in prev_data]
            
        q_data = {
            "time_step": time_step,
            "data": new_data
        }
        
        print(json.dumps(q_data))
        sys.stdout.flush()
        
        time_step += 1
        time.sleep(0.1)
        
    except Exception as e:
        print(f"Error: {e}", file=sys.stderr)
        break
