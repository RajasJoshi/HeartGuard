import dash
from dash import dcc, html, Input, Output, State
import dash_bootstrap_components as dbc
import plotly.express as px
import numpy as np
import pandas as pd
import socket
import threading
import queue

HOST = "127.0.0.1"  # Standard loopback interface (localhost)
PORT = 5000  # Port to listen on

# Sample data
heart_rate_data = 100 + np.random.randn(100)  # Random data for HR
heart_rate = 100
resp_rate = 28
core_temp = 40.3
spo2 = 92
spo2_data = 90 + np.random.randn(100)  # Random data for SpO2
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
                html.Span(" | "),
                html.Span("HRV: "),
                html.Span(id="hrv"),
                html.Span(" | "),
                html.Span(
                    dbc.Switch(id="theme-switch", label="Dark Mode", value=False),
                    className="theme-switch-container",
                ),
                dbc.Row(
                    [
                        dbc.Col(
                            dbc.Button(
                                "Save Data",
                                id="save-button",
                                className="save-button",
                            )
                        ),
                        # Column for Save Button
                    ],
                ),
                html.Span(id="save-confirmation"),
                html.Span(id="output-image-upload"),  # For confirmation messages
            ],
        ),
        html.Div(  # Content area
            id="content",
            children=[
                dcc.Graph(id="ecg-graph"),
                dcc.Graph(id="heart-rate-graph"),
                dcc.Graph(id="spo2-graph"),
            ],
        ),
        html.Div(
            [
                # ... Your other layout components ...
                dcc.Store(id="data-queue"),  # Store the initial empty queue here
                dcc.Interval(id="interval-component", interval=1000, n_intervals=0),
            ]
        ),
    ]
)

# Shared data queue
data_queue = queue.Queue()


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


@app.callback(
    Output("save-confirmation", "children"),
    Input("save-button", "n_clicks"),
    State("ecg-graph", "figure"),
    State("heart-rate-graph", "figure"),  # Add HR graph
    State("spo2-graph", "figure"),
)  # Add SpO2 graph
def save_data(n_clicks, ecg_figure, hr_figure, spo2_figure):
    if n_clicks:
        ecg_data = ecg_figure["data"][0]["y"]
        hr_data = hr_figure["data"][0]["y"]
        spo2_data = spo2_figure["data"][0]["y"]

        # Option 1: Save as separate CSV files
        pd.DataFrame(ecg_data).to_csv("ecg_data.csv", index=False)
        pd.DataFrame(hr_data).to_csv("hr_data.csv", index=False)
        pd.DataFrame(spo2_data).to_csv("spo2_data.csv", index=False)

        return "ECG, HR, and SpO2 data saved!"


@app.callback(
    Output("heart-rate", "children"),
    Output("resp-rate", "children"),
    Output("core-temp", "children"),
    Output("spo2", "children"),
    Output("hrv", "children"),
    Input("theme-switch", "value"),  # Or a suitable trigger
)
def update_vital_signs(theme_value):
    # ... Logic to get the current values (if needed) ...
    return heart_rate, resp_rate, core_temp, spo2, hrv


@app.callback(
    Output("ecg-graph", "figure"),
    Input("interval-component", "n_intervals"),  # Assuming an Interval component
    Input("theme-switch", "value"),
    State("data-queue", "data"),
)  # Keep existing input if needed
def update_ecg_graph(n_intervals, dark_mode, data_queue):
    try:
        new_data = data_queue.get_nowait()
        ecg_fig = px.line(new_data, title="ECG Signal", labels={"value": "ECG (mV)"})

        # Access the trace (assuming it's the first one)
        ecg_trace = ecg_fig.data[0]

        # Set the trace name
        ecg_trace.name = "ECG Signal"

        if dark_mode:
            ecg_fig.update_layout(
                paper_bgcolor="rgba(0,0,0,0)",
                plot_bgcolor="rgba(0,0,0,0)",
                font_color="white",
            )

        return ecg_fig
    except queue.Empty:
        return dash.no_update


@app.callback(
    Output("heart-rate-graph", "figure"),
    Input("theme-switch", "value"),
)
def update_heart_rate_graph(dark_mode):
    hr_fig = px.line(heart_rate_data, title="Heart Rate", labels={"value": "HR (bpm)"})

    hr_trace = hr_fig.data[0]
    hr_trace.name = "Heart Rate"

    if dark_mode:
        hr_fig.update_layout(
            paper_bgcolor="rgba(0,0,0,0)",
            plot_bgcolor="rgba(0,0,0,0)",
            font_color="white",
        )

    return hr_fig  # Return both figure and text


@app.callback(
    Output("spo2-graph", "figure"),
    Input("theme-switch", "value"),
)
def update_spo2_graph(dark_mode):
    spo2_fig = px.line(spo2_data, title="Blood Oxygen", labels={"value": "SpO2 (%)"})

    spo2_trace = spo2_fig.data[0]
    spo2_trace.name = "SpO2"

    if dark_mode:
        spo2_fig.update_layout(
            paper_bgcolor="rgba(0,0,0,0)",
            plot_bgcolor="rgba(0,0,0,0)",
            font_color="white",
        )

    return spo2_fig  # Return both figure and text


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
