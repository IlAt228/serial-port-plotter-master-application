/***************************************************************************
**  This file is part of Serial Port Plotter                              **
**                                                                        **
**                                                                        **
**  Serial Port Plotter is a program for plotting integer data from       **
**  serial port using Qt and QCustomPlot                                  **
**                                                                        **
**  This program is free software: you can redistribute it and/or modify  **
**  it under the terms of the GNU General Public License as published by  **
**  the Free Software Foundation, either version 3 of the License, or     **
**  (at your option) any later version.                                   **
**                                                                        **
**  This program is distributed in the hope that it will be useful,       **
**  but WITHOUT ANY WARRANTY; without even the implied warranty of        **
**  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         **
**  GNU General Public License for more details.                          **
**                                                                        **
**  You should have received a copy of the GNU General Public License     **
**  along with this program.  If not, see http://www.gnu.org/licenses/.   **
**                                                                        **
****************************************************************************
**           Author: Borislav                                             **
**           Contact: b.kereziev@gmail.com                                **
**           Date: 29.12.14                                               **
****************************************************************************/

#include "mainwindow.hpp"
#include "ui_mainwindow.h"
#include "LEDManager.h"

const QBluetoothUuid MainWindow::NUS_SERVICE_UUID{QString("6E400001-B5A3-F393-E0A9-E50E24DCCA9E")};
const QBluetoothUuid MainWindow::NUS_TX_UUID     {QString("6E400003-B5A3-F393-E0A9-E50E24DCCA9E")};
const QBluetoothUuid MainWindow::NUS_RX_UUID     {QString("6E400002-B5A3-F393-E0A9-E50E24DCCA9E")};
//#include <x86intrin.h>

/**
 * @brief Вычисление частоты сердечных сокращений (ЧСС) из данных AFE4404
 * @param irSamples Массив значений с ИК канала AFE4404 (940 нм)
 * @param sampleCount Количество отсчетов в массиве
 * @param samplingRate Частота дискретизации в Гц
 * @return Частота сердечных сокращений в ударах в минуту (0 при недостаточном качестве сигнала)
 */
int calculateHeartRate(std::vector<int> irSamples, int sampleCount, float samplingRate) {
    if (sampleCount < 50 || samplingRate <= 0) return 0;

    // 1. Предварительная фильтрация: скользящее среднее для подавления высокочастотных шумов
    QVector<double> filteredSignal(sampleCount);
    const int filterWindowSize = 3; // Небольшое окно для сохранения формы сигнала

    for (int i = filterWindowSize/2; i < sampleCount - filterWindowSize/2; ++i) {
        double sum = 0;
        for (int j = -filterWindowSize/2; j <= filterWindowSize/2; ++j) {
            sum += irSamples[i + j];
        }
        filteredSignal[i] = sum / filterWindowSize;
    }
    // Копируем краевые значения
    for (int i = 0; i < filterWindowSize/2; ++i) {
        filteredSignal[i] = irSamples[i];
        filteredSignal[sampleCount - 1 - i] = irSamples[sampleCount - 1 - i];
    }

    // 2. Выделение AC компоненты (пульсовой волны)
    QVector<double> acComponent(sampleCount);
    const int dcFilterWindowSize = static_cast<int>(samplingRate); // Окно ~1 сек для выделения DC

    for (int i = dcFilterWindowSize/2; i < sampleCount - dcFilterWindowSize/2; ++i) {
        double dcSum = 0;
        for (int j = -dcFilterWindowSize/2; j <= dcFilterWindowSize/2; ++j) {
            dcSum += filteredSignal[i + j];
        }
        double dcValue = dcSum / dcFilterWindowSize;
        acComponent[i] = filteredSignal[i] - dcValue;
    }

    // 3. Нормализация AC компоненты и детектирование пиков
    QVector<int> peaks;
    double maxValue = 0;
    double minValue = acComponent[dcFilterWindowSize/2];

    // Находим диапазон значений для нормализации
    for (int i = dcFilterWindowSize/2; i < sampleCount - dcFilterWindowSize/2; ++i) {
        if (acComponent[i] > maxValue) maxValue = acComponent[i];
        if (acComponent[i] < minValue) minValue = acComponent[i];
    }

    double amplitude = maxValue - minValue;
    if (amplitude < 100) return 0; // Слишком маленькая амплитуда - плохой сигнал

    double threshold = 0.3 * amplitude; // Порог для детектирования пиков
    double minPeakDistance = 0.4 * samplingRate; // Минимальное расстояние между пиками ~250 мс

    int lastPeakIndex = -minPeakDistance;

    // Ищем локальные максимумы, превышающие порог
    for (int i = dcFilterWindowSize/2 + 1; i < sampleCount - dcFilterWindowSize/2 - 1; ++i) {
        // Проверяем, является ли точка локальным максимумом
        if (acComponent[i] > acComponent[i-1] &&
            acComponent[i] > acComponent[i+1] &&
            acComponent[i] > threshold) {

            // Проверяем минимальное расстояние до предыдущего пика
            if (i - lastPeakIndex >= minPeakDistance) {
                peaks.append(i);
                lastPeakIndex = i;
            }
        }
    }

    // 4. Расчет ЧСС по последним пикам
    if (peaks.size() < 2) return 0; // Недостаточно пиков для расчета

    // Рассчитываем средний интервал между пиками (RR-интервал)
    double avgRRInterval = 0;
    int peakCount = 0;
    const int maxPeaksToUse = 5; // Используем не более 5 последних пиков для расчета

    int startIndex = qMax(0, static_cast<int>(peaks.size()) - maxPeaksToUse);
    for (int i = startIndex; i < peaks.size() - 1; ++i) {
        avgRRInterval += (peaks[i+1] - peaks[i]);
        peakCount++;
    }

    avgRRInterval /= peakCount; // Средний RR-интервал в отсчетах
    double rrIntervalSeconds = avgRRInterval / samplingRate; // RR-интервал в секундах

    // Преобразуем в ЧСС (уд/мин)
    double heartRate = 60.0 / rrIntervalSeconds;

    // Проверка на физиологически возможные значения
    if (heartRate < 40.0) {
        //return 0;
    }

    if (heartRate > 200.0) {
        //return 0;
    }

    return static_cast<int>(qRound(heartRate));
}


/**
 * @brief Constructor
 * @param parent
 */
MainWindow::MainWindow (QWidget *parent) :
  QMainWindow (parent),
  ui (new Ui::MainWindow),

  /* Populate colors */
  line_colors{
      /* For channel data (gruvbox palette) */
      /* Light */
      QColor ("#fb4934"), //R
      QColor ("#fabd2f"),//("#b8bb26"), //IR
      QColor ("#8ec07c"), //("#fabd2f"), //G
      QColor ("#83a598"), //Ambient
      QColor ("#d3869b"),
      QColor ("#8ec07c"),
      QColor ("#fe8019"),
      /* Light */
      QColor ("#cc241d"),
      QColor ("#98971a"),
      QColor ("#d79921"),
      QColor ("#458588"),
      QColor ("#b16286"),
      QColor ("#689d6a"),
      QColor ("#d65d0e"),
   },
  gui_colors {
      /* Monochromatic for axes and ui */
      QColor (48,  47,  47,  255), /**<  0: qdark ui dark/background color */
      QColor (80,  80,  80,  255), /**<  1: qdark ui medium/grid color */
      QColor (170, 170, 170, 255), /**<  2: qdark ui light/text color */
      QColor (48,  47,  47,  200)  /**<  3: qdark ui dark/background color w/transparency */
    },

  /* Main vars */
  connected (false),
  plotting (false),
  dataPointNumber (0),
  channels(0),
  serialPort (nullptr),
  STATE (WAIT_START),
  NUMBER_OF_POINTS (500)
{
  ui->setupUi (this);

  /* Init UI and populate UI controls */
  createUI();

  /* Setup plot area and connect controls slots */
  setupPlot();

  timer = new QTimer(this);
  connect(timer, &QTimer::timeout, this, &MainWindow::onTimeout);
  timer->start(100); // Запуск таймера с интервалом 100 мс

  data_vector.assign(3, std::vector<double>(30));
  green_samples.resize(500);

  /* Wheel over plot when plotting */
  connect (ui->plot, SIGNAL (mouseWheel (QWheelEvent*)), this, SLOT (on_mouse_wheel_in_plot (QWheelEvent*)));

  /* Slot for printing coordinates */
  connect (ui->plot, SIGNAL (mouseMove (QMouseEvent*)), this, SLOT (onMouseMoveInPlot (QMouseEvent*)));

  /* Channel selection */
  connect (ui->plot, SIGNAL(selectionChangedByUser()), this, SLOT(channel_selection()));
  connect (ui->plot, SIGNAL(legendDoubleClick (QCPLegend*, QCPAbstractLegendItem*, QMouseEvent*)), this, SLOT(legend_double_click (QCPLegend*, QCPAbstractLegendItem*, QMouseEvent*)));

  /* Connect update timer to replot slot */
  connect (&updateTimer, SIGNAL (timeout()), this, SLOT (replot()));
    //settingsWindow = new AdvancedSettings(this);
  //connect(settingsWindow, &AdvancedSettings::sendDataRequested,
          //this, &MainWindow::onSendDataRequested);
  m_csvFile = nullptr;
  ui->main_Rf1->addItems(QStringList() << "500k" << "250k" << "100k" << "50k" << "25k" << "10k" << "1M" << "2M");
  ui->main_Rf2->addItems(QStringList() << "500k" << "250k" << "100k" << "50k" << "25k" << "10k" << "1M" << "2M");
  ui->main_Cf1->addItems(QStringList() << "5p" << "2.5p" << "10p" << "7.5p" << "20p" << "17.5p" << "25p" << "22.5p");
  ui->main_Cf2->addItems(QStringList() << "5p" << "2.5p" << "10p" << "7.5p" << "20p" << "17.5p" << "25p" << "22.5p");
  ui->main_checkSEP->setChecked(false);
}
bool COMopen = false;
bool hardReset = true;
/** ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

/**
 * @brief Destructor
 */
