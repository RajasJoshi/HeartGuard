import socket
import time
import random
import numpy as np

HOST = "127.0.0.1"  # Standard loopback interface (localhost)
PORT = 5000  # Port to listen on

sample_rate = 100
ecg_data = np.loadtxt("assets/ECG_1000Hz_7.dat")


with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as s:
    s.bind((HOST, PORT))
    s.listen()
    print("Server listening...")
    conn, addr = s.accept()  # Accept a client connection
    with conn:
        print("Connected by", addr)
        while True:
            for value in ecg_data:
                heart_rate = 75 + random.randint(-5, 5)
                data = f"{str(value)},{heart_rate}\n".encode()                 
                conn.sendall(data)
                time.sleep(0.01)
