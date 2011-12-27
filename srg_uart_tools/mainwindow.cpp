#include "mainwindow.h"
#include "ui_mainwindow.h"


#pragma pack(push)
#pragma pack(1)
typedef struct {
    uint8_t header;

    uint8_t type;
    uint16_t value1;
    uint16_t value2;
    uint16_t value3;
    uint16_t value4;

    uint8_t crc;
} TPCInfo;
#pragma pack(pop)

#define PCINFO_HEADER 0xDE
#define PCINFO_TYPE_IRON 0x01
#define PCINFO_TYPE_PRINT 0x05

uint8_t _crc_ibutton_update(uint8_t crc, uint8_t data) {
    uint8_t i;
    crc = crc ^ data;
    for (i = 0; i < 8; i++) {
        if (crc & 0x01)
            crc = (crc >> 1) ^ 0x8C;
        else
            crc >>= 1;
    }
    return crc;
}

uint8_t check_uart_info(TPCInfo *info) {

    if(info->header != PCINFO_HEADER)
        return 0;

    uint8_t i, crc = 0;

    uint8_t *p = (uint8_t *)info;
    for(i = 0; i < sizeof(TPCInfo) - 1; i++)
        crc = _crc_ibutton_update(crc, p[i]);

    if(info->crc != crc)
        return 0;

    return 1;
}




MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    setCentralWidget(ui->centralWidget);

    ui->cbPorts->clear();
    QList<QextPortInfo> ports = QextSerialEnumerator::getPorts();
    foreach( QextPortInfo port, ports) {
        ui->cbPorts->addItem(port.physName);
    }
    ui->cbPorts->setCurrentIndex(0);

    m_port = new QextSerialPort(QextSerialPort::EventDriven);
    m_file_log = new QFile("srg_uart.log");
    m_file_log->open(QIODevice::WriteOnly | QIODevice::Append);

    init_plot();
}

MainWindow::~MainWindow()
{
    destroy_plot();

    if(m_port->isOpen()) m_port->close();
    delete m_port;

    if(m_file_log->isOpen()) m_file_log->close();
    delete m_file_log;

    delete ui;
}

void MainWindow::on_pbPortOpen_clicked()
{
  if(m_port->isOpen()) {
      m_port->close();

      ui->pbPortOpen->setText("Open");
      ui->lbPortStatus->setText("<span style=color:red>closed</span>");

      toOutput("Port " + m_port->portName() + " closed<br><br>");

      ui->gbCommands->setEnabled(false);
      ui->gbOutput->setEnabled(false);

  }
  else {
      QString index = ui->cbSpeeds->currentText();
      BaudRateType speed;

      if(index == "9600")
        speed = BAUD9600;
      else
      if(index == "19200")
        speed = BAUD19200;
      else
      if(index == "38400")
        speed = BAUD38400;
      else
      if(index == "57600")
        speed = BAUD57600;
      else
      if(index == "115200")
        speed = BAUD115200;
      else
        speed = BAUD50;

      m_port->setBaudRate(speed);
      m_port->setFlowControl(FLOW_OFF);
      m_port->setParity(PAR_NONE);
      m_port->setDataBits(DATA_8);
      m_port->setStopBits(STOP_1);

      if(m_port->open(QIODevice::ReadWrite) == true) {
          ui->pbPortOpen->setText("Close");
          ui->lbPortStatus->setText("<span style=color:green>open</span>");

          ui->gbCommands->setEnabled(true);
          ui->gbOutput->setEnabled(true);

          connect(m_port, SIGNAL(readyRead()), this, SLOT(onReadyRead()));

          toOutput("Port " + m_port->portName() + " opened<br>");
      }
      else {
          ui->lbPortStatus->setText("<span style=color:red>Error: "+ m_port->errorString() +"</span>");
      }

  }
}

QString bytesToString(const QByteArray &data)
{
    static const char numbers[] = "0123456789ABCDEF";
    QString r;
    r.resize(data.size()*3-1);
    int rpos = 0;
    for (int i = 0; i < data.size(); ++i) {
        uchar c = data.at(i);
        r[rpos++] = QChar(numbers[(c >> 4) % 16]);
        r[rpos++] = QChar(numbers[c % 16]);
        if (i != data.size() - 1)
            r[rpos++] = QChar(' ');
    }
    return r;
}