MainWindow::~MainWindow()
{
    closeCsvFile();
      
    if (bleController) bleController->disconnectFromDevice();
    delete ui;
}
/** ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

void MainWindow::onTimeout()
{
    if (QDateTime::currentMSecsSinceEpoch() - last_data_time > 100) {
        ui->verticalSlider_4->setValue(1);
        ui->verticalSlider_4->setValue(0);
        qDebug() << "Таймер сработал!";
    }

}

/**
 * @brief Create remaining elements and populate the controls
 */
void MainWindow::createUI()
{
    /* Python BLE bridge mode — no BLE scan needed */
    ui->comboPort->clear();
    ui->comboPort->addItem("localhost");
    enable_com_controls(true);
    ui->savePNGButton->setEnabled(false);
    ui->statusBar->showMessage("Run ble_bridge.py, then select 'localhost' and click Connect.");

    /* Populate baud rate combo box with standard rates */
    ui->comboBaud->addItem ("1200");
    ui->comboBaud->addItem ("2400");
    ui->comboBaud->addItem ("4800");
    ui->comboBaud->addItem ("9600");
    ui->comboBaud->addItem ("19200");
    ui->comboBaud->addItem ("38400");
    ui->comboBaud->addItem ("57600");
    ui->comboBaud->addItem ("115200");
    /* And some not-so-standard */
    ui->comboBaud->addItem ("128000");
    ui->comboBaud->addItem ("153600");
    ui->comboBaud->addItem ("230400");
    ui->comboBaud->addItem ("256000");
    ui->comboBaud->addItem ("460800");
    ui->comboBaud->addItem ("921600");

    /* Select 115200 bits by default */
    ui->comboBaud->setCurrentIndex (7);

    /* Populate data bits combo box */
    ui->comboData->addItem ("8 bits");
    ui->comboData->addItem ("7 bits");

    /* Populate parity combo box */
    ui->comboParity->addItem ("none");
    ui->comboParity->addItem ("odd");
    ui->comboParity->addItem ("even");

    /* Populate stop bits combo box */
    ui->comboStop->addItem ("1 bit");
    ui->comboStop->addItem ("2 bits");

    /* Initialize the listwidget */
    ui->listWidget_Channels->clear();
}
/** ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

/**
 * @brief Setup the plot area
 */
void MainWindow::setupPlot()
{
    /* Remove everything from the plot */
    ui->plot->clearItems();

    /* Background for the plot area */
    ui->plot->setBackground (gui_colors[0]);

    /* Used for higher performance (see QCustomPlot real time example) */
    ui->plot->setNotAntialiasedElements (QCP::aeAll);
    QFont font;
    font.setStyleStrategy (QFont::NoAntialias);
    ui->plot->legend->setFont (font);

    /** See QCustomPlot examples / styled demo **/
    /* X Axis: Style */
    ui->plot->xAxis->grid()->setPen (QPen(gui_colors[2], 1, Qt::DotLine));
    ui->plot->xAxis->grid()->setSubGridPen (QPen(gui_colors[1], 1, Qt::DotLine));
    ui->plot->xAxis->grid()->setSubGridVisible (true);
    ui->plot->xAxis->setBasePen (QPen (gui_colors[2]));
    ui->plot->xAxis->setTickPen (QPen (gui_colors[2]));
    ui->plot->xAxis->setSubTickPen (QPen (gui_colors[2]));
    ui->plot->xAxis->setUpperEnding (QCPLineEnding::esSpikeArrow);
    ui->plot->xAxis->setTickLabelColor (gui_colors[2]);
    ui->plot->xAxis->setTickLabelFont (font);
    /* Range */
    ui->plot->xAxis->setRange (dataPointNumber - ui->spinPoints->value(), dataPointNumber);

    /* Y Axis */
    ui->plot->yAxis->grid()->setPen (QPen(gui_colors[2], 1, Qt::DotLine));
    ui->plot->yAxis->grid()->setSubGridPen (QPen(gui_colors[1], 1, Qt::DotLine));
    ui->plot->yAxis->grid()->setSubGridVisible (true);
    ui->plot->yAxis->setBasePen (QPen (gui_colors[2]));
    ui->plot->yAxis->setTickPen (QPen (gui_colors[2]));
    ui->plot->yAxis->setSubTickPen (QPen (gui_colors[2]));
    ui->plot->yAxis->setUpperEnding (QCPLineEnding::esSpikeArrow);
    ui->plot->yAxis->setTickLabelColor (gui_colors[2]);
    ui->plot->yAxis->setTickLabelFont (font);
    /* Range */
    //ui->plot->yAxis->setRange (ui->spinAxesMin->value(), ui->spinAxesMax->value());
    /* User can change Y axis tick step with a spin box */
    //ui->plot->yAxis->setAutoTickStep (false);
    //ui->plot->yAxis->(ui->spinYStep->value());

    /* User interactions Drag and Zoom are allowed only on X axis, Y is fixed manually by UI control */
    ui->plot->setInteraction (QCP::iRangeDrag, true);
    //ui->plot->setInteraction (QCP::iRangeZoom, true);
    ui->plot->setInteraction (QCP::iSelectPlottables, true);
    ui->plot->setInteraction (QCP::iSelectLegend, true);
    ui->plot->axisRect()->setRangeDrag (Qt::Horizontal);
    ui->plot->axisRect()->setRangeZoom (Qt::Horizontal);

    /* Legend */
    QFont legendFont;
    legendFont.setPointSize (9);
    ui->plot->legend->setVisible (true);
    ui->plot->legend->setFont (legendFont);
    ui->plot->legend->setBrush (gui_colors[3]);
    ui->plot->legend->setBorderPen (gui_colors[2]);
    /* By default, the legend is in the inset layout of the main axis rect. So this is how we access it to change legend placement */
    ui->plot->axisRect()->insetLayout()->setInsetAlignment (0, Qt::AlignTop|Qt::AlignRight);
}
/** ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

/**
 * @brief Enable/disable COM controls
 * @param enable true enable, false disable
 */
void MainWindow::enable_com_controls (bool enable)
{
  /* Com port properties */
  ui->comboBaud->setEnabled (enable);
  ui->comboData->setEnabled (enable);
  ui->comboParity->setEnabled (enable);
  ui->comboPort->setEnabled (enable);
  ui->comboStop->setEnabled (enable);

  /* Toolbar elements */
  ui->actionConnect->setEnabled (enable);
  ui->actionPause_Plot->setEnabled (!enable);
  ui->actionDisconnect->setEnabled (!enable);
}
/** ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

/**
 * @brief Open the inside serial port; connect its signals
 * @param portInfo
 * @param baudRate
 * @param dataBits
 * @param parity
 * @param stopBits
 */

// ── BLE helpers ───────────────────────────────────────────────────────────────

bool MainWindow::isBleConnected() const
{
    if (tcpSocket && tcpSocket->state() == QAbstractSocket::ConnectedState)
        return true;
    return nusRxChar.isValid();
}

void MainWindow::bleWrite(const QByteArray &data)
{
    if (tcpSocket && tcpSocket->state() == QAbstractSocket::ConnectedState) {
        tcpSocket->write(data);
        return;
    }
    if (!nusService || !nusRxChar.isValid()) return;
    nusService->writeCharacteristic(nusRxChar, data, QLowEnergyService::WriteWithoutResponse);
}

// ── TCP ───────────────────────────────────────────────────────────────────────

void MainWindow::connectTCP(const QString &host, quint16 port)
{
    connect(this, SIGNAL(portOpenOK()),  this, SLOT(portOpenedSuccess()));
    connect(this, SIGNAL(portOpenFail()), this, SLOT(portOpenedFail()));
    connect(this, SIGNAL(portClosed()),  this, SLOT(onPortClosed()));
    connect(this, SIGNAL(newData(QStringList)), this, SLOT(onNewDataArrived(QStringList)));
    connect(this, SIGNAL(newData(QStringList)), this, SLOT(saveStream(QStringList)));

    if (tcpSocket) { tcpSocket->deleteLater(); tcpSocket = nullptr; }
    tcpSocket = new QTcpSocket(this);
    connect(tcpSocket, &QTcpSocket::connected,    this, &MainWindow::onTcpConnected);
    connect(tcpSocket, &QTcpSocket::readyRead,    this, &MainWindow::onTcpDataReady);
    connect(tcpSocket, &QTcpSocket::disconnected, this, &MainWindow::onTcpDisconnected);
    connect(tcpSocket, &QAbstractSocket::errorOccurred, this, [this](QAbstractSocket::SocketError) {
        ui->statusBar->showMessage("TCP: " + tcpSocket->errorString());
        emit portOpenFail();
    });

    ui->statusBar->showMessage(QString("TCP: connecting to %1:%2 ...").arg(host).arg(port));
    tcpSocket->connectToHost(host, port);
}

void MainWindow::onTcpConnected()
{
    ui->statusBar->showMessage("TCP: connected to Python bridge!");
    emit portOpenOK();
}

void MainWindow::onTcpDataReady()
{
    processData(tcpSocket->readAll());
}

