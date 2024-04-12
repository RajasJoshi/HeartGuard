import sys
from PyQt6 import QtCore
from PyQt6.QtWidgets import (
    QApplication,
    QMainWindow,
    QVBoxLayout,
    QWidget,
    QLabel,
    QHBoxLayout,
    QButtonGroup,
    QRadioButton,
)
import pyqtgraph as pg
import numpy as np
from random import randint
from qt_material import apply_stylesheet
import socket
import threading
import queue
from collections import deque
from PyQt6.QtGui import QIcon

HOST = "192.168.236.65"  
PORT = 5000
SAMPLING_RATE = 860  # Hz
SAMPLING_PERIOD = 1000 / SAMPLING_RATE  # ms
buffer_size = 4 * SAMPLING_RATE  # Adjust buffer size as needed

# Shared data queue
raw_data_queue = queue.Queue()
filtered_data_queue = queue.Queue()
heartrate_data_queue = queue.Queue()



# Client connection function
def connect_to_server():
    with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as s:
        s.connect((HOST, PORT))  
        while True:
            data = s.recv(1024)
            if not data:
                break
            
            for value in data.decode("utf-8").splitlines():
                if value:
                    values = value.split(',')
                    for value in values:
                        value = value.strip()
                        if value[:3] == "raw":
                            try:
                                raw_data_queue.put(float(value[4:]))
                            except:
                                print('raw failed',value[4:])
                                pass
                        elif value[:8] == "filtered":
                            try:
                                filtered_data_queue.put(float(value[9:]))
                            except:
                                print('filtered failed',value[9:])
                                pass
                        elif value[:9] == "heartrate":
                            try:
                                heartrate_data_queue.put(float(value[10:]))
                            except:
                                print('hr failed',value[10:])
                                pass


# Start the client connection thread
client_thread = threading.Thread(target=connect_to_server)
client_thread.start()


class MainWindow(QMainWindow):
    def __init__(self):
        super().__init__()

        self.setWindowTitle("HEARTGUARD LITE")

        self.raw_buffer = deque([0] * buffer_size, maxlen=buffer_size) 
        self.filtered_buffer = deque([0] * buffer_size, maxlen=buffer_size)
        self.heartrate_buffer = deque([0] * buffer_size, maxlen=buffer_size)
        
        
        # Central Widget and Main Layout
        central_widget = QWidget()
        self.setCentralWidget(central_widget)
        main_layout = QVBoxLayout(central_widget)
        self.setWindowIcon(QIcon("assets/banner.png"))

        # Theme control elements in info row
        self.theme_button_group = QButtonGroup()
        self.dark_blue_button = QRadioButton("Dark Blue")
        self.light_blue_button = QRadioButton("Light Blue")
        self.theme_button_group.addButton(self.dark_blue_button)
        self.theme_button_group.addButton(self.light_blue_button)
        self.dark_blue_button.setChecked(True)
        self.theme_button_group.buttonToggled.connect(self.update_theme)

        # Information Row
        info_row_layout = QHBoxLayout()
        self.heart_rate_label = QLabel("Heart Rate: ")
        self.resp_rate_label = QLabel("Resp Rate: ")
        self.core_temp_label = QLabel("Core Temp: ")
        self.spo2_label = QLabel("SpO2: ")
        self.hrv_label = QLabel("HRV: ")
        info_row_layout.addWidget(self.heart_rate_label)
        info_row_layout.addWidget(self.resp_rate_label)
        info_row_layout.addWidget(self.core_temp_label)
        info_row_layout.addWidget(self.spo2_label)
        info_row_layout.addWidget(self.hrv_label)
        # Add theme controls to the info row layout
        info_row_layout.addWidget(self.dark_blue_button)
        info_row_layout.addWidget(self.light_blue_button)
        main_layout.addLayout(info_row_layout)

        # Create PlotWidgets with initial labels, titles, and legends
        self.plot1 = pg.PlotWidget()
        self.plot1.setLabel("bottom", "X-Axis")
        self.plot1.setLabel("left", "Y-Axis")
        self.plot1.setTitle("ECG")
        self.plot1.addLegend()

        self.plot2 = pg.PlotWidget()
        self.plot2.setLabel("bottom", "X-Axis")
        self.plot2.setLabel("left", "Y-Axis")
        self.plot2.setTitle("Filtered ECG")
        self.plot2.addLegend()

        self.plot3 = pg.PlotWidget()
        self.plot3.setLabel("bottom", "X-Axis")
        self.plot3.setLabel("left", "Y-Axis")
        self.plot3.setTitle("Heart Rate")
        self.plot3.setYRange(40,220)
        self.plot3.addLegend()

        # Graph Row (You might want another layout here depending on graph arrangement)
        main_layout.addWidget(self.plot1)
        main_layout.addWidget(self.plot2)
        main_layout.addWidget(self.plot3)

        # Initial plotting
        self.graphUpdater()

        # Timer for periodic updates
        self.timer = QtCore.QTimer()
        self.timer.timeout.connect(self.graphUpdater)
        self.timer.start(30)  # Update every second

    def graphUpdater(self):
        while not raw_data_queue.empty():
            try:
                raw = raw_data_queue.get()
                self.raw_buffer.append(raw)
            except:
                pass
        while not filtered_data_queue.empty():
            try:
                filtered = filtered_data_queue.get()
                self.filtered_buffer.append(filtered)
            except:
                pass
        while not heartrate_data_queue.empty():
            try:
                heartrate = heartrate_data_queue.get()
                self.heartrate_buffer.append(heartrate)
            except:
                pass
        
        x1_data = np.arange(-len(self.raw_buffer), 0)
        x2_data = np.arange(-len(self.filtered_buffer), 0)
        x3_data = np.arange(-len(self.heartrate_buffer), 0)
        y1_data = np.array(self.raw_buffer)
        y2_data = np.array(self.filtered_buffer)
        y3_data = np.array(self.heartrate_buffer)

        # Update the plots
        self.plot1.plot(x1_data, y1_data, clear=True)
        self.plot2.plot(x2_data, y2_data, clear=True)
        self.plot3.plot(x3_data, y3_data, clear=True)

        # Update information labels (example)
        self.heart_rate_label.setText("Heart Rate: {}".format(randint(60, 100)))
        self.resp_rate_label.setText("Resp Rate: {}".format(randint(12, 20)))
        self.core_temp_label.setText(
            "Core Temp: {:.1f}".format(np.random.uniform(36.5, 37.5))
        )
        self.spo2_label.setText("SpO2: {}".format(randint(95, 99)))
        self.hrv_label.setText("HRV: {}".format(randint(40, 80)))

    def update_theme(self, button):
        themes = {
            self.dark_blue_button: "dark_blue.xml",
            self.light_blue_button: "light_blue.xml",
        }
        if button.isChecked():
            theme_file = themes[button]
            apply_stylesheet(app, theme=theme_file)


if __name__ == "__main__":
    app = QApplication(sys.argv)
    window = MainWindow()
    apply_stylesheet(app, theme="dark_blue.xml")  # Initial theme
    window.show()
    app.exec()