void MainWindow::onReadyRead()
{
    QString log;

    int a = m_port->bytesAvailable();
    if(a <= 0) return;

    if(a + m_uart_rx.size() > UART_BUFF_SIZE) {
      m_uart_rx.remove(0, a);
    }
    m_uart_rx.append(m_port->read(a));

    int index = m_uart_rx.indexOf(PCINFO_HEADER, 0);
    if(index < 0)
        return;

    if(index > 0)
        m_uart_rx.remove(0, index);

    if(m_uart_rx.size() < sizeof(TPCInfo))
        return;

    TPCInfo info;
    uint8_t i;
    uint8_t *p = (uint8_t *)&info;
    for(i = 0; i < sizeof(TPCInfo); i++)
        p[i] = m_uart_rx.at(i);

    m_uart_rx.remove(0, sizeof(TPCInfo));

    if(!check_uart_info(&info))
        return;

    /*
        QString str;
        str = bytesToString(m_uart_rx);
        log.append("RECV: " + str + "\n");
    */


    if(info.type == PCINFO_TYPE_IRON) {
        //log.append("X = " + NUM(m_uart_timer.elapsed() / 1000.0) + "; Y = "  + NUM(info.value) + '\n');

        quint64 timer = m_uart_timer.elapsed() / 1000.0;

        m_uart_temp_x.append(timer);
        m_uart_temp_y.append(info.value1);

        m_uart_pwm_x.append(timer);
        m_uart_pwm_y.append(info.value2);

        m_marker_need_temp->setValue(0, info.value3);

        draw_plot();
    }


    //log2.append("Recv: " + cmd + '\n');


    if(log.size()) toOutput(log);
}


void MainWindow::toOutput(QString msg) {
    ui->teOutput->append(msg.replace('\0', '?'));

    if(m_file_log->isOpen()) {
        msg = "<strong>[" + QTime::currentTime().toString("hh:mm:ss") + "]</strong> " + msg;
        m_file_log->write((msg.replace("<br>", "\r\n") + "\r\n").toAscii());
    }
}

void MainWindow::on_pbCmd_clicked()
{
    QString cmd = ui->leCmd->text();

    QString log;

    log = "SND `" + cmd + "`";
    toOutput(log);

    m_port->write(cmd.toAscii() + '\r');
    m_port->flush();
}