void MainWindow::onTcpDisconnected()
{
    if (connected) {
        emit portClosed();
        ui->statusBar->showMessage("TCP: disconnected!");
        connected = false;
        plotting  = false;
        ui->actionConnect->setEnabled(true);
        ui->actionPause_Plot->setEnabled(false);
        ui->actionDisconnect->setEnabled(false);
        enable_com_controls(true);
    }
    tcpSocket->deleteLater();
    tcpSocket = nullptr;
}

// ── BLE connect / service discovery ──────────────────────────────────────────

void MainWindow::connectBLE(const QBluetoothDeviceInfo &device)
{
    connect(this, SIGNAL(portOpenOK()),  this, SLOT(portOpenedSuccess()));
    connect(this, SIGNAL(portOpenFail()), this, SLOT(portOpenedFail()));
    connect(this, SIGNAL(portClosed()),  this, SLOT(onPortClosed()));
    connect(this, SIGNAL(newData(QStringList)), this, SLOT(onNewDataArrived(QStringList)));
    connect(this, SIGNAL(newData(QStringList)), this, SLOT(saveStream(QStringList)));

    if (bleController) {
        bleController->disconnectFromDevice();
        delete bleController;
        bleController = nullptr;
    }

    QBluetoothLocalDevice localDev;
    bleController = QLowEnergyController::createCentral(device, localDev.address(), this);
    connect(bleController, &QLowEnergyController::connected,
            this, &MainWindow::onBleControllerConnected);
    connect(bleController, &QLowEnergyController::serviceDiscovered,
            this, &MainWindow::onBleServiceDiscovered);
    connect(bleController, &QLowEnergyController::disconnected,
            this, &MainWindow::onBleControllerDisconnected);
    connect(bleController, &QLowEnergyController::errorOccurred,
            this, [this](QLowEnergyController::Error err) {
                ui->statusBar->showMessage(QString("BLE error code: %1").arg((int)err));
                qDebug() << "BLE errorOccurred:" << (int)err;
                emit portOpenFail();
            });

    ui->statusBar->showMessage("BLE: connecting...");
    bleController->connectToDevice();

    // Timeout: if not connected in 15s — reset and let user retry
    QTimer::singleShot(15000, this, [this]() {
        if (!isBleConnected() && !connected) {
            ui->statusBar->showMessage("BLE: timeout. Click Connect to retry.");
            if (bleController) bleController->disconnectFromDevice();
            nusRxChar = QLowEnergyCharacteristic();
            enable_com_controls(true);
        }
    });
}

void MainWindow::onBleControllerConnected()
{
    ui->statusBar->showMessage("BLE: discovering services...");
    bleController->discoverServices();
}

void MainWindow::onBleServiceDiscovered(const QBluetoothUuid &uuid)
{
    if (uuid != NUS_SERVICE_UUID) return;

    if (nusService) { delete nusService; nusService = nullptr; }

    nusService = bleController->createServiceObject(uuid, this);
    connect(nusService, &QLowEnergyService::stateChanged,
            this, &MainWindow::onBleServiceStateChanged);
    connect(nusService, &QLowEnergyService::characteristicChanged,
            this, &MainWindow::onBleCharacteristicChanged);
    nusService->discoverDetails();
}

void MainWindow::onBleServiceStateChanged(QLowEnergyService::ServiceState state)
{
    if (state != QLowEnergyService::RemoteServiceDiscovered) return;

    auto txChar = nusService->characteristic(NUS_TX_UUID);
    nusRxChar   = nusService->characteristic(NUS_RX_UUID);

    // Subscribe to notifications from ESP32
    auto desc = txChar.descriptor(
        QBluetoothUuid::DescriptorType::ClientCharacteristicConfiguration);
    if (desc.isValid())
        nusService->writeDescriptor(desc, QByteArray::fromHex("0100"));

    emit portOpenOK();
}

void MainWindow::onBleCharacteristicChanged(const QLowEnergyCharacteristic &c,
                                             const QByteArray &value)
{
    Q_UNUSED(c)
    processData(value);
}

void MainWindow::onBleControllerDisconnected()
{
    nusRxChar = QLowEnergyCharacteristic();
    if (connected) {
        emit portClosed();
        ui->statusBar->showMessage("BLE: disconnected!");
        connected = false;
        plotting  = false;
        ui->actionConnect->setEnabled(true);
        ui->actionPause_Plot->setEnabled(false);
        ui->actionDisconnect->setEnabled(false);
        ui->actionRecord_stream->setEnabled(true);
        enable_com_controls(true);
    }
}
/** ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

/**
 * @brief Slot for closing the port
 */
void MainWindow::onPortClosed()
{
    //qDebug() << "Port closed signal received!";
    updateTimer.stop();
    connected = false;
    plotting = false;
    
    //--
    closeCsvFile();
    
    disconnect (this, SIGNAL(portOpenOK()),  this, SLOT(portOpenedSuccess()));
    disconnect (this, SIGNAL(portOpenFail()), this, SLOT(portOpenedFail()));
    disconnect (this, SIGNAL(portClosed()),  this, SLOT(onPortClosed()));
    disconnect (this, SIGNAL(newData(QStringList)), this, SLOT(onNewDataArrived(QStringList)));
    disconnect (this, SIGNAL(newData(QStringList)), this, SLOT(saveStream(QStringList)));
}
/** ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

/**
 * @brief Port Combo Box index changed slot; displays info for selected port when combo box is changed
 * @param arg1
 */
void MainWindow::on_comboPort_currentIndexChanged (const QString &arg1)
{
    Q_UNUSED(arg1)
}
/** ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

/**
 * @brief Slot for port opened successfully
 */
void MainWindow::portOpenedSuccess()
{
    //qDebug() << "Port opened signal received!";
    setupPlot();                                                                          // Create the QCustomPlot area
    ui->statusBar->showMessage ("Connected!");
    enable_com_controls (false);                                                                // Disable controls if port is open
    
    if(ui->actionRecord_stream->isChecked())
    {
        //--> Create new CSV file with current date/timestamp
        openCsvFile();
    }
    /* Lock the save option while recording */
    ui->actionRecord_stream->setEnabled(false);

    updateTimer.start (20);                                                                // Slot is refreshed 20 times per second
    connected = true;                                                                      // Set flags
    plotting = true;
}
/** ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

/**
 * @brief Slot for fail to open the port
 */
void MainWindow::portOpenedFail()
{
    ui->statusBar->showMessage("BLE: connection failed. Try again.");
    nusRxChar = QLowEnergyCharacteristic();
    enable_com_controls(true);
}
/** ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

/**
 * @brief Replot
 */
void MainWindow::replot()
{
  ui->plot->xAxis->setRange (dataPointNumber - ui->spinPoints->value(), dataPointNumber);
  ui->plot->replot();
}
/** ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

/**
 * @brief Slot for new data from serial port . Data is comming in QStringList and needs to be parsed
 * @param newData
 */

/*void MainWindow::onNewDataArrived(QStringList newData)
{
    static int data_members = 0;
    static int channel = 0;
    static int i = 0;
    volatile bool you_shall_NOT_PASS = false;

    // When a fast baud rate is set (921kbps was the first to starts to bug),
       this method is called multiple times (2x in the 921k tests), so a flag
       is used to throttle
       TO-DO: Separate processes, buffer data (1) and process data (2)
    while (you_shall_NOT_PASS) {}
    you_shall_NOT_PASS = true;

    if (plotting)
      {
        // Get size of received list
        data_members = newData.size();

        // Parse data
        for (i = 0; i < data_members; i++)
          {
            // Update number of axes if needed
            while (ui->plot->plottableCount() <= channel)
              {
                // Add new channel data
                ui->plot->addGraph();
                ui->plot->graph()->setPen (line_colors[channels % CUSTOM_LINE_COLORS]);
                ui->plot->graph()->setName (QString("Channel %1").arg(channels));
                if(ui->plot->legend->item(channels))
                {
                    ui->plot->legend->item (channels)->setTextColor (line_colors[channels % CUSTOM_LINE_COLORS]);
                }
                ui->listWidget_Channels->addItem(ui->plot->graph()->name());
                ui->listWidget_Channels->item(channel)->setForeground(QBrush(line_colors[channels % CUSTOM_LINE_COLORS]));
                channels++;
              }

            // [TODO] Method selection and plotting
            // X-Y
            if (0)
              {

              }
            // Rolling (v1.0.0 compatible)
            else
              {
                // Add data to Graph 0
                  double raw = newData[channel].toDouble();         // сырое значение от твоего парсера
                  double volts = raw * (1.2 / (1 << 21));           // перевод в Вольты
                //ui->plot->graph(channel)->addData(dataPointNumber, volts);
                ui->plot->graph(channel)->addData (dataPointNumber, newData[channel].toDouble());
                // Increment data number and channel
                channel++;
              }
          }

        //Post-parsing
        // X-Y
        if (0)
          {

          }
        // Rolling (v1.0.0 compatible)
        else
          {
            dataPointNumber++;
            channel = 0;
          }
      }
    you_shall_NOT_PASS = false;
}*/
void MainWindow::onNewDataArrived(QStringList newData)
{
    static int data_members = 0;
    volatile bool you_shall_NOT_PASS = false;

    while (you_shall_NOT_PASS) {}
    you_shall_NOT_PASS = true;
    int idx;
    if (plotting)
    {
        data_members = newData.size();
        int maxChannels = 4;

        // --- Создаём три графика один раз ---
        while (ui->plot->plottableCount() < maxChannels)
        {
            idx = ui->plot->plottableCount();
            ui->plot->addGraph();
            ui->plot->graph(idx)->setPen(line_colors[idx % CUSTOM_LINE_COLORS]);
            ui->plot->graph(idx)->setName(QString("Channel %1").arg(idx));

            if(ui->plot->legend->item(idx))
                ui->plot->legend->item(idx)->setTextColor(line_colors[idx % CUSTOM_LINE_COLORS]);

            ui->listWidget_Channels->addItem(ui->plot->graph(idx)->name());
            ui->listWidget_Channels->item(idx)->setForeground(QBrush(line_colors[idx % CUSTOM_LINE_COLORS]));
        }

        // --- Добавляем данные в каждый график ---
        for (int ch = 0; ch < maxChannels && ch < data_members; ch++)
        {
            double raw = newData[ch].toDouble();
            //raw = convert_to_power(raw, idx);
            ui->plot->graph(ch)->addData(dataPointNumber, raw);
        }

        // --- Счётчик X (точек) ---
        dataPointNumber++;
    }

    you_shall_NOT_PASS = false;
}
double MainWindow::convert_to_power(double raw, int ind)
{
    double s;
    int r;
    double Idac;
    switch (ind)
    {
    case 0:
        s=0.46;
        r=LEDfeatures.GAIN_SEP;;
        Idac = Offset.LED2 * 0.47;
        if (Offset.POL2 == 1)
            Idac *= (-1);
        break;
    case 1:
        s=0.77;
        r=LEDfeatures.GAIN_SEP;
        Idac = Offset.LED3 * 0.47;
        if (Offset.POL3 == 1)
            Idac *= (-1);
        break;
    case 2:
        s=0.26;
        r=LEDfeatures.GAIN;
        Idac = Offset.LED1 * 0.47;
        if (Offset.POL1 == 1)
            Idac *= (-1);
        break;
    case 3: //ambient
        s=0.26;
        r=LEDfeatures.GAIN;
        Idac = Offset.AMB1 * 0.47;
        if (Offset.AMB1 == 1)
            Idac *= (-1);
        break;
    default:
        s=1000;
        r = LEDfeatures.GAIN;
        Idac = Offset.AMB1 * 0.47;
        if (Offset.AMB1 == 1)
            Idac *= (-1);
        break;
    }
    double Vadc = raw*1.2/2097000;
    double Iph = Vadc/r;
    Iph += Idac;
    double P = Iph/s;
    return P;
}
/** ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

/**
 * @brief Slot for spin box for plot minimum value on y axis
 * @param arg1
 */
