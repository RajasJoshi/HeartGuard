import dash
from dash import dcc, html, Input, Output
import dash_bootstrap_components as dbc
import plotly.express as px
import numpy as np
import csv
import socket
import threading
import queue


# ecg_server_address = ("127.0.0.1", 5000)
# data_queue = queue.Queue()  # Queue to transfer data from thread to Dash


# # Data Reception Thread
# def receive_ecg_data():
#     with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as s:
#         s.connect(ecg_server_address)
#         print("Connected to ECG data server")

#         while True:
#             data = s.recv(1024).decode()
#             if not data:
#                 break
#             ecg_str, hr_str = data.strip().split(",")
#             data_queue.put((ecg_str, hr_str))


# # Start the data reception thread
# receive_thread = threading.Thread(target=receive_ecg_data, daemon=True)
# receive_thread.start()


# ECG Data Loading (Replace with your loading logic)
def load_ecg_data(filename):
    ecg_data = np.loadtxt(filename)
    return ecg_data


# Preprocess to get heart rate (Placeholder, align with your sensor)
def calculate_heart_rate(ecg_data, sample_rate):
    # Simulate simple calculation
    peaks = np.where(ecg_data > 0.5)[0]
    rr_intervals = np.diff(peaks) / sample_rate
    heart_rate = 60.0 / rr_intervals
    return heart_rate


def calculate_current_heart_rate(heart_rate_data):
    num_recent_values = 10  # Take the last 10 values
    if len(heart_rate_data) >= num_recent_values:
        recent_values = heart_rate_data[-num_recent_values:]
        return sum(recent_values) / len(recent_values)
    else:
        return None  # Or a default value if not enough data


# Load data (adjust as needed)
ecg_data = load_ecg_data("assets/ECG_1000Hz_7.dat")
heart_rate_data = calculate_heart_rate(
    ecg_data, sample_rate=1000
)  # Assuming original data is 1000Hz
heart_rate = calculate_current_heart_rate(heart_rate_data)  # Get current HR
resp_rate = 28
core_temp = 40.3
spo2 = 92
hrv = 0.2

# Dash App
app = dash.Dash(__name__)


app.layout = html.Div(
    [
        html.Div(  # Header
            id="header",
            children=[
                html.Img(src=app.get_asset_url("banner.png"), alt="Logo", id="logo"),
                html.H2("HeartGuard Lite", id="title"),
            ],
        ),
        html.Div(  # Information Row
            id="info-row",
            children=[
                html.Span("Heart Rate: "),
                html.Span(id="heart-rate"),
                html.Span(" bpm | "),  # Added separators
                html.Span("Resp Rate: "),
                html.Span(id="resp-rate"),
                html.Span(" bpm | "),
                html.Span("Core Temp: "),
                html.Span(id="core-temp"),
                html.Span(" °C | "),
                html.Span("SpO2: "),
                html.Span(id="spo2"),
                html.Span(" %"),
                html.Div(
                    dbc.Switch(id="theme-switch", label="Dark Mode", value=False),
                    className="theme-switch-container",
                ),
            ],
        ),
        html.Div(  # Content area
            id="content",
            children=[
                dcc.Graph(id="ecg-graph"),
                dcc.Graph(id="heart-rate-graph"),
            ],
        ),
    ]
)


@app.callback(
    Output("heart-rate", "children"),
    Output("resp-rate", "children"),
    Output("core-temp", "children"),
    Output("spo2", "children"),
    Input("theme-switch", "value"),  # Or a suitable trigger
)
def update_vital_signs(theme_value):
    # ... Logic to get the current values (if needed) ...
    return heart_rate, resp_rate, core_temp, spo2


@app.callback(Output("ecg-graph", "figure"), Input("theme-switch", "value"))
def update_ecg_graph(dark_mode):

    # global data_queue
    # print("Queue size After:", data_queue.qsize())
    # ecg_str, hr_str = data_queue.get_nowait()
    # ecg_dataN += [float(x) for x in ecg_str.split()]
    # heart_rate = float(hr_str)
    # print("ECG:", ecg_dataN, "HR:", heart_rate)

    ecg_fig = px.line(ecg_data, title="ECG Signal", labels={"value": "ECG (mV)"})

    if dark_mode:
        ecg_fig.update_layout(
            paper_bgcolor="rgba(0,0,0,0)",
            plot_bgcolor="rgba(0,0,0,0)",
            font_color="white",
        )

    return ecg_fig


@app.callback(Output("heart-rate-graph", "figure"), Input("theme-switch", "value"))
def update_heart_rate_graph(dark_mode):
    hr_fig = px.line(heart_rate_data, title="Heart Rate", labels={"value": "HR (bpm)"})

    if dark_mode:
        hr_fig.update_layout(
            paper_bgcolor="rgba(0,0,0,0)",
            plot_bgcolor="rgba(0,0,0,0)",
            font_color="white",
        )

    return hr_fig  # Return both figure and text


app.index_string = """<!DOCTYPE html>
<html>

<head>
    <title>HeartGuard Dashboard</title>
    {%metas%}
    {%favicon%}
    {%css%}
    <link rel="stylesheet" href="https://cdn.jsdelivr.net/npm/bootstrap@5.2.3/dist/css/bootstrap.min.css">
    <link href="https://fonts.googleapis.com/css2?family=Roboto:wght@400;700&display=swap" rel="stylesheet"> 
    <script src="https://cdn.jsdelivr.net/npm/bootstrap@5.2.3/dist/js/bootstrap.bundle.min.js"></script>
    <link rel="stylesheet" href="/assets/styles.css">
</head>

<body>
    <script>
        window.addEventListener('load', function () {
            setTimeout(function () {
                document.getElementById('theme-switch').addEventListener('change', function (event) {
                    document.body.classList.toggle('dark-mode');
                });
            }, 100);
        });
    </script>

    {%app_entry%}

    <div id="header">
    </div>

    <div id="content">
    </div>

    <footer>
        {%config%}
        {%scripts%}
        {%renderer%}
    </footer>
</body>

</html>"""

if __name__ == "__main__":
    app.run_server(debug=True)