void MainWindow::init_plot(void) {
    ui->qwtPlot->setTitle(QString::fromUtf8("Зависимость T(t)"));

    m_legend = new QwtLegend();
    m_legend->setItemMode(QwtLegend::ReadOnlyItem);
    ui->qwtPlot->insertLegend(m_legend, QwtPlot::BottomLegend);

    m_grid = new QwtPlotGrid;
    m_grid->enableXMin(true);

    m_grid->setMajPen(QPen(Qt::black, 0, Qt::DotLine));
    m_grid->setMinPen(QPen(Qt::gray, 0, Qt::DotLine));
    m_grid->attach(ui->qwtPlot);

    ui->qwtPlot->enableAxis(QwtPlot::yLeft, true);
    ui->qwtPlot->enableAxis(QwtPlot::yRight, true);
    ui->qwtPlot->enableAxis(QwtPlot::xBottom, true);

    ui->qwtPlot->setAxisTitle(QwtPlot::xBottom, QString::fromUtf8("t, сек"));
    //ui->qwtPlot->setAxisScale(QwtPlot::xBottom, 0, 100);

    ui->qwtPlot->setAxisTitle(QwtPlot::yLeft, QString::fromUtf8("Temp, °C"));
    ui->qwtPlot->setAxisScale(QwtPlot::yLeft, 0, 400);

    ui->qwtPlot->setAxisTitle(QwtPlot::yRight, QString::fromUtf8("Pow, %"));
    ui->qwtPlot->setAxisScale(QwtPlot::yRight, 0, 300);

    m_curv_temp = new QwtPlotCurve(QString::fromUtf8("T1(t)"));
    m_curv_temp->setRenderHint(QwtPlotItem::RenderAntialiased);
    m_curv_temp->setYAxis(QwtPlot::yLeft);
    m_curv_temp->setXAxis(QwtPlot::xBottom);
    m_curv_temp->setPen(QPen(Qt::blue));

    //m_curv_temp->setItemAttribute(QwtPlotItem::AutoScale, true);

   /* m_symbol1 = new QwtSymbol();
    m_symbol1->setStyle(QwtSymbol::Ellipse);
    m_symbol1->setPen(QColor(Qt::black));
    m_symbol1->setSize(4);
    m_curv1->setSymbol(m_symbol1);
    */

    m_curv_pwm = new QwtPlotCurve(QString::fromUtf8("T2(t)"));
    m_curv_pwm->setRenderHint(QwtPlotItem::RenderAntialiased);
    m_curv_pwm->setYAxis(QwtPlot::yRight);
    m_curv_pwm->setXAxis(QwtPlot::xBottom);
    m_curv_pwm->setPen(QPen(Qt::darkGreen));

    //m_curv_pwm->setItemAttribute(QwtPlotItem::AutoScale, true);



    m_marker_need_temp = new QwtPlotMarker();
    m_marker_need_temp->setValue(0.0, 0.0);
    m_marker_need_temp->setLineStyle(QwtPlotMarker::HLine);
    m_marker_need_temp->setLabelAlignment(Qt::AlignRight | Qt::AlignBottom);
    m_marker_need_temp->setLinePen(QPen(Qt::red, 0, Qt::DashDotLine));
    m_marker_need_temp->setYAxis(QwtPlot::yLeft);
    m_marker_need_temp->setXAxis(QwtPlot::xBottom);
    m_marker_need_temp->attach(ui->qwtPlot);


    m_zoom = new QwtChartZoom(ui->qwtPlot);
    m_zoom->setRubberBandColor(Qt::white);

    ui->qwtPlot->canvas()->setCursor(Qt::ArrowCursor);

    m_curv_temp->attach(ui->qwtPlot);
    m_curv_pwm->attach(ui->qwtPlot);

    ui->qwtPlot->setAutoReplot(true);

    //ui->qwtPlot->setAxisAutoScale(QwtPlot::yRight);
    //ui->qwtPlot->setAxisAutoScale(QwtPlot::yLeft);


    m_picker = new QwtPlotPicker(QwtPlot::xBottom, QwtPlot::yLeft,
        QwtPlotPicker::CrossRubberBand, QwtPicker::AlwaysOn, ui->qwtPlot->canvas());
    m_picker->setStateMachine(new QwtPickerDragPointMachine());
    m_picker->setRubberBandPen(QColor(Qt::green));
    m_picker->setRubberBand(QwtPicker::CrossRubberBand);
    m_picker->setTrackerPen(QColor(Qt::blue));


    m_uart_timer.start();

    m_uart_temp_x.clear();
    m_uart_temp_y.clear();
}


void MainWindow::destroy_plot(void) {

    //delete m_legend;
    //delete m_grid;

    //delete m_curv1;
    //delete m_curv2;

    //delete m_symbol1;
    //delete m_zoom;

}

void MainWindow::draw_plot(void) {
    m_curv_temp->setRawSamples(m_uart_temp_x.constData(), m_uart_temp_y.constData(), m_uart_temp_x.size());
    m_curv_pwm->setRawSamples(m_uart_pwm_x.constData(), m_uart_pwm_y.constData(), m_uart_pwm_x.size());


    if(m_uart_pwm_x.size() > 10000)
        this->on_btnClear_clicked();

}

void MainWindow::on_btnClear_clicked()
{
    m_uart_timer.start();

    m_uart_temp_x.clear();
    m_uart_temp_y.clear();

    m_uart_pwm_x.clear();
    m_uart_pwm_y.clear();

    ui->qwtPlot->setAxisScale(QwtPlot::xBottom, 0, 50);

    draw_plot();
}