void MainWindow::on_spinAxesMin_valueChanged(int arg1)
{
    ui->plot->yAxis->setRangeLower (arg1);
    ui->plot->replot();
}
/** ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

/**
 * @brief Slot for spin box for plot maximum value on y axis
 * @param arg1
 */
void MainWindow::on_spinAxesMax_valueChanged(int arg1)
{
    ui->plot->yAxis->setRangeUpper (arg1);
    ui->plot->replot();
}
/** ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

/**
 * @brief Read data for inside serial port
 */

/*
void MainWindow::readData()
{
    if(serialPort->bytesAvailable()) {                                                    // If any bytes are available
        QByteArray data = serialPort->readAll();                                          // Read all data in QByteArray

        if(!data.isEmpty()) {                                                             // If the byte array is not empty
            char *temp = data.data();                                                     // Get a '\0'-terminated char* to the data

            if (!filterDisplayedData){
                ui->textEdit_UartWindow->append(data);
            }
            for(int i = 0; temp[i] != '\0'; i++) {                                        // Iterate over the char*
                switch(STATE) {                                                           // Switch the current state of the message
                case WAIT_START:                                                          // If waiting for start [$], examine each char
                    if(temp[i] == START_MSG) {                                            // If the char is $, change STATE to IN_MESSAGE
                        STATE = IN_MESSAGE;
                        receivedData.clear();                                             // Clear temporary QString that holds the message
                        break;                                                            // Break out of the switch
                    }
                    break;
                case IN_MESSAGE:                                                          // If state is IN_MESSAGE
                    if(temp[i] == END_MSG) {                                              // If char examined is ;, switch state to END_MSG
                        STATE = WAIT_START;
                        QStringList incomingData = receivedData.split(' ');               // Split string received from port and put it into list
                        if(filterDisplayedData){
                            ui->textEdit_UartWindow->append(receivedData);
                        }
                        emit newData(incomingData);                                       // Emit signal for data received with the list
                        break;
                    }
                    else if (isdigit (temp[i]) || isspace (temp[i]) || temp[i] =='-' || temp[i] =='.')
                      {
                        // If examined char is a digit, and not '$' or ';', append it to temporary string
                        receivedData.append(temp[i]);
                      }
                    break;
                default: break;
                }
            }
        }
    }
}*/

//один канал

/*void MainWindow::readData()
{
    while(serialPort->bytesAvailable()) {
        QByteArray data = serialPort->readAll();

        for (int i = 0; i < data.size(); i++) {
            char byte = data[i];

            switch (STATE) {
            case WAIT_START:
                if (byte == START_MSG) {   // стартовое '$'
                    STATE = IN_MESSAGE;
                    receivedBytes.clear(); // очищаем буфер
                }
                break;

            case IN_MESSAGE:
                if (byte == END_MSG) {
                    STATE = WAIT_START;

                    // здесь у тебя накопились сырые байты
                    QList<int> values;
                    for (int j = 0; j + 2 < receivedBytes.size(); j += 3) {
                        int val = ((receivedBytes[j+2] & 0xFF) |
                                   ((receivedBytes[j+1] & 0xFF) << 8) |
                                   ((receivedBytes[j] & 0xFF) << 16));
                        if (val & 0x800000) val |= 0xFF000000;  // sign extension
                        values.append(val);
                    }

                    // преобразуем в QStringList для сигнала
                    QStringList strList;
                    for (int v : values)
                        strList << QString::number(v);

                    if (filterDisplayedData) {
                        ui->textEdit_UartWindow->append(strList.join(" "));
                    }

                    emit newData(strList);  // ✅ теперь ошибок компиляции не будет
                }
                else {
                    receivedBytes.append(byte);  // накапливаем байты
                }
                break;

            default:
                break;
            }
        }
    }
}*/

/*void MainWindow::readData()
{
    if(serialPort->bytesAvailable()) {
        QByteArray data = serialPort->readAll();

        if(!data.isEmpty()) {
            char *temp = data.data();

            for(int i = 0; temp[i] != '\0'; i++) {
                switch(STATE) {
                case WAIT_START:
                    if(temp[i] == START_MSG) {
                        STATE = IN_MESSAGE;
                        receivedBytes.clear();   // теперь храним сырые байты, не строку
                    }
                    break;

                case IN_MESSAGE:
                    if(temp[i] == END_MSG) {
                        STATE = WAIT_START;

                        QList<int> values;
                        // по 3 байта на канал
                        for (int j = 0; j + 2 < receivedBytes.size(); j += 3) {
                            int val = ( (receivedBytes[j+2] & 0xFF) |
                                       ((receivedBytes[j+1] & 0xFF) << 8) |
                                       ((receivedBytes[j] & 0xFF) << 16) );
                            // sign extension для 24 бит
                            if (val & 0x800000) val |= 0xFF000000;

                            values.append(val);
                        }

                        // теперь переведём в QStringList, как хочет проект
                        QStringList incomingData;
                        for (int v : values) {
                            incomingData << QString::number(v);
                        }

                        if(filterDisplayedData) {
                            ui->textEdit_UartWindow->append(incomingData.join(" "));
                        }

                        emit newData(incomingData);
                    }
                    else {
                        // всё что между $ и ; мы просто складываем байтами
                        receivedBytes.append(temp[i]);
                    }
                    break;

                default: break;
                }
            }
        }
    }
}*/

//бинарный поток
void MainWindow::readData()
{
    // not used in BLE mode — data arrives via onBleCharacteristicChanged → processData()
}

