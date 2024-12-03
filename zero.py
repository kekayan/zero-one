import sys
import time
import json
import select
import random

# Create a poll object to monitor stdin
poll_obj = select.poll()
poll_obj.register(sys.stdin, select.POLLIN)

while True:
    try:
        # Check if there's data available to read (timeout = 0 means non-blocking)
        if poll_obj.poll(0):
            p_data = sys.stdin.readline()
            if not p_data:  # EOF
                break
                
            with open("p_data_received.json", "a") as f:
                f.write(p_data + "\n")
        time.sleep(10)
        q_data = {
            "timestamp": time.time(),
            "data": [random.randint(0, 100) for _ in range(3)]
        }
        with open("q_data_sent.json", "a") as f:
            f.write(json.dumps(q_data) + "\n")
        # Write to stdout
        print(q_data)
        sys.stdout.flush()
        
    except Exception as e:
        print(f"Error: {e}", file=sys.stderr)
        break
