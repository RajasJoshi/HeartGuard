import socket
import time
import random
import numpy as np
import signal

HOST = "127.0.0.1"  # Standard loopback interface (localhost)
PORT = 5000  # Port to listen on

sample_rate = 100
ecg_data = np.loadtxt("assets/ECG_1000Hz_7.dat")


def signal_handler(sig, frame):
    print("Received termination signal. Shutting down server...")
    global running  # Indicate that the server should stop
    running = False


# Register signal handlers
signal.signal(signal.SIGINT, signal_handler)  # For Ctrl+C
signal.signal(signal.SIGTERM, signal_handler)  # For 'kill' command

running = True  # Flag to control the server loop

with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as s:
    s.bind((HOST, PORT))
    s.listen()
    print("Server listening...")
    conn, addr = s.accept()  # Accept a client connection
    with conn:
        print("Connected by", addr)
        while running:  # Check the running flag
            for value in ecg_data:
                data = f"{str(value)}\n".encode()
                conn.sendall(data)
                time.sleep(0.01)
