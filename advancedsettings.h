#ifndef ADVANCEDSETTINGS_H
#define ADVANCEDSETTINGS_H

#include <QDialog>
#include <QTabWidget>
#include "LEDManager.h"
//#include "ui_advancedsettings.h"

//QT_BEGIN_NAMESPACE

namespace Ui {
class AdvancedSettings;
}

//QT_END_NAMESPACE
class AdvancedSettings : public QDialog
{
    Q_OBJECT
protected:
    void showEvent(QShowEvent *event) override;   // ← срабатывает при открытии окна
    void closeEvent(QCloseEvent *event) override; // ← срабатывает при закрытии окна
private slots:
    //void apply();
    void on_buttonBox_accepted();

    void on_buttonBox_rejected();

    //void on_buttonBox_clicked(QAbstractButton *button);

    void on_Rf1_comboBox_currentTextChanged(const QString &arg1);

    void on_Cf1_comboBox_currentTextChanged(const QString &arg1);

    void on_Separate_checkBox_checkStateChanged(const Qt::CheckState &arg1);

    void on_checkBox_checkStateChanged(const Qt::CheckState &arg1);

    void on_checkBox_2_checkStateChanged(const Qt::CheckState &arg1);

    void on_checkBox_3_checkStateChanged(const Qt::CheckState &arg1);

    void on_bright1_valueChanged(int value);

    void on_label_22_linkActivated(const QString &link);

    void on_bright2_valueChanged(int value);

    void on_bright3_valueChanged(int value);

    void on_pushButton_3_clicked(bool enabled);

    void on_pushButton_2_clicked(bool enabled);

    void on_pushButton_clicked(bool enabled);

    void onAnyValueChanged();
    void on_checkBox_Dec_checkStateChanged(const Qt::CheckState &arg1);

    void on_multiplex_valueChanged(int value);

public slots:

signals:
    void sendDataRequested();
    void settingsUpdated();
public:
    explicit AdvancedSettings(QWidget *parent = nullptr);
    ~AdvancedSettings();

private:
    void fillInfo();
    void saveAdvGUI();
    void loadAdvGUI();
    void setupGUI();
    int* getTimings(int duration);
    void swapTabs(QTabWidget *tabs);
    bool dataChanged = false;
    bool checkPassed = false;
    Ui::AdvancedSettings *m_ui; // Правильное объявление
};

#endif // ADVANCEDSETTINGS_H