void MainWindow::processData(const QByteArray &data)
{
    if (data.isEmpty()) return;

    // для отладки можно показывать HEX-приём (раскомментировать при надобности)
    // ui->textEdit_UartWindow->append(data.toHex(' '));

    // добавляем в накопительный буфер
    rxBuffer.append(data);

    // обработка циклом — на случай, если в буфере несколько фреймов
    while (true)
    {
        // ищем старт символа
        int start = rxBuffer.indexOf('$');
        if (start == -1) {
            // не нашли старт — мусор, чистим буфер (или оставляем, если хочешь)
            rxBuffer.clear();
            break;
        }

        // если до старта есть мусор — выбрасываем его
        if (start > 0) {
            rxBuffer.remove(0, start);
        }

        // ищем конец фрейма ';' после стартового символа
        int end = rxBuffer.indexOf(';', 1); // искать начиная со смещения 1 (после '$')
        if (end == -1) {
            // полного фрейма ещё нет — ждём следующего чтения
            // (что осталось в rxBuffer — начало фрейма)
            break;
        }

        // у нас есть полный фрейм от 0 до end
        QByteArray payload = rxBuffer.mid(1, end - 1); // содержимое между $ и ;

        // удаляем обработанный фрейм из буфера (включая ';')
        rxBuffer.remove(0, end + 1);

        // проверка на корректность длины (каждое число = 3 байта)
        if (payload.size() == 0) {
            qDebug() << "Empty frame received";
            continue;
        }
        if (payload.size() % 3 != 0) {
            //qDebug() << "Bad frame length:" << payload.size() << " -> resyncing";
            // попытка ресинхронизироваться: если в текущем буфере есть следующий '$', сдвинемся к нему
            int nxt = rxBuffer.indexOf('$');
            if (nxt >= 0) {
                rxBuffer.remove(0, nxt);
            } else {
                rxBuffer.clear();
            }
            continue;
        }

        // распаковываем по 3 байта (MSB first — как у тебя MCU)
        int nvals = payload.size() / 3;
        if (nvals != 4) {
            return;
        }
        QStringList incomingData;
        for (int k = 0; k < nvals; ++k) {
            int off = k * 3;
            // MCU отправляет: (sample>>16)&0xFF, (sample>>8)&0xFF, sample&0xFF  => MSB, mid, LSB
            uint8_t b0 = static_cast<uint8_t>(payload[off]);     // MSB
            uint8_t b1 = static_cast<uint8_t>(payload[off+1]);   // mid
            uint8_t b2 = static_cast<uint8_t>(payload[off+2]);   // LSB

            int val = ( (b0 << 16) | (b1 << 8) | b2 );
            // sign extension for 24-bit signed
            if (val & 0x800000) val |= 0xFF000000;

            incomingData << QString::number(val);
        }

        qDebug()<<"nvals = "<<nvals;

        // выводим в окно (если нужно) и эмитим сигнал
        int sz = 7;
        idx_ = (idx_ + 1) % sz;
        for (int ii = 0; ii < 3; ++ii) {
            data_vector[ii][idx_] = incomingData[ii].toDouble();
        }

        if (0 <= green_idx_ && green_idx_ < 500) {
            green_samples[green_idx_] = incomingData[2].toDouble();
            green_idx_++;
        }

        if (green_idx_ == 500) {
            auto heart_rate = calculateHeartRate(green_samples, 500, 100);
            qDebug() << "Пульс " << heart_rate;
            green_idx_ = 0;
        }



        if (idx_ == 0) {
            std::vector<double> data_sum(3, 0);
            for (int ii = 0; ii < 3; ++ii) {
                for (int jj = 0; jj < sz; ++jj) {
                    data_sum[ii] += data_vector[ii][jj];
                }
            }
            calculate(data_sum[2] / sz, data_sum[0] / sz, data_sum[1] / sz);
        }
        if (filterDisplayedData && idx_ == 0) ui->textEdit_UartWindow->append(incomingData.join(" "));
        emit newData(incomingData);
        last_data_time = QDateTime::currentMSecsSinceEpoch();
    } // while(true)
}


/** ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

/**
 * @brief Number of axes combo; when changed, display axes colors in status bar
 * @param index
 */
//void MainWindow::on_comboAxes_currentIndexChanged(int index)
//{
//    if(index == 0) {
//      ui->statusBar->showMessage("Axis 1: Red");
//    } else if(index == 1) {
//        ui->statusBar->showMessage("Axis 1: Red; Axis 2: Yellow");
//    } else {
//        ui->statusBar->showMessage("Axis 1: Red; Axis 2: Yellow; Axis 3: Green");
//    }
//}
/** ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

/**
 * @brief Spin box for changing the Y Tick step
 * @param arg1
 */
void MainWindow::on_spinYStep_valueChanged(int arg1)
{
    ui->plot->yAxis->ticker()->setTickCount(arg1);
    ui->plot->replot();
    ui->spinYStep->setValue(ui->plot->yAxis->ticker()->tickCount());
}
/** ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

/**
 * @brief Save a PNG image of the plot to current EXE directory
 */
void MainWindow::on_savePNGButton_clicked()
{
    ui->plot->savePng (QString::number(dataPointNumber) + ".png", 1920, 1080, 2, 50);
}
/** ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

/**
 * @brief Prints coordinates of mouse pointer in status bar on mouse release
 * @param event
 */
void MainWindow::onMouseMoveInPlot(QMouseEvent *event)
{
    int xx = int(ui->plot->xAxis->pixelToCoord(event->position().x()));
    int yy = int(ui->plot->yAxis->pixelToCoord(event->position().y()));
    QString coordinates("X: %1 Y: %2");
    coordinates = coordinates.arg(xx).arg(yy);
    ui->statusBar->showMessage(coordinates);
}
/** ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

/**
 * @brief Send plot wheelmouse to spinbox
 * @param event
 */
void MainWindow::on_mouse_wheel_in_plot (QWheelEvent *event)
{
    QWheelEvent inverted_event = QWheelEvent(event->position(), event->globalPosition(),
                                             event->pixelDelta(), event->angleDelta(), event->buttons(),
                                             event->modifiers(), event->phase(), event->inverted());
  QApplication::sendEvent (ui->spinPoints, &inverted_event);
}
/** ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

/**
 * @brief Select both line and legend (channel)
 * @param plottable
 * @param event
 */
void MainWindow::channel_selection (void)
{
    /* synchronize selection of graphs with selection of corresponding legend items */
     for (int i = 0; i < ui->plot->graphCount(); i++)
       {
         QCPGraph *graph = ui->plot->graph(i);
         QCPPlottableLegendItem *item = ui->plot->legend->itemWithPlottable (graph);
         if (item->selected())
           {
             item->setSelected (true);
   //          graph->set (true);
           }
         else
           {
             item->setSelected (false);
     //        graph->setSelected (false);
           }
       }
}
/** ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

/**
 * @brief Rename a graph by double clicking on its legend item
 * @param legend
 * @param item
 */
void MainWindow::legend_double_click(QCPLegend *legend, QCPAbstractLegendItem *item, QMouseEvent *event)
{
    Q_UNUSED (legend)
    Q_UNUSED(event)
    /* Only react if item was clicked (user could have clicked on border padding of legend where there is no item, then item is 0) */
    if (item)
      {
        QCPPlottableLegendItem *plItem = qobject_cast<QCPPlottableLegendItem*>(item);
        bool ok;
        QString newName = QInputDialog::getText (this, "Change channel name", "New name:", QLineEdit::Normal, plItem->plottable()->name(), &ok, Qt::Popup);
        if (ok)
          {
            plItem->plottable()->setName(newName);
            for(int i=0; i<ui->plot->graphCount(); i++)
            {
                ui->listWidget_Channels->item(i)->setText(ui->plot->graph(i)->name());
            }
            ui->plot->replot();
          }
      }
}
/** ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

/**
 * @brief Spin box controls how many data points are collected and displayed
 * @param arg1
 */
void MainWindow::on_spinPoints_valueChanged (int arg1)
{
    Q_UNUSED(arg1)
    ui->plot->xAxis->setRange (dataPointNumber - ui->spinPoints->value(), dataPointNumber);
    ui->plot->replot();
}
/** ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

/**
 * @brief Shows a window with instructions
 */
void MainWindow::on_actionHow_to_use_triggered()
{
  helpWindow = new HelpWindow (this);
  helpWindow->setWindowTitle ("How to use this application");
  helpWindow->show();
}

/** ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

/**
 * @brief Connects to COM port or restarts plotting
 */

void MainWindow::on_actionConnect_triggered()
{
    if (connected)
    {
        /* Is connected, restart if paused */
        if (!plotting)
        {                                                                              // Start plotting
            updateTimer.start();                                                              // Start updating plot timer
            plotting = true;
            ui->actionConnect->setEnabled (false);
            ui->actionPause_Plot->setEnabled (true);
            ui->statusBar->showMessage ("Plot restarted!");
        }
    }
    else
    {
        connectTCP("127.0.0.1", 12345);
    }
}

/** ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

/**
 * @brief Keep COM port open but pause plotting
 */
void MainWindow::on_actionPause_Plot_triggered()
{
  if (plotting)
    {
      updateTimer.stop();                                                               // Stop updating plot timer
      plotting = false;
      ui->actionConnect->setEnabled (true);
      ui->actionPause_Plot->setEnabled (false);
      ui->statusBar->showMessage ("Plot paused, new data will be ignored");
    }
}
/** ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

/**
 * @brief Keep COM port open but pause plotting
 */
void MainWindow::on_actionRecord_stream_triggered()
{
    if (ui->actionRecord_stream->isChecked())
    {
      ui->statusBar->showMessage ("Data will be stored in csv file");
    }
    else
    {
      ui->statusBar->showMessage ("Data will not be stored anymore");
    }
}
/** ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

/**
 * @brief Closes COM port and stop plotting
 */
void MainWindow::on_actionDisconnect_triggered()
{
  if (connected)
    {
      nusRxChar = QLowEnergyCharacteristic(); // invalidate before disconnect to suppress onBleControllerDisconnected UI update
      if (bleController)
          bleController->disconnectFromDevice();
      if (tcpSocket) {
          tcpSocket->disconnectFromHost();
          tcpSocket->deleteLater();
          tcpSocket = nullptr;
      }

      emit portClosed();

      ui->statusBar->showMessage("Disconnected!");
      connected = false;
      ui->actionConnect->setEnabled(true);
      plotting  = false;
      ui->actionPause_Plot->setEnabled(false);
      ui->actionDisconnect->setEnabled(false);
      ui->actionRecord_stream->setEnabled(true);
      receivedData.clear();
      ui->savePNGButton->setEnabled(false);
      enable_com_controls(true);
    }
}
/** ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

/**
 * @brief Clear all channels data and reset plot area
 *
 * This function will not delete the channel itself (legend will stay)
 */
void MainWindow::on_actionClear_triggered()
{
    ui->plot->clearPlottables();
    ui->listWidget_Channels->clear();
    channels = 0;
    dataPointNumber = 0;
    emit setupPlot();
    ui->plot->replot();
}
/** ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

/**
 * @brief Open a new CSV file to save received data
 *
 */
void MainWindow::openCsvFile(void)
{
  m_csvFile = new QFile(QDateTime::currentDateTime().toString("yyyy-MM-d-HH-mm-ss-")+"data-out.csv");
  if(!m_csvFile)
      return;
  if (!m_csvFile->open(QIODevice::ReadWrite | QIODevice::Text))
        return;
  
}
/** ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

/**
 * @brief Open a new CSV file to save received data
 *
 */
