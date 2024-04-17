#include "window.h"
#include <sstream>
#include <vector>

const int UPDATE_INTERVAL_MS = 40;

Window::Window(QWidget *parent) : QWidget(parent) 
{
    heartqt.window = this;

    //Create status bar
    statusBar = new QStatusBar(this);

    // set up the initial plot data for the first graph
    for( int index=0; index<plotDataSize; ++index )
    {
        xData1[index] = index;
        yData1[index] = 0;
    }

    // set up the initial plot data for the second graph
    for( int index=0; index<plotDataSize; ++index )
    {
        xData2[index] = index;
        yData2[index] = 0;
    }

    // set up the initial plot data for the third graph
    for( int index=0; index<plotDataSize; ++index )
    {
        xData3[index] = index;
        yData3[index] = 0;
    }

    bpm = 0;
    spo2 = 0;
    ir = 0;
    red = 0;

    QString message = "BPM: " + QString::number(bpm) + " SpO2: " + QString::number(spo2) + " IR: " + QString::number(ir) + " Red: " + QString::number(red);

    // Add a message to the status bar
    statusBar->showMessage(message);

    // create curves for the three graphs
    curve1 = new QwtPlotCurve;
    curve2 = new QwtPlotCurve;
    curve3 = new QwtPlotCurve;

    // create plots for the three graphs
    plot1 = new QwtPlot;
    plot2 = new QwtPlot;
    plot3 = new QwtPlot;

    plot1->setStyleSheet("background-color: #777; color: #ddd;");
    plot2->setStyleSheet("background-color: #777; color: #ddd;");
    plot3->setStyleSheet("background-color: #777; color: #ddd;");

    // make plot curves from the data and attach them to the plots
    curve1->setSamples(xData1, yData1, plotDataSize);
    curve1->attach(plot1);
    curve2->setSamples(xData2, yData2, plotDataSize);
    curve2->attach(plot2);
    curve3->setSamples(xData3, yData3, plotDataSize);
    curve3->attach(plot3);


    QPen bluePen(Qt::green);

    curve1->setPen(bluePen);
    curve2->setPen(bluePen);
    curve3->setPen(bluePen);


    plot1->setAxisScale(QwtPlot::yLeft,-5,5);
    plot1->setAxisScale(QwtPlot::xBottom,0,UPDATE_INTERVAL_MS / 250);
    plot1->replot();
    plot1->show();

    plot2->setAxisScale(QwtPlot::yLeft,-5,5);
    plot2->setAxisScale(QwtPlot::xBottom,0,UPDATE_INTERVAL_MS / 250);
    plot2->replot();
    plot2->show();

    plot3->setAxisScale(QwtPlot::yLeft,20,250);
    plot3->setAxisScale(QwtPlot::xBottom,0,UPDATE_INTERVAL_MS / 250);
    plot3->replot();
    plot3->show();

    button = new QPushButton("Reset");
    // see https://doc.qt.io/qt-5/signalsandslots-syntaxes.html
    connect(button,&QPushButton::clicked,this,&Window::reset);

    // set up the layout
    vLayout1 = new QVBoxLayout();
    vLayout1->addWidget(button);

    vLayout2 = new QVBoxLayout();
    vLayout2->addWidget(plot1);

    vLayout3 = new QVBoxLayout();
    vLayout3->addWidget(plot2);

    vLayout4 = new QVBoxLayout();
    vLayout4->addWidget(plot3);

    vLayout5 = new QVBoxLayout();
    vLayout5->addWidget(statusBar);

    // plot the graphs below button
    hLayout = new QVBoxLayout();
    hLayout->addLayout(vLayout1);
    hLayout->addLayout(vLayout2);
    hLayout->addLayout(vLayout3);
    hLayout->addLayout(vLayout4);
    hLayout->addLayout(vLayout5);

    setLayout(hLayout);

    // Example style sheet (adjust colors as needed)
    QString styleSheet = R"(
        QWidget {
            background-color: #303030; 
            color: white; 
        }
        QPushButton { 
            background-color: #505050; 
            color: white;
        }
    )";

    this->setStyleSheet(styleSheet);


    

    // Initialize TCP Client
    tcpClient = new QTcpSocket(this);

    // Connect to server (replace with your actual server details)
    tcpClient->connectToHost("10.0.0.21", 5000); 

    // Connect signals and slots (we'll add these slots next)
    connect(tcpClient, &QTcpSocket::connected, this, &Window::handleConnected);
    connect(tcpClient, &QTcpSocket::readyRead, this, &Window::handleDataReceived);
    connect(tcpClient, &QTcpSocket::errorOccurred, this, &Window::handleSocketError);

    //     // a fake data sample every 10ms
    // heartqt.startms(10);
    // Screen refresh every 40ms
    startTimer(UPDATE_INTERVAL_MS);
}

