#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>


#include <QDebug>
#include <QList>
#include <QTime>
#include <QFile>
#include <QDataStream>
#include <QElapsedTimer>

#include <qextserialenumerator.h>
#include <qextserialport.h>

#include <math.h>

#include <qwt_symbol.h>
#include <qwt_legend.h>

#include <qwt_plot_canvas.h>
#include <qwt_plot_grid.h>
#include <qwt_plot_curve.h>
#include <qwt_plot_marker.h>
#include <qwt_plot_picker.h>
#include <qwt_picker_machine.h>

#include "qwtchartzoom.h"

#define UART_BUFF_SIZE 512
#define MAX_TEMP 450


typedef quint8 uint8_t;
typedef quint16 uint16_t;

#define NUM(x) QString::number(x)


namespace Ui {
    class MainWindow;
}



class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();

private slots:
    void on_pbPortOpen_clicked();

    void onReadyRead();

    void on_pbCmd_clicked();

    void on_btnClear_clicked();

private:
    Ui::MainWindow *ui;

    QextSerialPort *m_port;
    QFile *m_file_log;


    QwtLegend *m_legend;
    QwtPlotGrid *m_grid;
    QwtPlotCurve *m_curv_temp, *m_curv_pwm, *m_curv_need_temp;
    QwtPlotMarker *m_marker_need_temp;

    //QwtSymbol *m_symbol1;

    //QwtPlotZoomer *m_zoom;
    QwtChartZoom *m_zoom;
    QwtPlotPicker *m_picker;


    void toOutput(QString msg);


    void init_plot(void);
    void destroy_plot(void);
    void draw_plot(void);

    void add_newdata(void);

    QElapsedTimer m_uart_timer;

    QVector<qreal> m_uart_temp_x;
    QVector<qreal> m_uart_temp_y;

    QVector<qreal> m_uart_pwm_x;
    QVector<qreal> m_uart_pwm_y;


    QByteArray m_uart_rx;
};

#endif // MAINWINDOW_H