void MainWindow::closeCsvFile(void)
{
  if(!m_csvFile) return;
  m_csvFile->close();
  if(m_csvFile) delete m_csvFile;
  m_csvFile = nullptr;
}
/** ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

/**
 * @brief Open a new CSV file to save received data
 *
 */
void MainWindow::saveStream(QStringList newData)
{
  if(!m_csvFile)
    return;
  if(ui->actionRecord_stream->isChecked())
  {
      QTextStream out(m_csvFile);
      foreach (const QString &str, newData) {
        out << str << ",";
      }
      out << "\n";
  }
}

/** ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

void MainWindow::on_pushButton_TextEditHide_clicked()
{
    if(ui->pushButton_TextEditHide->isChecked())
    {
        ui->textEdit_UartWindow->setVisible(false);
        ui->pushButton_TextEditHide->setText("Show TextBox");
    }
    else
    {
        ui->textEdit_UartWindow->setVisible(true);
        ui->pushButton_TextEditHide->setText("Hide TextBox");
    }
}

void MainWindow::on_pushButton_ShowallData_clicked()
{
    if(ui->pushButton_ShowallData->isChecked())
    {
        filterDisplayedData = false;
        ui->pushButton_ShowallData->setText("Filter Incoming Data");
    }
    else
    {
        filterDisplayedData = true;
        ui->pushButton_ShowallData->setText("Show All Incoming Data");
    }
}

void MainWindow::on_pushButton_AutoScale_clicked()
{   
    ui->plot->yAxis->rescale(true);
    ui->spinAxesMax->setValue(int(ui->plot->yAxis->range().upper) + int(ui->plot->yAxis->range().upper*0.1));
    ui->spinAxesMin->setValue(int(ui->plot->yAxis->range().lower) + int(ui->plot->yAxis->range().lower*0.1));

    need_calculate_number = 3;
    green_idx_ = -1;
    // if (data_vector[2][idx_] > 1e6) {
    //     need_calculate_number = 3;
    //     Offset.LED3 = 9;
    //     Offset.POL3 = 1;
    //     ui->textEdit_UartWindow->append("need_calculate = 3");
    // } else if (data_vector[2][idx_] > 0) {
    //     need_calculate_number = 2;
    // } else {
    //     need_calculate_number = 3;
    //     LED[need_calculate_number - 1].bright = 9;
    //     Offset.LED3 = 9;
    //     Offset.POL3 = 1;
    //     ui->textEdit_UartWindow->append("need_calculate = 3");
    // }
    // idx_ = 0;

    // ui->main_scrollLED1->setValue(LED[0].bright);
    // ui->main_scrollLED2->setValue(LED[1].bright);
    // ui->main_scrollLED3->setValue(LED[2].bright);
    // ui->verticalSlider->setValue(-Offset.LED1);
    // ui->verticalSlider_2->setValue(-Offset.LED2);
    // ui->verticalSlider_3->setValue(-Offset.LED3);
}

void MainWindow::calculate(double value1, double value2, double value3) {
    ui->textEdit_UartWindow->append("calculate");
    qDebug() << "calculate: " << value1 << " " << value2 << " " << value3 << "\n";
    if (need_calculate_number == 3) {
        if (value3 > 1e6) {
            green_idx_ = -1;
            Offset.POL3 = 1;
            if (Offset.LED3 == 15) {
                LED[2].bright--;
                Offset.LED3 = 1;
            } else  {
                Offset.LED3++;
            }
        } else if (value3 < 0) {
            green_idx_ = -1;
            LED[2].bright = 9;
            Offset.LED3 = 1;
            Offset.POL3 = 1;
        } else {
            need_calculate_number--;
        }
    } else if (need_calculate_number == 2) {
        if (value2 > 1e6) {
            green_idx_ = -1;
            Offset.POL2 = 1;
            if (Offset.LED2 == 15) {
                LED[1].bright--;
                Offset.LED2 = 1;
            } else  {
                Offset.LED2++;
            }
        } else if (value2 < 0) {
            green_idx_ = -1;
            LED[1].bright = 9;
            Offset.LED2 = 1;
            Offset.POL2 = 1;
        } else {
            need_calculate_number--;
        }
    } else if (need_calculate_number == 1) {
        if (value1 > 1e6) {
            green_idx_ = -1;
            Offset.POL1 = 1;
            if (Offset.LED1 == 15) {
                LED[0].bright--;
                Offset.LED1 = 1;
            } else  {
                Offset.LED1++;
            }
        } else if (value1 < 0) {
            green_idx_ = -1;
            LED[0].bright = 63;
            Offset.LED1 = 1;
            Offset.POL1 = 1;
        } else {
            need_calculate_number--;
            ui->plot->yAxis->rescale(true);
            auto mx = std::max(std::max(value1, value2), value3);
            auto mn = std::min(std::min(value1, value2), value3);
            ui->spinAxesMax->setValue(int(mx)*1.3);
            ui->spinAxesMin->setValue(int(mn)*0.7);
            if (green_idx_ == -1) {
                green_idx_ = 0;
            }
            return;
        }
    }

    ui->main_scrollLED1->setValue(LED[0].bright);
    ui->main_scrollLED2->setValue(LED[1].bright);
    ui->main_scrollLED3->setValue(LED[2].bright);
    ui->verticalSlider->setValue(-Offset.LED1);
    ui->verticalSlider_2->setValue(-Offset.LED2);
    ui->verticalSlider_3->setValue(-Offset.LED3);
    idx_ = 0;
}

void MainWindow::on_pushButton_ResetVisible_clicked()
{
    for(int i=0; i<ui->plot->graphCount(); i++)
    {
        ui->plot->graph(i)->setVisible(true);
        ui->listWidget_Channels->item(i)->setBackground(Qt::NoBrush);
    }
}

void MainWindow::on_listWidget_Channels_itemDoubleClicked(QListWidgetItem *item)
{
    int graphIdx = ui->listWidget_Channels->currentRow();

    if(ui->plot->graph(graphIdx)->visible())
    {
        ui->plot->graph(graphIdx)->setVisible(false);
        item->setBackground(Qt::black);
    }
    else
    {
        ui->plot->graph(graphIdx)->setVisible(true);
        item->setBackground(Qt::NoBrush);
    }
    ui->plot->replot();
}

void MainWindow::on_pushButton_clicked()
{
    ui->comboPort->clear();
    /* List all available serial ports and populate ports combo box */
    for (QSerialPortInfo port : QSerialPortInfo::availablePorts())
    {
        ui->comboPort->addItem (port.portName());
    }
}

void MainWindow::on_actionSettings_triggered()
{
    settingsWindow = new AdvancedSettings (this);
    connect(settingsWindow, &AdvancedSettings::settingsUpdated,
            this, &MainWindow::updateFromSettings);
    settingsWindow->setWindowTitle ("AFE4404 configuration");
    settingsWindow->show();

}

void MainWindow::onSendDataRequested()
{
}

QByteArray MainWindow::makeArray(quint32 raw) //LSB first
{
    QByteArray packet;
    for (int i=0; i<3; i++)
    {
        packet.append(static_cast<char>(raw & 0xFF));
        raw = raw >> 8;
    }
    return packet;
}


