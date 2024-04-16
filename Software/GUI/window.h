#ifndef WINDOW_H
#define WINDOW_H

#include <QWidget>
#include <QBoxLayout>
#include <QPushButton>
#include <qwt/qwt_plot.h>
#include <qwt/qwt_plot_curve.h>
#include <mutex>
#include <cmath>
#include <fstream>
#include "CppTimer.h"
#include "tcpclient.hpp"

class Window : public QWidget
{
    Q_OBJECT

    class HeartGuardQt : public CppTimer {
    public:
        Window* window = nullptr;
        int count = 0;
        void timerEvent() {
            std::string received = "ecg,0.0056,0.14234,85#ppg,82,12";
            window->hasData(received);
            ++count;
        }
    };

public:
    Window(QWidget *parent = 0);
    virtual ~Window();

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
    void hasData(std::string& received);

    std::mutex mtx;
};

#endif // WINDOW_H