Window::~Window() {
    heartqt.stop();
}

void Window::reset() {
    // set up the initial plot data for the first graph
    for( int index=0; index<plotDataSize; ++index )
    {
        xData1[index] = index;
        yData1[index] = 0;
    }

    // set up the initial plot data for the second graph
    for( int index=0; index<plotDataSize; ++index )
    {
        xData2[index] = index;
        yData2[index] = 0;
    }

    // set up the initial plot data for the third graph
    for( int index=0; index<plotDataSize; ++index )
    {
        xData3[index] = index;
        yData3[index] = 0;
    }

    bpm = 0;
    spo2 = 0;
    ir = 0;
    red = 0;
}


void Window::hasData(std::string& received) {
    mtx.lock();
    // Move the existing data for all three graphs
    std::move( yData1, yData1 + plotDataSize - 1, yData1 + 1 );
    std::move( yData2, yData2 + plotDataSize - 1, yData2 + 1 );
    std::move( yData3, yData3 + plotDataSize - 1, yData3 + 1 );
    

    // Create a stringstream from the input string
    std::istringstream iss(received);
    std::string message;

    std::vector<std::string> result;


    while (std::getline(iss, message, '#')) {
        if (!message.empty()) {
            std::istringstream segmentStream(message);
            std::string token;
            std::vector<std::string> tokens;
            std::cout << "Message: " << message << "\n";

            while (std::getline(segmentStream, token, ',')) {
                tokens.push_back(token);
            }
            std::cout << "Tokens: " << tokens.size() << "\n";

            if (tokens.size() == 8) {
                for (const std::string& subToken : tokens) {
                    result.push_back(subToken);
                }
            }else{
                result = {"0","0","0","0","0","0","0","0"};
            }
            tokens.clear();
        }
    }


    // Update the first graph data
    yData1[0] = std::stod(result[1]);
    // Update the second graph data (example)
    yData2[0] = std::stod(result[2]);
    // Update the third graph data (example)
    yData3[0] = std::stod(result[3]);
    bpm = std::stod(result[4]);
    spo2 = std::stod(result[5]);
    ir = std::stod(result[6]);
    red = std::stod(result[7]);
    mtx.unlock();
}

// screen refresh
void Window::timerEvent( QTimerEvent * )
{
    mtx.lock();
    // Update the first graph
    curve1->setSamples(xData1, yData1, plotDataSize);
    // Update the second graph
    curve2->setSamples(xData2, yData2, plotDataSize);
    // Update the third graph
    curve3->setSamples(xData3, yData3, plotDataSize);
    mtx.unlock();
    plot1->replot();
    plot2->replot();
    plot3->replot();
    update();
    QString message = "BPM: " + QString::number(bpm) + 
                    " SpO2: " + QString::number(spo2) + 
                    " IR: " + QString::number(ir) + 
                    " Red: " + QString::number(red);
    statusBar->showMessage(message);

    
}

void Window::handleConnected() {
    qDebug() << "Connected to server!";
    // You might want to update UI elements or take other actions on successful connection
}

void Window::handleDataReceived() {
    QByteArray data = tcpClient->readAll();
    std::string strData = data.toStdString();
    qDebug() << "Data received:" << data;
    hasData(strData);

    // Process the received data by integrating it into your 'hasData' function
}

void Window::handleSocketError(QAbstractSocket::SocketError error) {
    qDebug() << "Error:" << tcpClient->errorString();
    // Handle connection errors appropriately
}