void MainWindow::on_actionSend_triggered()
{
    if (!isBleConnected()) {
        ui->statusBar->showMessage("BLE: not connected — cannot send configuration.");
        return;
    }

    ui->statusBar->showMessage("BLE: sending configuration...");

    auto sendCmd = [&](uint8_t addr, uint32_t val) {
        QByteArray frame;
        frame.append(static_cast<char>(addr));
        frame.append(makeArray(val));
        qDebug() << "FRAME: " << addr << " " << val;
        bleWrite(frame);
        QThread::msleep(10);
    };

    sendCmd(0x00,0x08); //0
    sendCmd(0x23,1<<9); //1
    sendCmd(0x1E,(quint32)ADC.NUMAV | (quint32)1<<8); //2
        qDebug() << "ADC.NUMAV:" << ADC.NUMAV;
    quint32 tempDIV=LEDfeatures.DIV_PRF;
        if (tempDIV > 0)
        tempDIV += 3;
    sendCmd(0x39,tempDIV); //3
        qDebug() << "LEDfeatures.DIV_PRF:" << LEDfeatures.DIV_PRF;
        qDebug() << "tempDIV:" << tempDIV;
    sendCmd(0x1D,LEDfeatures.PRF); //4
    sendCmd(0x09,LED[1].LEDSTC); //5
    qDebug() << "LED[1].LEDSTC" << LED[1].LEDSTC;
    sendCmd(0x0A,LED[1].LEDENDC); //6
    qDebug() << "LED[1].LEDENDC" << LED[1].LEDENDC;
    sendCmd(0x01,LED[1].STC); //7
    qDebug() << "LED[1].STC:" << LED[1].STC;
    sendCmd(0x02,LED[1].ENDC); //8
    qDebug() << "LED[1].ENDC:" << LED[1].ENDC;
    sendCmd(0x15,ADC.STC0); //9
    sendCmd(0x16,ADC.END0); //10
    sendCmd(0x0D,LED[1].CONVST); //11
    qDebug() << "LED[1].CONVST" << LED[1].CONVST;
    sendCmd(0x0E,LED[1].CONVEND); //12
    qDebug() << "LED[1].CONVEND" << LED[1].CONVEND;
    if (LED[2].enabled)
    {
        sendCmd(0x36,LED[2].LEDSTC); //13
        qDebug() << "LED[2].LEDSTC" << LED[2].LEDSTC;
        sendCmd(0x37,LED[2].LEDENDC); //14
    }
    else
    {
        sendCmd(0x36,0); //13
        sendCmd(0x37,0); //14
    }
    if (!LED[2].enabled) //send amb2 or led3
    {
        sendCmd(0x05, AMB[1].STC); //15
        qDebug() << "AMB[1].STC" << AMB[1].STC;
        sendCmd(0x06, AMB[1].ENDC); //16
        qDebug() << "AMB[1].STC" << AMB[1].ENDC;
    }
    else
    {
        sendCmd(0x05, LED[2].STC); //15
        qDebug() << "LED[2].STC" << LED[2].STC;
        sendCmd(0x06, LED[2].ENDC); //16
        qDebug() << "LED[2].ENDC" << LED[2].ENDC;
    }
    sendCmd(0x17,ADC.STC1); //17
    sendCmd(0x18,ADC.END1); //18
    if (!LED[2].enabled) //send amb2 or led3
    {
        sendCmd(0x0F,AMB[1].CONVST); //19
        sendCmd(0x10,AMB[1].CONVEND); //20
    }
    else
    {
        sendCmd(0x0F,LED[2].CONVST); //19
        sendCmd(0x10,LED[2].CONVEND); //20
    }
    sendCmd(0x03,LED[0].LEDSTC); //21
    qDebug() << "LED[0].LEDSTC:" << LED[0].LEDSTC;
    sendCmd(0x04,LED[0].LEDENDC); //22
    qDebug() << "LED[0].LEDENDC:" << LED[0].LEDENDC;
    sendCmd(0x07,LED[0].STC); //23
    qDebug() << "LED[0].STC" << LED[0].STC;
    sendCmd(0x08,LED[0].ENDC); //24
    qDebug() << "LED[0].ENDC" << LED[0].ENDC;
    sendCmd(0x19,ADC.STC2); //25
    sendCmd(0x1A,ADC.END2); //26
    sendCmd(0x11,LED[0].CONVST); //27
    sendCmd(0x12,LED[0].CONVEND); //28
    sendCmd(0x0B,AMB[0].STC); //29
    qDebug() << "AMB[0].STC" << AMB[0].STC;
    sendCmd(0x0C,AMB[0].ENDC); //30
    qDebug() << "AMB[0].ENDC" << AMB[0].ENDC;
    sendCmd(0x1B,ADC.STC3); //31
    sendCmd(0x1C,ADC.END3); //32
    sendCmd(0x13,AMB[0].CONVST); //33
    sendCmd(0x14,AMB[0].CONVEND); //34
    sendCmd(0x32,LEDfeatures.PDNSTC); //35
    sendCmd(0x33,LEDfeatures.PDNENDC); //36
    qDebug() << "LEDfeatures.PDNENDC" << LEDfeatures.PDNENDC;
    quint32 temP=0;
    if (LED[0].enabled)
        temP |= (quint32)LED[0].bright;
    if (LED[1].enabled)
        temP |= (quint32)LED[1].bright<<6;
    if (LED[2].enabled)
        temP |= (quint32)LED[2].bright<<12;
    sendCmd(0x22,temP); //37
    qDebug() << "LED[0].bright" << LED[0].bright;
    qDebug() << "LED[1].bright" << LED[1].bright;
    qDebug() << "LED[2].bright" << LED[2].bright;
    sendCmd(0x21,(quint32)LEDfeatures.CF <<3 | (quint32)LEDfeatures.GAIN); //38
    if (LEDfeatures.SepGain)
        sendCmd(0x20, (quint32)1<<15 |(quint32)LEDfeatures.CF_SEP <<3 | (quint32)LEDfeatures.GAIN_SEP); //39
    else
        sendCmd(0x20, 0);//LEDfeatures.CF_SEP <<3 | LEDfeatures.GAIN_SEP); //39

    if (ADC.decEn)
        sendCmd(0x3D, (quint32)1 << 5 | (quint32)ADC.decFactor << 1); //40
    else
        sendCmd(0x3D, 0); //40
    sendCmd(0x3A, full_offset()); //41
    ui->statusBar->showMessage("BLE: configuration sent!");
}
void MainWindow::updateFromSettings()
{
    // например:
    ui->main_boxLED1->setEnabled(LED[0].enabled);
    ui->main_boxLED2->setEnabled(LED[1].enabled);
    ui->main_boxLED3->setEnabled(LED[2].enabled);

    ui->main_scrollLED1->setValue(LED[0].bright);
    ui->main_scrollLED2->setValue(LED[1].bright);
    ui->main_scrollLED3->setValue(LED[2].bright);


    ui->main_labelLED1->setText(convertBrightness(LED[0].bright));
    ui->main_labelLED2->setText(convertBrightness(LED[1].bright));
    ui->main_labelLED3->setText(convertBrightness(LED[2].bright));

    ui->main_checkSEP->setChecked(LEDfeatures.SepGain);
    ui->main_Rf1->setCurrentIndex(LEDfeatures.GAIN);
    ui->main_Cf1->setCurrentIndex(LEDfeatures.CF);
    ui->main_boxGAIN2->setEnabled(LEDfeatures.SepGain);
    if (LEDfeatures.SepGain)
    {
        ui->main_boxGAIN2->setEnabled(true);
        ui->main_Rf2->setCurrentIndex(LEDfeatures.GAIN_SEP);
        ui->main_Cf2->setCurrentIndex(LEDfeatures.CF_SEP);
    }
    qDebug() << "Main window updated from Advanced Settings";
}

QString MainWindow::convertBrightness(int val)
{
    int f = val*8/10;
    int g = (val*8)%10;
    QString txt;
    if (val != 63)
        txt=QString::number(f)+"."+QString::number(g);
    else
        txt=QString::number(f);
    txt += " mA";
    return txt;
}

void MainWindow::on_main_scrollLED1_valueChanged(int value)
{
    LED[0].bright = value;
    qDebug() << "LED[0].bright: " << LED[0].bright;
    quint32 temP=0;
    if (LED[0].enabled)
        temP |= (quint32)LED[0].bright;
    if (LED[1].enabled)
        temP |= (quint32)LED[1].bright<<6;
    if (LED[2].enabled)
        temP |= (quint32)LED[2].bright<<12;
    if (isBleConnected())
    {
        send_from_main(0x22, temP);
        qDebug() << "LED1 brightness changed ";
    }
    else
    {
        qDebug() << "LED1 brightness changed, but COM port is closed — skipping send.";
    }
    ui->main_labelLED1->setText(convertBrightness(LED[0].bright));
}

void MainWindow::send_from_main(uint8_t addr, uint32_t val)
{
    QByteArray frame;
    frame.append(static_cast<char>(0x44));
    frame.append(static_cast<char>(addr));
    frame.append(makeArray(val));
    qDebug() << frame;
    bleWrite(frame);
}


void MainWindow::on_main_scrollLED2_valueChanged(int value)
{
    LED[1].bright = value;
    qDebug() << "LED[1].bright: " << LED[1].bright;
    quint32 temP=0;
    if (LED[0].enabled)
        temP |= (quint32)LED[0].bright;
    if (LED[1].enabled)
        temP |= (quint32)LED[1].bright<<6;
    if (LED[2].enabled)
        temP |= (quint32)LED[2].bright<<12;
    if (isBleConnected())
    {
        send_from_main(0x22, temP);
        qDebug() << "LED2 brightness changed ";
    }
    else
    {
        qDebug() << "LED2 brightness changed, but COM port is closed — skipping send.";
    }
    ui->main_labelLED2->setText(convertBrightness(LED[1].bright));
}


void MainWindow::on_main_scrollLED3_valueChanged(int value)
{
    LED[2].bright = value;
    qDebug() << "LED[2].bright: " << LED[2].bright;
    quint32 temP=0;
    if (LED[0].enabled)
        temP |= (quint32)LED[0].bright;
    if (LED[1].enabled)
        temP |= (quint32)LED[1].bright<<6;
    if (LED[2].enabled)
        temP |= (quint32)LED[2].bright<<12;
    //if (serialPort->open(QIODevice::ReadWrite))
        //send_from_main(0x22, temP);
    if (isBleConnected())
    {
        send_from_main(0x22, temP);
        qDebug() << "LED3 brightness changed ";
    }
    else
    {
        qDebug() << "LED3 brightness changed, but COM port is closed — skipping send.";
    }
    ui->main_labelLED3->setText(convertBrightness(LED[2].bright));
}


void MainWindow::on_main_checkSEP_checkStateChanged(const Qt::CheckState &arg1)
{
    bool enabled = (arg1 == Qt::Checked);
    LEDfeatures.SepGain = enabled;
    qDebug()<<enabled;
    ui->main_boxGAIN2->setEnabled(enabled);
    ui->main_Cf2->setCurrentIndex(ui->main_Cf1->currentIndex());
    ui->main_Rf2->setCurrentIndex(ui->main_Rf1->currentIndex());
    ui->main_Rf2->setEnabled(enabled);
    ui->main_Cf2->setEnabled(enabled);
    if (LEDfeatures.SepGain)
    {
        if (isBleConnected())
        {
            send_from_main(0x20, 1<<15 |(quint32)LEDfeatures.CF_SEP <<3 | (quint32)LEDfeatures.GAIN_SEP);
            qDebug() << "SEP=enabled: send 0x20, 1<<15 | " << LEDfeatures.CF_SEP <<" <<3 | " << LEDfeatures.GAIN_SEP;
        }
        else
        {
            qDebug() << "SEP=enabled, but COM port is closed — skipping send.";
        }
    }
    else
    {
        //send_from_main(0x20, 0);
        if (isBleConnected())
        {
            send_from_main(0x20, 0);
            qDebug() << "SEP=disabled: send 0x20, 0 ";
        }
        else
        {
            qDebug() << "SEP=disabled, but COM port is closed — skipping send.";
        }
    }
}


