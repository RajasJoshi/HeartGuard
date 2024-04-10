import streamlit as st
import numpy as np
import pandas as pd
import socket
import threading
import queue
import time
import plotly.express as px

HOST = "127.0.0.1"
PORT = 5000

data_queue = queue.Queue()
ecg_data = []  # ECG data
heart_rate_data = 100 + np.random.randn(100)  # Random data for HR
heart_rate = 100
resp_rate = 28
core_temp = 40.3
spo2 = 92
spo2_data = 90 + np.random.randn(100)  # Random data for SpO2
hrv = 0.2
index_counter = 0


# ------ STREAMLIT APP ------
st.set_page_config(page_title="HeartGuard Lite", layout="wide")

# Header
st.title("HeartGuard Lite")

# Information Row
col1, col2, col3, col4, col5 = st.columns(5)
metrics = {
    "Heart Rate": "N/A",
    "Resp Rate": "N/A",
    "Core Temp": "N/A",
    "SpO2": "N/A",
    "HRV": "N/A",
}

# Graph placeholders
ecg_placeholder = st.empty()
hr_placeholder = st.empty()
spo2_placeholder = st.empty()


# Client connection function
def connect_to_server():
    with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as s:
        s.connect((HOST, PORT))  # Connect to the server
        while True:
            data = s.recv(1024)
            if not data:
                break
            data_queue.put(data.decode("utf-8"))


# Start the client connection thread
client_thread = threading.Thread(target=connect_to_server)
client_thread.start()


@st.cache_data  # Use the appropriate caching command
def update_metrics():
    global heart_rate, resp_rate, core_temp, spo2, hrv

    metrics_col1, metrics_col2 = st.columns(2)

    with metrics_col1:
        st.metric("Heart Rate", f"{heart_rate} bpm")
        st.metric("Resp Rate", f"{resp_rate} bpm")

    with metrics_col2:
        st.metric("Core Temp", f"{core_temp} °C")
        st.metric("SpO2", f"{spo2} %")
        st.metric("HRV", hrv)


def update_ecg():
    global ecg_data, index_counter
    try:
        latest_data = data_queue.get_nowait()
        ecg_data.append(latest_data)

        ecg_fig = px.line(
            pd.DataFrame(
                {"ECG Signal": ecg_data},
                index=range(index_counter, index_counter + len(ecg_data)),
            )
        )
        ecg_placeholder.plotly_chart(ecg_fig)

        index_counter += len(ecg_data)
    except queue.Empty:
        pass


def update_heart_rate():

    hr_fig = px.line(pd.DataFrame({"Heart Rate": heart_rate_data}))
    hr_placeholder.plotly_chart(hr_fig)


def update_spo2():
    spo2_fig = px.line(pd.DataFrame({"Spo2": spo2_data}))
    spo2_placeholder.plotly_chart(spo2_fig)


# Loop to update dashboard (Adjust if needed)
while True:

    # Update everything
    update_metrics()
    update_ecg()
    update_heart_rate()
    update_spo2()

    time.sleep(0.1)  # Adjust update rate if needed
