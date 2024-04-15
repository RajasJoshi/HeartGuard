#ifndef WINDOW_H
#define WINDOW_H

#include <QWidget>
#include <QBoxLayout>
#include <QPushButton>
#include <qwt/qwt_plot.h>
#include <qwt/qwt_plot_curve.h>
#include <QTcpSocket>
#include <mutex>
#include <cmath>
#include "CppTimer.h"

class Window : public QWidget
{
    Q_OBJECT

    class HeartGuardQt : public CppTimer {
        static constexpr double gain = 7.5;
    public:
        Window* window = nullptr;
        int count = 0;
        void timerEvent() {
            double inVal = gain * sin(M_PI * count / 50.0);
            window->hasData(inVal);
            ++count;
        }
    };

public:
    Window();
    ~Window();

protected:
    void timerEvent(QTimerEvent *event) override;

private:
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
    
    QVBoxLayout *hLayout; // Overall layout

    HeartGuardQt heartqt;

    double xData1[plotDataSize]; // Data array for the first graph
    double yData1[plotDataSize];
    double xData2[plotDataSize]; // Data array for the second graph
    double yData2[plotDataSize];
    double xData3[plotDataSize]; // Data array for the third graph
    double yData3[plotDataSize];

    void reset();
    void hasData(double inVal);

    QTcpSocket *tcpSocket; // TCP socket for network communication

    std::mutex mtx;
};

#endif // WINDOW_H