void MainWindow::on_main_Rf1_currentIndexChanged(int index)
{
    LEDfeatures.GAIN = index;
    if (ui->main_checkSEP->checkState() == false)
    {
        ui->main_Rf2->setCurrentIndex(ui->main_Rf1->currentIndex());
    }
    if (isBleConnected())
    {
        send_from_main(0x21, ((quint32)LEDfeatures.CF << 3) | (quint32)LEDfeatures.GAIN);
        qDebug() << "RF1 changed: send 0x21, " << LEDfeatures.CF <<" <<3 | " << LEDfeatures.GAIN;
    }
    else
    {
        qDebug() << "RF1 changed, but COM port is closed — skipping send.";
    }
}


void MainWindow::on_main_Cf1_currentIndexChanged(int index)
{
    LEDfeatures.CF = index;
    if (ui->main_checkSEP->checkState() == false)
    {
        ui->main_Cf2->setCurrentIndex(ui->main_Cf1->currentIndex());
    }
    if (isBleConnected())
    {
        send_from_main(0x21, ((quint32)LEDfeatures.CF << 3) | (quint32)LEDfeatures.GAIN);
        qDebug() << "CF1 changed: send 0x21, " << LEDfeatures.CF <<" <<3 | " << LEDfeatures.GAIN;
    }
    else
    {
        qDebug() << "CF1 changed, but COM port is closed — skipping send.";
    }
}


void MainWindow::on_main_Rf2_currentIndexChanged(int index) //вызывается при 
{
    LEDfeatures.GAIN_SEP = index;
    if (LEDfeatures.SepGain)
    {
        if (isBleConnected())
        {
            send_from_main(0x20, 1<<15 |(quint32)LEDfeatures.CF_SEP <<3 | (quint32)LEDfeatures.GAIN_SEP);
            qDebug() << "RF2 changed, SEP=enabled: send 0x20, 1<<15 | " << LEDfeatures.CF_SEP <<" <<3 | " << LEDfeatures.GAIN_SEP;
        }
        else
        {
            qDebug() << "RF2 changed, SEP=enabled, but COM port is closed — skipping send.";
        }
    }
    else
    {
        //send_from_main(0x20, 0);
        if (isBleConnected())
        {
            send_from_main(0x20, 0);
            qDebug() << "RF2 changed, SEP=disabled: send 0x20, 0 ";
        }
        else
        {
            qDebug() << "RF2 changed, SEP=disabled, but COM port is closed — skipping send.";
        }
    }
}


void MainWindow::on_main_Cf2_currentIndexChanged(int index)
{
    LEDfeatures.CF_SEP = index;
    if (LEDfeatures.SepGain)
    {
        if (isBleConnected())
        {
            send_from_main(0x20, 1<<15 |(quint32)LEDfeatures.CF_SEP <<3 | (quint32)LEDfeatures.GAIN_SEP);
            qDebug() << "CF2 changed, SEP=enabled: send 0x20, 1<<15 | " << LEDfeatures.CF_SEP <<" <<3 | " << LEDfeatures.GAIN_SEP;
        }
        else
        {
            qDebug() << "CF2 changed, SEP=enabled, but COM port is closed — skipping send.";
        }
    }
    else
    {
        //send_from_main(0x20, 0);
        if (isBleConnected())
        {
            send_from_main(0x20, 0);
            qDebug() << "CF2 changed, SEP=disabled: send 0x20, 0 ";
        }
        else
        {
            qDebug() << "CF2 changed, SEP=disabled, but COM port is closed — skipping send.";
        }
    }
}

quint32 MainWindow::full_offset()
{
    quint32 offset;
    offset = (quint32)Offset.POL1 << 9;
    offset |= (quint32)Offset.LED1 << 5;
    offset |= (quint32)Offset.POL2 << 19;
    offset |= (quint32)Offset.LED2 << 15;
    offset |= (quint32)Offset.POL3 << 4;
    offset |= (quint32)Offset.LED3;
    offset |= (quint32)Offset.POL4 << 14;
    offset |= (quint32)Offset.AMB1 << 10;
    return offset;
}

void MainWindow::on_verticalSlider_valueChanged(int value)
{
    ui->offset_label1->setText(QString::number(value));
    if (value >= 0)
        Offset.POL1 = 0;
    else
        Offset.POL1 = 1;
    Offset.LED1 = abs(value);
    qDebug() << "POL_1: " << Offset.POL1;
    qDebug() << "LED_1: " << Offset.LED1;
    if (isBleConnected())
    {
        send_from_main(0x3A, full_offset());
        qDebug() << "Offset1 changed: send 0x20, " << full_offset();
    }
    else
    {
        qDebug() << "Offset1 changed, but COM port is closed — skipping send.";
    }
}


void MainWindow::on_verticalSlider_2_valueChanged(int value)
{
    ui->offset_label2->setText(QString::number(value));
    if (value >= 0)
        Offset.POL2 = 0;
    else
        Offset.POL2 = 1;
    Offset.LED2 = abs(value);
    qDebug() << "POL_2: " << Offset.POL2;
    qDebug() << "LED_2: " << Offset.LED2;
    if (isBleConnected())
    {
        send_from_main(0x3A, full_offset());
        qDebug() << "Offset2 changed: send 0x20, " << full_offset();
    }
    else
    {
        qDebug() << "Offset2 changed, but COM port is closed — skipping send.";
    }
}


void MainWindow::on_verticalSlider_3_valueChanged(int value)
{
    ui->offset_label3->setText(QString::number(value));
    if (value >= 0)
        Offset.POL3 = 0;
    else
        Offset.POL3 = 1;
    Offset.LED3 = abs(value);
    qDebug() << "POL_3: " << Offset.POL3;
    qDebug() << "LED_3: " << Offset.LED3;
    if (isBleConnected())
    {
        send_from_main(0x3A, full_offset());
        qDebug() << "Offset3 changed: send 0x20, " << full_offset();
    }
    else
    {
        qDebug() << "Offset3 changed, but COM port is closed — skipping send.";
    }
}


void MainWindow::on_verticalSlider_4_valueChanged(int value)
{
    ui->offset_label4->setText(QString::number(value));
    if (value >= 0)
        Offset.POL4 = 0;
    else
        Offset.POL4 = 1;
    Offset.AMB1 = abs(value);
    qDebug() << "POL_4: " << Offset.POL1;
    qDebug() << "AMB_1: " << Offset.AMB1;
    if (isBleConnected())
    {
        send_from_main(0x3A, full_offset());
        qDebug() << "Offset_AMB changed: send 0x20, " << full_offset();
    }
    else
    {
        qDebug() << "Offset_AMB changed, but COM port is closed — skipping send.";
    }
}


void MainWindow::on_HardReset_button_clicked()
{
    if (!isBleConnected()) return;

    QByteArray frame;
    frame.append(static_cast<char>(0x48));
    bleWrite(frame);

    ui->main_boxGAIN1->setEnabled(false);
    ui->main_boxGAIN2->setEnabled(false);
    ui->main_boxLED1->setEnabled(false);
    ui->main_boxLED2->setEnabled(false);
    ui->main_boxLED3->setEnabled(false);
    ui->main_Rf1->setCurrentIndex(0);
    ui->main_Cf1->setCurrentIndex(0);
    ui->main_Rf2->setCurrentIndex(0);
    ui->main_Cf2->setCurrentIndex(0);
    ui->main_checkSEP->setChecked(false);
    ui->main_scrollLED1->setValue(0);
    ui->main_scrollLED2->setValue(0);
    ui->main_scrollLED3->setValue(0);
    ui->verticalSlider->setValue(0);
    ui->verticalSlider_2->setValue(0);
    ui->verticalSlider_3->setValue(0);
    ui->verticalSlider_4->setValue(0);
    for (int i=0; i<3; i++)
    {
        LED[i].bright = 0;
        LED[i].STC = 0;
        LED[i].ENDC = 0;
        LED[i].LEDSTC = 0;
        LED[i].LEDENDC = 0;
        LED[i].CONVST = 0;
        LED[i].CONVEND = 0;
    }
    ADC.decEn = false;
    ADC.decFactor = 0;
    ADC.NUMAV = 0;
    ADC.END0 = 0;
    ADC.END1 = 0;
    ADC.END2 = 0;
    ADC.END3 = 0;
    ADC.STC0 = 0;
    ADC.STC1 = 0;
    ADC.STC2 = 0;
    ADC.STC3 = 0;
    LEDfeatures.CF = 0;
    LEDfeatures.CF_SEP = false;
    LEDfeatures.DIV_PRF = 0;
    LEDfeatures.GAIN = 0;
    LEDfeatures.GAIN_SEP = 0;
    LEDfeatures.PDNSTC = 0;
    LEDfeatures.PDNENDC = 0;
    LEDfeatures.PRF = 0;
    for (int i=0; i<2; i++)
    {
        AMB[i].STC = 0;
        AMB[i].ENDC = 0;
        AMB[i].CONVST = 0;
        AMB[i].CONVEND = 0;
        AMB[i].enabled = false;
    }
    Offset.LED1 = 0;
    Offset.LED2 = 0;
    Offset.LED3 = 0;
    Offset.AMB1 = 0;
    Offset.POL1 = 0;
    Offset.POL2 = 0;
    Offset.POL3 = 0;
    Offset.POL4 = 0;
}

