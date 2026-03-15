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

#ifndef MAINWINDOW_HPP
#define MAINWINDOW_HPP

#include <QMainWindow>
#include <QtSerialPort/QtSerialPort>
#include <QSerialPortInfo>
#include "helpwindow.hpp"
#include "advancedsettings.h"
#include "qcustomplot/qcustomplot.h"
#include <QTimer>
#include <QDateTime>

#define START_MSG       '$'
#define END_MSG         ';'

#define WAIT_START      1
#define IN_MESSAGE      2
#define UNDEFINED       3

#define CUSTOM_LINE_COLORS   14
#define GCP_CUSTOM_LINE_COLORS 4

namespace Ui {
    class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();
    void calculate(double value1, double value2, double value3);

private slots:
    void on_comboPort_currentIndexChanged(const QString &arg1);                           // Slot displays message on status bar
    void portOpenedSuccess();                                                             // Called when port opens OK
    void portOpenedFail();                                                                // Called when port fails to open
    void onPortClosed();                                                                  // Called when closing the port
    void replot();                                                                        // Slot for repainting the plot
    void onNewDataArrived(QStringList newData);                                           // Slot for new data from serial port
    void saveStream(QStringList newData);                                                 // Save the received data to the opened file
    void on_spinAxesMin_valueChanged(int arg1);                                           // Changing lower limit for the plot
    void on_spinAxesMax_valueChanged(int arg1);                                           // Changing upper limit for the plot
    void readData();                                                                      // Slot for inside serial port
    //void on_comboAxes_currentIndexChanged(int index);                                     // Display number of axes and colors in status bar
    double convert_to_power(double raw, int ind);

    void on_spinYStep_valueChanged(int arg1);                                             // Spin box for changing Y axis tick step
    void on_savePNGButton_clicked();                                                      // Button for saving JPG
    void onMouseMoveInPlot (QMouseEvent *event);                                          // Displays coordinates of mouse pointer when clicked in plot in status bar
    void on_spinPoints_valueChanged (int arg1);                                           // Spin box controls how many data points are collected and displayed
    void on_mouse_wheel_in_plot (QWheelEvent *event);                                  // Makes wheel mouse works while plotting
    void onTimeout();
    /* Used when a channel is selected (plot or legend) */
    void channel_selection (void);
    void legend_double_click (QCPLegend *legend, QCPAbstractLegendItem *item, QMouseEvent *event);

    void on_actionConnect_triggered();
    void on_actionDisconnect_triggered();
    void on_actionHow_to_use_triggered();
    void on_actionPause_Plot_triggered();
    void on_actionClear_triggered();
    void on_actionRecord_stream_triggered();

    void on_pushButton_TextEditHide_clicked();

    void on_pushButton_ShowallData_clicked();

    void on_pushButton_AutoScale_clicked();

    void on_pushButton_ResetVisible_clicked();

    void on_listWidget_Channels_itemDoubleClicked(QListWidgetItem *item);

    void on_pushButton_clicked();

    void on_actionSettings_triggered();


    void on_actionSend_triggered();
    void updateFromSettings();
    void on_main_scrollLED1_valueChanged(int value);

    void on_main_scrollLED2_valueChanged(int value);

    void on_main_scrollLED3_valueChanged(int value);
    void send_from_main(uint8_t addr, uint32_t val);
    QString convertBrightness(int val);
    quint32 full_offset();
    void on_main_checkSEP_checkStateChanged(const Qt::CheckState &arg1);

    void on_main_Rf1_currentIndexChanged(int index);

    void on_main_Cf1_currentIndexChanged(int index);

    void on_main_Rf2_currentIndexChanged(int index);

    void on_main_Cf2_currentIndexChanged(int index);

    void on_verticalSlider_valueChanged(int value);

    void on_verticalSlider_2_valueChanged(int value);

    void on_verticalSlider_3_valueChanged(int value);

    void on_verticalSlider_4_valueChanged(int value);

    void on_HardReset_button_clicked();

signals:
    void portOpenFail();                                                                  // Emitted when cannot open port
    void portOpenOK();                                                                    // Emitted when port is open
    void portClosed();                                                                    // Emitted when port is closed
    void newData(QStringList data);                                                       // Emitted when new data has arrived

private:
    Ui::MainWindow *ui;

    /* Line colors */
    QColor line_colors[CUSTOM_LINE_COLORS];
    QColor gui_colors[GCP_CUSTOM_LINE_COLORS];

    /* Main info */
    bool connected;                                                                       // Status connection variable
    bool plotting;                                                                        // Status plotting variable
    int dataPointNumber;                                                                  // Keep track of data points
    /* Channels of data (number of graphs) */
    int channels;

    /* Data format */
    int data_format;   

    /* Textbox Related */
    bool filterDisplayedData = true;

    /* Listview Related */
    QStringListModel *channelListModel;
    QStringList     channelStrList;

    //-- CSV file to save data
    QFile* m_csvFile = nullptr;
    void openCsvFile(void);
    void closeCsvFile(void);
    void onSendDataRequested(QSerialPort *serialPort);
    QByteArray makeArray(quint32 raw);

    QTimer updateTimer;                                                                   // Timer used for replotting the plot
    QTime timeOfFirstData;                                                                // Record the time of the first data point
    double timeBetweenSamples;                                                            // Store time between samples
    QSerialPort *serialPort;                                                              // Serial port; runs in this thread
    QString receivedData;                                                                 // Used for reading from the port
    QByteArray receivedBytes;
    QByteArray rxBuffer; // накопительный буфер для бинарного потока
    int STATE;                                                                            // State of recieiving message from port
    int NUMBER_OF_POINTS;                                                                 // Number of points plotted
    HelpWindow *helpWindow;
    AdvancedSettings *settingsWindow;

    void createUI();                                                                      // Populate the controls
    void enable_com_controls (bool enable);                                               // Enable/disable controls
    void setupPlot();                                                                     // Setup the QCustomPlot
                                                                                          // Open the inside serial port with these parameters
    void openPort(QSerialPortInfo portInfo, int baudRate, QSerialPort::DataBits dataBits, QSerialPort::Parity parity, QSerialPort::StopBits stopBits);
    bool busy = false;
    int need_calculate_number = 0;
    int idx_ = 0;
    QTimer *timer;
    int val_timer = 0;
    qint64 last_data_time;
    std::vector<std::vector<double>> data_vector;

    std::vector<int> green_samples;
    int green_idx_ = -1;
};


#endif                                                                                    // MAINWINDOW_HPP
