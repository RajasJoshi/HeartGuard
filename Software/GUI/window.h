#ifndef WINDOW_H
#define WINDOW_H

#include <QWidget>
#include <QString>
#include <QStatusBar>
#include <QBoxLayout>
#include <QPushButton>
#include <qwt/qwt_plot.h>
#include <qwt/qwt_plot_curve.h>
#include <iostream>
#include <mutex>
#include <cmath>
#include <fstream>
#include "CppTimer.h"
#include <QTcpSocket>
#include <QHostAddress>
#include <QApplication>

class Window : public QWidget
{
    Q_OBJECT

    class HeartGuardQt : public CppTimer {
    public:
        Window* window = nullptr;
        int count = 0;
        void timerEvent() {
            ++count;
        }
    };

public:
    Window(QWidget *parent = 0);
    virtual ~Window();

protected:
    void timerEvent(QTimerEvent *event) override;

private slots: 
    void handleConnected();
    void handleDataReceived();
    void handleSocketError(QAbstractSocket::SocketError error);

private:

    QStatusBar *statusBar;

    QTcpSocket *tcpClient; // Our TCP Client
    
    static constexpr int plotDataSize = 4000;

    QPushButton *button;
    QwtPlot *plot1; // Plot for the first graph
    QwtPlot *plot2; // Plot for the second graph
    QwtPlot *plot3; // Plot for the third graph
    QwtPlotCurve *curve1; // Curve for the first graph
    QwtPlotCurve *curve2; // Curve for the second graph
    QwtPlotCurve *curve3; // Curve for the third graph

    QVBoxLayout *vLayout1; // Layout for the first graph
    QVBoxLayout *vLayout2; // Layout for the second graph
    QVBoxLayout *vLayout3; // Layout for the third graph
    QVBoxLayout *vLayout4; // Layout for the third graph
    QVBoxLayout *vLayout5; // Layout for the message
    
    QVBoxLayout *hLayout; // Overall layout

    HeartGuardQt heartqt;

    double xData1[plotDataSize]; // Data array for the first graph
    double yData1[plotDataSize];
    double xData2[plotDataSize]; // Data array for the second graph
    double yData2[plotDataSize];
    double xData3[plotDataSize]; // Data array for the third graph
    double yData3[plotDataSize];
    double bpm;
    double spo2;
    double ir;
    double red;

    void reset();
    void hasData(std::string& received);

    std::mutex mtx;
};

#endif // WINDOW_H
