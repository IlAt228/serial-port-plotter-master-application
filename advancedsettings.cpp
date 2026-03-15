#include "advancedsettings.h"
#include "ui_advancedsettings.h"
#include "LEDManager.h"

#include <QIntValidator>
#include <QLineEdit>
#include <QSerialPortInfo>
#include <QMessageBox>
#include <QCloseEvent>
AdvancedSettings::AdvancedSettings(QWidget *parent) :
    QDialog(parent),
    m_ui(new Ui::AdvancedSettings)
{
    m_ui->setupUi(this);
    setWindowTitle("Advanced Settings");
    fillInfo();
    loadAdvGUI();
    setupGUI();
    // Получаем кнопки
    //applyButton = m_ui->buttonBox->button(QDialogButtonBox::Apply);
    //cancelButton = m_ui->buttonBox->button(QDialogButtonBox::Cancel);

    // Изначально Apply недоступна
    m_ui->buttonBox->button(QDialogButtonBox::Apply)->setEnabled(false);

    // Убираем авто-срабатывание по Enter
    m_ui->buttonBox->button(QDialogButtonBox::Apply)->setAutoDefault(false);
    m_ui->buttonBox->button(QDialogButtonBox::Apply)->setDefault(false);
    /*connect(m_ui->buttonBox, &QDialogButtonBox::accepted,
            this, &AdvancedSettings::apply);
    connect(m_ui->buttonBox, &QDialogButtonBox::rejected,
            this, &AdvancedSettings::reject); */
    connect(m_ui->buttonBox->button(QDialogButtonBox::Apply),
            &QPushButton::clicked,
            this,
            &AdvancedSettings::on_buttonBox_accepted);
    const QList<QWidget*> widgets = findChildren<QWidget*>();
    for (QWidget *w : widgets)
    {
        // QSpinBox, QDoubleSpinBox
        if (auto spin = qobject_cast<QAbstractSpinBox*>(w))
            connect(spin, SIGNAL(editingFinished()), this, SLOT(onAnyValueChanged()));

        // QSlider
        else if (auto slider = qobject_cast<QSlider*>(w))
            connect(slider, SIGNAL(valueChanged(int)), this, SLOT(onAnyValueChanged()));

        // QLineEdit
        else if (auto edit = qobject_cast<QLineEdit*>(w))
            connect(edit, SIGNAL(textChanged(QString)), this, SLOT(onAnyValueChanged()));

        // QCheckBox
        else if (auto cb = qobject_cast<QCheckBox*>(w))
            connect(cb, SIGNAL(stateChanged(int)), this, SLOT(onAnyValueChanged()));

        // QComboBox
        else if (auto combo = qobject_cast<QComboBox*>(w))
            connect(combo, SIGNAL(currentIndexChanged(int)), this, SLOT(onAnyValueChanged()));
    }
}

AdvancedSettings::~AdvancedSettings()
{
    delete m_ui;
}
void AdvancedSettings::showEvent(QShowEvent *event)
{
    QDialog::showEvent(event);
    loadAdvGUI(); // обновляем UI при каждом открытии окна
}
void AdvancedSettings::closeEvent(QCloseEvent *event)
{
    if (dataChanged) {
        // Показываем диалог подтверждения
        QMessageBox::StandardButton reply;
        reply = QMessageBox::question(this, "Подтверждение",
                                      "Есть несохранённые изменения. Закрыть окно без сохранения?",
                                      QMessageBox::Yes | QMessageBox::No);
        if (reply == QMessageBox::Yes) {
            event->accept(); // Закрываем окно
        } else {
            event->ignore(); // Не закрываем окно
        }
    } else {
        event->accept(); // Если изменений нет, просто закрываем
        saveAdvGUI();
    }


    //QDialog::closeEvent(event);
}
void AdvancedSettings::setupGUI()
{
    m_ui->label_22->setEnabled(false);
    m_ui->label_23->setEnabled(false);
    m_ui->label_24->setEnabled(false);
    m_ui->label_29->setEnabled(false);
    m_ui->label_32->setEnabled(false);
    m_ui->label_37->setEnabled(false);
    m_ui->label_42->setEnabled(false);
    m_ui->label_43->setEnabled(false);
    m_ui->label_44->setEnabled(false);
    m_ui->bright1->setEnabled(false);
    m_ui->bright2->setEnabled(false);
    m_ui->bright3->setEnabled(false);
    m_ui->LED1_dur->setEnabled(false);
    m_ui->LED2_dur->setEnabled(false);
    m_ui->LED3_dur->setEnabled(false);
    m_ui->decim_comboBox->setEnabled(false);
    m_ui->tab_1->setEnabled(false);
    m_ui->tab_2->setEnabled(false);
    m_ui->tab_3->setEnabled(false);
    m_ui->tab_4->setEnabled(false);
    m_ui->tab_5->setEnabled(false);
    loadAdvGUI();
}
void AdvancedSettings::saveAdvGUI()
{
    //LED1
    LED[0].enabled = m_ui->checkBox->isChecked();
    LED[0].LEDSTC = m_ui->spinBox_L1onStart->value();
    LED[0].LEDENDC = m_ui->spinBox_L1onEnd->value();
    LED[0].STC = m_ui->spinBox_L1sampStart->value();
    LED[0].ENDC = m_ui->spinBox_L1sampEnd->value();
    LED[0].CONVST = m_ui->spinBox_L1cpStart->value();
    LED[0].CONVEND = m_ui->spinBox_L1cpEnd->value();
    LED[0].bright = m_ui->bright1->value();
    //LED2
    LED[1].enabled = m_ui->checkBox_2->isChecked();
    LED[1].LEDSTC = m_ui->spinBox_L2onStart->value();
    LED[1].LEDENDC = m_ui->spinBox_L2onEnd->value();
    LED[1].STC = m_ui->spinBox_L2sampStart->value();
    LED[1].ENDC = m_ui->spinBox_L2sampEnd->value();
    LED[1].CONVST = m_ui->spinBox_L2cpStart->value();
    LED[1].CONVEND = m_ui->spinBox_L2cpEnd->value();
    LED[1].bright = m_ui->bright2->value();
    //LED3
    LED[2].enabled = m_ui->checkBox_3->isChecked();
    LED[2].LEDSTC = m_ui->spinBox_L3onStart->value();
    LED[2].LEDENDC = m_ui->spinBox_L3onEnd->value();
    LED[2].STC = m_ui->spinBox_L3sampStart->value();
    LED[2].ENDC = m_ui->spinBox_L3sampEnd->value();
    LED[2].CONVST = m_ui->spinBox_L3cpStart->value();
    LED[2].CONVEND = m_ui->spinBox_L3cpEnd->value();
    LED[2].bright = m_ui->bright3->value();
    //Ambient 1
    AMB[0].enabled = m_ui->tab_4->isEnabled();
    AMB[0].STC = m_ui->spinBox_L1sampStart_2->value();
    AMB[0].ENDC = m_ui->spinBox_L1sampEnd_2->value();
    AMB[0].CONVST = m_ui->spinBox_L1cpStart_2->value();
    AMB[0].CONVEND = m_ui->spinBox_L1cpEnd_2->value();
    //Ambient 2
    AMB[1].enabled = m_ui->tab_5->isEnabled();
    AMB[1].STC = m_ui->spinBox_L1sampStart_3->value();
    AMB[1].ENDC = m_ui->spinBox_L1sampEnd_3->value();
    AMB[1].CONVST = m_ui->spinBox_L1cpStart_3->value();
    AMB[1].CONVEND = m_ui->spinBox_L1cpEnd_3->value();

    ADC.STC0 = m_ui->ADC0start->value();
    ADC.END0 = m_ui->ADC0end->value();
    ADC.STC1 = m_ui->ADC1start->value();
    ADC.END1 = m_ui->ADC1end->value();
    ADC.STC2 = m_ui->ADC2start->value();
    ADC.END2 = m_ui->ADC2end->value();
    ADC.STC3 = m_ui->ADC3start->value();
    ADC.END3 = m_ui->ADC3end->value();
    ADC.NUMAV = m_ui->NUMAV_comboBox->currentIndex();
    ADC.decEn = m_ui->checkBox_Dec->isChecked();
    if (ADC.decEn)
        ADC.decFactor = m_ui->decim_comboBox->currentIndex();
    else
        ADC.decFactor = 0;
    qDebug() << ADC.decEn;
    qDebug() << ADC.decFactor;



    LEDfeatures.PRF = m_ui->PRFcounter->value();
    LEDfeatures.DIV_PRF = m_ui->CLKDIV_PRF->currentIndex();
    LEDfeatures.PDNSTC = m_ui->PDNstart->value();
    LEDfeatures.PDNENDC = m_ui->PDNend->value();
    LEDfeatures.GAIN = m_ui->Rf1_comboBox->currentIndex();
    LEDfeatures.CF = m_ui->Cf1_comboBox->currentIndex();
    LEDfeatures.SepGain = m_ui->Separate_checkBox->isChecked();
    LEDfeatures.GAIN_SEP = m_ui->Rf2_comboBox->currentIndex();
    LEDfeatures.CF_SEP = m_ui->Cf2_comboBox->currentIndex();
}
void AdvancedSettings::on_buttonBox_accepted()
{
    saveAdvGUI();
    qDebug()<<"accepted";
    //emit sendDataRequested();
    emit settingsUpdated();
    close();
}


void AdvancedSettings::on_buttonBox_rejected()
{
    qDebug()<<"rejected";
    hide();
}


/*void AdvancedSettings::on_buttonBox_clicked(QAbstractButton *button)
{
    qDebug()<<button;
}*/

void AdvancedSettings::fillInfo()
{
    m_ui->NUMAV_comboBox->addItems(QStringList() << "0" << "1" << "2" << "3" << "4" << "5" << "6" << "7" << "8" << "9" << "10" << "11" << "12" << "13" << "14" << "15");
    m_ui->Rf1_comboBox->addItems(QStringList() << "500k" << "250k" << "100k" << "50k" << "25k" << "10k" << "1M" << "2M");
    m_ui->Rf2_comboBox->addItems(QStringList() << "500k" << "250k" << "100k" << "50k" << "25k" << "10k" << "1M" << "2M");
    m_ui->Cf1_comboBox->addItems(QStringList() << "5p" << "2.5p" << "10p" << "7.5p" << "20p" << "17.5p" << "25p" << "22.5p");
    m_ui->Cf2_comboBox->addItems(QStringList() << "5p" << "2.5p" << "10p" << "7.5p" << "20p" << "17.5p" << "25p" << "22.5p");
    m_ui->Separate_checkBox->setChecked(false);
    m_ui->decim_comboBox->addItems(QStringList() << "1" << "2" << "4" << "8" << "16");
    m_ui->CLKDIV_PRF->addItems(QStringList() << "1" << "2" << "4" << "8" << "16");
}
void AdvancedSettings::loadAdvGUI()
{
    //int NUMAV = ADC.NUMAV;
    int DIV = m_ui->CLKDIV_PRF->currentIndex();
    DIV = pow(2, DIV);
    double fTE = 4.0/DIV, dtTE = DIV/4.0;
    double fADC = 4.0, fs = 4.0;
    m_ui->checkBox->setChecked(LED[0].enabled);
    m_ui->label_22->setEnabled(LED[0].enabled);
    m_ui->label_29->setEnabled(LED[0].enabled);
    m_ui->label_42->setEnabled(LED[0].enabled);
    m_ui->bright1->setEnabled(LED[0].enabled);
    m_ui->LED1_dur->setEnabled(LED[0].enabled);
    //on_checkBox_checkStateChanged(LED[0].enabled ? Qt::Checked : Qt::Unchecked);
    m_ui->tab_1->setEnabled(LED[0].enabled);
    m_ui->spinBox_L1onStart->setValue(LED[0].LEDSTC);
    m_ui->spinBox_L1onEnd->setValue(LED[0].LEDENDC);
    m_ui->spinBox_L1onDur->setValue(LED[0].LEDENDC-LED[0].LEDSTC);
    m_ui->LED1_dur->setValue((LED[0].LEDENDC-LED[0].LEDSTC+1)*dtTE);
    m_ui->spinBox_L1sampStart->setValue(LED[0].STC);
    m_ui->spinBox_L1sampEnd->setValue(LED[0].ENDC);
    m_ui->spinBox_L1sampDur->setValue(LED[0].ENDC-LED[0].STC);
    m_ui->spinBox_L1cpStart->setValue(LED[0].CONVST);
    m_ui->spinBox_L1cpEnd->setValue(LED[0].CONVEND);
    m_ui->spinBox_L1cpDur->setValue(LED[0].CONVEND-LED[0].CONVST);
    m_ui->bright1->setValue(LED[0].bright);
    on_bright1_valueChanged(LED[0].bright);

    m_ui->checkBox_2->setChecked(LED[1].enabled);
    m_ui->label_23->setEnabled(LED[1].enabled);
    m_ui->label_32->setEnabled(LED[1].enabled);
    m_ui->label_43->setEnabled(LED[1].enabled);
    m_ui->bright2->setEnabled(LED[1].enabled);
    m_ui->LED2_dur->setEnabled(LED[1].enabled);
    //on_checkBox_2_checkStateChanged(LED[1].enabled ? Qt::Checked : Qt::Unchecked);
    m_ui->tab_2->setEnabled(LED[1].enabled);
    m_ui->spinBox_L2onStart->setValue(LED[1].LEDSTC);
    m_ui->spinBox_L2onEnd->setValue(LED[1].LEDENDC);
    m_ui->spinBox_L2onDur->setValue(LED[1].LEDENDC-LED[1].LEDSTC);
    m_ui->LED2_dur->setValue((LED[1].LEDENDC-LED[1].LEDSTC+1)*dtTE);
    m_ui->spinBox_L2sampStart->setValue(LED[1].STC);
    m_ui->spinBox_L2sampEnd->setValue(LED[1].ENDC);
    m_ui->spinBox_L2sampDur->setValue(LED[1].ENDC-LED[1].STC);
    m_ui->spinBox_L2cpStart->setValue(LED[1].CONVST);
    m_ui->spinBox_L2cpEnd->setValue(LED[1].CONVEND);
    m_ui->spinBox_L2cpDur->setValue(LED[1].CONVEND-LED[1].CONVST);
    m_ui->bright2->setValue(LED[1].bright);
    on_bright2_valueChanged(LED[1].bright);

    m_ui->checkBox_3->setChecked(LED[2].enabled);
    m_ui->label_24->setEnabled(LED[2].enabled);
    m_ui->label_37->setEnabled(LED[2].enabled);
    m_ui->label_44->setEnabled(LED[2].enabled);
    m_ui->bright3->setEnabled(LED[2].enabled);
    m_ui->LED3_dur->setEnabled(LED[2].enabled);
    //on_checkBox_3_checkStateChanged(LED[2].enabled ? Qt::Checked : Qt::Unchecked);
    m_ui->tab_3->setEnabled(LED[2].enabled);
    m_ui->spinBox_L3onStart->setValue(LED[2].LEDSTC);
    m_ui->spinBox_L3onEnd->setValue(LED[2].LEDENDC);
    m_ui->spinBox_L3onDur->setValue(LED[2].LEDENDC-LED[2].LEDSTC);
    m_ui->LED3_dur->setValue((LED[2].LEDENDC-LED[2].LEDSTC+1)*dtTE);
    m_ui->spinBox_L3sampStart->setValue(LED[2].STC);
    m_ui->spinBox_L3sampEnd->setValue(LED[2].ENDC);
    m_ui->spinBox_L3sampDur->setValue(LED[2].ENDC-LED[2].STC);
    m_ui->spinBox_L3cpStart->setValue(LED[2].CONVST);
    m_ui->spinBox_L3cpEnd->setValue(LED[2].CONVEND);
    m_ui->spinBox_L3cpDur->setValue(LED[2].CONVEND-LED[2].CONVST);
    m_ui->bright3->setValue(LED[2].bright);
    on_bright3_valueChanged(LED[2].bright);

    m_ui->tab_4->setEnabled(AMB[0].enabled);
    m_ui->spinBox_L1sampStart_2->setValue(AMB[0].STC);
    m_ui->spinBox_L1sampEnd_2->setValue(AMB[0].ENDC);
    m_ui->spinBox_L1cpStart_2->setValue(AMB[0].CONVST);
    m_ui->spinBox_L1cpEnd_2->setValue(AMB[0].CONVEND);
        m_ui->tab_5->setEnabled(AMB[0].enabled);
        m_ui->spinBox_L1sampStart_3->setValue(AMB[1].STC);
        m_ui->spinBox_L1sampEnd_3->setValue(AMB[1].ENDC);
        m_ui->spinBox_L1cpStart_3->setValue(AMB[1].CONVST);
        m_ui->spinBox_L1cpEnd_3->setValue(AMB[1].CONVEND);
    m_ui->ADC0start->setValue(ADC.STC0);
    m_ui->ADC0end->setValue(ADC.END0);
    m_ui->ADC1start->setValue(ADC.STC1);
    m_ui->ADC1end->setValue(ADC.END1);
    m_ui->ADC2start->setValue(ADC.STC2);
    m_ui->ADC2end->setValue(ADC.END2);
    m_ui->ADC3start->setValue(ADC.STC3);
    m_ui->ADC3end->setValue(ADC.END3);
    m_ui->NUMAV_comboBox->setCurrentIndex(ADC.NUMAV);
    //qDebug() << "ADC enable: " <<ADC.decEn;
    //qDebug() << "ADC factor (index): " << ADC.decFactor;
    m_ui->checkBox_Dec->setChecked(ADC.decEn);
    m_ui->decim_comboBox->setCurrentIndex(ADC.decFactor);
    //qDebug() << m_ui->decim_comboBox->currentIndex();

    m_ui->PRFcounter->setValue(LEDfeatures.PRF);
    m_ui->PDNstart->setValue(LEDfeatures.PDNSTC);
    m_ui->PDNend->setValue(LEDfeatures.PDNENDC);
    m_ui->Rf1_comboBox->setCurrentIndex(LEDfeatures.GAIN);
    m_ui->Cf1_comboBox->setCurrentIndex(LEDfeatures.CF);
    m_ui->Separate_checkBox->setChecked(LEDfeatures.SepGain);
    m_ui->Rf2_comboBox->setCurrentIndex(LEDfeatures.GAIN_SEP);
    m_ui->Cf2_comboBox->setCurrentIndex(LEDfeatures.CF_SEP);
    m_ui->CLKDIV_PRF->setCurrentIndex(LEDfeatures.DIV_PRF);
}

void AdvancedSettings::on_Rf1_comboBox_currentTextChanged(const QString &arg1)
{
    if (m_ui->Separate_checkBox->checkState() == false)
    {
        m_ui->Rf2_comboBox->setCurrentText(arg1);
    }
}


void AdvancedSettings::on_Cf1_comboBox_currentTextChanged(const QString &arg1)
{
    if (m_ui->Separate_checkBox->checkState() == false)
    {
        m_ui->Cf2_comboBox->setCurrentText(arg1);
    }
}


void AdvancedSettings::on_Separate_checkBox_checkStateChanged(const Qt::CheckState &arg1)
{
    bool enabled = (arg1 == Qt::Checked);
    qDebug()<<enabled;
    m_ui->Cf2_comboBox->setCurrentText(m_ui->Cf1_comboBox->currentText());
    m_ui->Rf2_comboBox->setCurrentText(m_ui->Rf1_comboBox->currentText());
    m_ui->Rf2_comboBox->setEnabled(enabled);
    m_ui->Cf2_comboBox->setEnabled(enabled);
}


void AdvancedSettings::on_checkBox_checkStateChanged(const Qt::CheckState &arg1)
{
    m_ui->label_22->setEnabled(arg1);
    m_ui->label_29->setEnabled(arg1);
    m_ui->label_42->setEnabled(arg1);
    m_ui->bright1->setEnabled(arg1);
    m_ui->LED1_dur->setEnabled(arg1);
}


void AdvancedSettings::on_checkBox_2_checkStateChanged(const Qt::CheckState &arg1)
{
    m_ui->label_23->setEnabled(arg1);
    m_ui->label_32->setEnabled(arg1);
    m_ui->label_43->setEnabled(arg1);
    m_ui->bright2->setEnabled(arg1);
    m_ui->LED2_dur->setEnabled(arg1);
}


void AdvancedSettings::on_checkBox_3_checkStateChanged(const Qt::CheckState &arg1)
{
    m_ui->label_24->setEnabled(arg1);
    m_ui->label_37->setEnabled(arg1);
    m_ui->label_44->setEnabled(arg1);
    m_ui->bright3->setEnabled(arg1);
    m_ui->LED3_dur->setEnabled(arg1);
}


void AdvancedSettings::on_bright1_valueChanged(int value)
{
    int f = value*8/10;
    int g = (value*8)%10;
    QString txt;
    if (value != 63)
        txt=QString::number(f)+"."+QString::number(g);
    else
        txt=QString::number(f);
    txt += " mA";
    m_ui->label_42->setText(txt);
}


void AdvancedSettings::on_label_22_linkActivated(const QString &link)
{

}


void AdvancedSettings::on_bright2_valueChanged(int value)
{
    int f = value*8/10;
    int g = (value*8)%10;
    QString txt;
    if (value != 63)
        txt=QString::number(f)+"."+QString::number(g);
    else
        txt=QString::number(f);
    txt += " mA";
    m_ui->label_43->setText(txt);
}


void AdvancedSettings::on_bright3_valueChanged(int value)
{
    int f = value*8/10;
    int g = (value*8)%10;
    QString txt;
    if (value != 63)
        txt=QString::number(f)+"."+QString::number(g);
    else
        txt=QString::number(f);
    txt += " mA";
    m_ui->label_44->setText(txt);
}

void AdvancedSettings::onAnyValueChanged()
{
    //m_ui->buttonBox->button(QDialogButtonBox::Apply)->setEnabled(false);
    dataChanged = true;
    checkPassed = false;

    QPushButton *applyButton = m_ui->buttonBox->button(QDialogButtonBox::Apply);
    if (applyButton)
        applyButton->setEnabled(false); // запрещаем применять пока не проверено

    qDebug() << "⚙️ Settings changed — Apply disabled";
}

//#include <QTabWidget>
//#include <QTabBar>

void AdvancedSettings::swapTabs(QTabWidget *tabs)
{
    if (!tabs) return;

    // Ищем вкладки по именам (objectName из Qt Designer)
    auto findTab = [&](const QString &name) -> QWidget* {
        for (int i = 0; i < tabs->count(); ++i) {
            QWidget *w = tabs->widget(i);
            if (w && w->objectName() == name)
                return w;
        }
        return nullptr;
    };

    QWidget *tab_LED1     = findTab("tab_1");
    QWidget *tab_LED2     = findTab("tab_2");
    QWidget *tab_LED3     = findTab("tab_3");
    QWidget *tab_Ambient1 = findTab("tab_4");
    QWidget *tab_Ambient2 = findTab("tab_5");

    bool led1 = m_ui->checkBox->isChecked();
    bool led2 = m_ui->checkBox_2->isChecked();
    bool led3 = m_ui->checkBox_3->isChecked();

    QList<QWidget*> newOrder;

    // Формируем новый порядок нужных вкладок
    if ((led1 && !led2 && !led3) ||
        (led2 && !led1 && !led3) ||
        (led1 && led2 && !led3))
    {
        // LED2 Ambient2 LED1 Ambient1
        if (tab_LED2)     newOrder << tab_LED2;
        if (tab_Ambient2) newOrder << tab_Ambient2;
        if (tab_LED1)     newOrder << tab_LED1;
        if (tab_Ambient1) newOrder << tab_Ambient1;
    }
    else
    {
        // LED2 LED3 LED1 Ambient1
        if (tab_LED2)     newOrder << tab_LED2;
        if (tab_LED3)     newOrder << tab_LED3;
        if (tab_LED1)     newOrder << tab_LED1;
        if (tab_Ambient1) newOrder << tab_Ambient1;
    }

    // Добавляем все остальные вкладки в конец, если их нет в списке
    for (int i = 0; i < tabs->count(); ++i) {
        QWidget *w = tabs->widget(i);
        if (w && !newOrder.contains(w))
            newOrder << w;
    }

    // Меняем порядок, вставляя вкладки в новом порядке
    for (int newIndex = 0; newIndex < newOrder.size(); ++newIndex) {
        QWidget *w = newOrder[newIndex];
        int currentIndex = tabs->indexOf(w);
        if (currentIndex != newIndex && currentIndex != -1) {
            tabs->tabBar()->moveTab(currentIndex, newIndex);
        }
    }
}


void AdvancedSettings::on_pushButton_3_clicked(bool enabled) //update button ПЕРЕДЕЛАТЬ ЧТОБ всегда было 4 вкладки
{
    //qDebug() << m_ui->buttonBox->Apply;
    //m_ui->buttonBox->button(QDialogButtonBox::Apply)->setEnabled(enabled);
    bool led1=m_ui->checkBox->isChecked();
    bool led2=m_ui->checkBox_2->isChecked();
    bool led3=m_ui->checkBox_3->isChecked();
    if (led1)
        m_ui->tab_1->setEnabled(true);
    if (led2)
        m_ui->tab_2->setEnabled(true);
    if (led3)
        m_ui->tab_3->setEnabled(true);

    m_ui->tab_4->setEnabled(true);
    if (led1&!led2&!led3 | led2&!led1&!led3 | led1&led2&!led3)
    {
        m_ui->tab_1->setEnabled(true);
        m_ui->tab_2->setEnabled(true);
        m_ui->tab_5->setEnabled(true);
        //
    }
    else
    {
        m_ui->tab_1->setEnabled(true);
        m_ui->tab_2->setEnabled(true);
        m_ui->tab_3->setEnabled(true);
    }
    //m_ui->tab_1->setEnabled(true);
    //m_ui->tab_2->setEnabled(true);
    //m_ui->tab_3->setEnabled(true);
    //m_ui->tab_4->setEnabled(true);
    //m_ui->tab_5->setEnabled(true);
    /*m_ui->spinBox_L1onDur->setValue(m_ui->LED1_dur->value());
    m_ui->spinBox_L2onDur->setValue(m_ui->LED2_dur->value());
    m_ui->spinBox_L3onDur->setValue(m_ui->LED3_dur->value());
    m_ui->spinBox_L1onEnd->setValue(m_ui->LED1_dur->value());
    m_ui->spinBox_L2onEnd->setValue(m_ui->LED2_dur->value());
    m_ui->spinBox_L3onEnd->setValue(m_ui->LED3_dur->value());*/

    swapTabs(m_ui->tabWidget);
}


void AdvancedSettings::on_pushButton_2_clicked(bool enabled) //check button
{
    on_pushButton_3_clicked(true);
    int NUMAV = m_ui->NUMAV_comboBox->currentIndex();
    int DIV = m_ui->CLKDIV_PRF->currentIndex();
    DIV = pow(2, DIV);
    double fTE = 4.0/DIV, dtTE = DIV/4.0;
    double fADC = 4.0, fs = 4.0;
    int t1;
    bool error_flg = false; // no errors
    int duration;

    int total_errors = 0;

    duration = m_ui->LED2_dur->value();
    if (25.0/dtTE > 0.2*(duration+1))
        t1 = 25/dtTE;
    else t1 = 0.2*(duration+1);
    if (m_ui->spinBox_L2onEnd->value()-m_ui->spinBox_L2onStart->value() < t1)
    {
        qDebug() << "LED2 t1 error";
        m_ui->label_error->setToolTip("L2t1");
        total_errors++;
        error_flg = true;
    }



    if (m_ui->ADC0start->value()-m_ui->spinBox_L2onEnd->value() < 2)
    {
        qDebug() << "LED2 t2 error";
        m_ui->label_error->setToolTip(m_ui->label_error->toolTip()+", "+"L2t2");
        total_errors++;
        error_flg = true;
    }
    if (m_ui->ADC0end->value()-m_ui->ADC0start->value() < 6)
    {
        qDebug() << "LED2 t3 error";
        m_ui->label_error->setToolTip(m_ui->label_error->toolTip()+", "+"L2t3");
        total_errors++;
        error_flg = true;
    }
    if (m_ui->spinBox_L2cpStart->value()-m_ui->ADC0end->value() < 2)
    {
        qDebug() << "LED2 t4 error";
        m_ui->label_error->setToolTip(m_ui->label_error->toolTip()+", "+"L2t4");
        total_errors++;
        error_flg = true;
    }
    int t5 = ceil((NUMAV+1)*200/(fADC*dtTE));
    if (m_ui->spinBox_L2cpEnd->value()-m_ui->spinBox_L2cpStart->value() < t5)
    {
        qDebug() << "LED2 t5 error";
        m_ui->label_error->setToolTip(m_ui->label_error->toolTip()+", "+"L2t5");
        total_errors++;
        error_flg = true;
    }

    bool led1=m_ui->checkBox->isChecked();
    bool led2=m_ui->checkBox_2->isChecked();
    bool led3=m_ui->checkBox_3->isChecked();
    if (led1&!led2&!led3 | led2&!led1&!led3 | led1&led2&!led3) //L2->A2->L1->A1
    {
        m_ui->tab_1->setEnabled(true);
        m_ui->tab_2->setEnabled(true);
        m_ui->tab_4->setEnabled(true);
        m_ui->tab_5->setEnabled(true);
        if (m_ui->ADC1start->value()-m_ui->spinBox_L1sampEnd_3->value() < 2)
        {
            qDebug() << "Ambient-2 t2 error";
            m_ui->label_error->setToolTip(m_ui->label_error->toolTip()+", "+"A2t2");
            total_errors++;
            error_flg = true;
        }
        if (m_ui->ADC1end->value()-m_ui->ADC1start->value() < 6)
        {
            qDebug() << "Ambient-2 t3 error";
            m_ui->label_error->setToolTip(m_ui->label_error->toolTip()+", "+"A2t3");
            total_errors++;
            error_flg = true;
        }
        if (m_ui->spinBox_L1cpStart_3->value()-m_ui->ADC1end->value() < 2)
        {
            qDebug() << "Ambient-2 t4 error";
            m_ui->label_error->setToolTip(m_ui->label_error->toolTip()+", "+"A2t4");
            total_errors++;
            error_flg = true;
        }
        if (m_ui->spinBox_L1cpEnd_3->value()-m_ui->spinBox_L1cpStart_3->value() < t5)
        {
            qDebug() << "Ambient-2 t5 error";
            m_ui->label_error->setToolTip(m_ui->label_error->toolTip()+", "+"A2t5");
            total_errors++;
            error_flg = true;
        }

        duration = m_ui->LED1_dur->value();
        if (25.0/dtTE > 0.2*(duration+1))
            t1 = 25/dtTE;
        else t1 = 0.2*(duration+1);
        if (m_ui->spinBox_L1onEnd->value()-m_ui->spinBox_L1onStart->value() < t1)
        {
            qDebug() << "LED1 t1 error";
            m_ui->label_error->setToolTip(m_ui->label_error->toolTip()+", "+"L1t1");
            total_errors++;
            error_flg = true;
        }
        if (m_ui->ADC2start->value()-m_ui->spinBox_L1onEnd->value() < 2)
        {
            qDebug() << "LED1 t2 error";
            m_ui->label_error->setToolTip(m_ui->label_error->toolTip()+", "+"L1t2");
            total_errors++;
            error_flg = true;
        }
        if (m_ui->ADC2end->value()-m_ui->ADC2start->value() < 6)
        {
            qDebug() << "LED1 t3 error";
            m_ui->label_error->setToolTip(m_ui->label_error->toolTip()+", "+"L1t3");
            total_errors++;
            error_flg = true;
        }
        if (m_ui->spinBox_L1cpStart->value()-m_ui->ADC2end->value() < 2)
        {
            qDebug() << "LED1 t4 error";
            m_ui->label_error->setToolTip(m_ui->label_error->toolTip()+", "+"L1t4");
            total_errors++;
            error_flg = true;
        }
        if (m_ui->spinBox_L1cpEnd->value()-m_ui->spinBox_L1cpStart->value() < t5)
        {
            qDebug() << "LED1 t5 error";
            m_ui->label_error->setToolTip(m_ui->label_error->toolTip()+", "+"L1t5");
            total_errors++;
            error_flg = true;
        }

        if (m_ui->ADC3start->value()-m_ui->spinBox_L1sampEnd_2->value() < 2)
        {
            qDebug() << "Ambient-1 t2 error";
            m_ui->label_error->setToolTip(m_ui->label_error->toolTip()+", "+"A1t2");
            total_errors++;
            error_flg = true;
        }
        if (m_ui->ADC3end->value()-m_ui->ADC3start->value() < 6)
        {
            qDebug() << "Ambient-1 t3 error";
            m_ui->label_error->setToolTip(m_ui->label_error->toolTip()+", "+"A1t3");
            total_errors++;
            error_flg = true;
        }
        if (m_ui->spinBox_L1cpStart_2->value()-m_ui->ADC3end->value() < 2)
        {
            qDebug() << "Ambient-1 t4 error";
            m_ui->label_error->setToolTip(m_ui->label_error->toolTip()+", "+"A1t4");
            total_errors++;
            error_flg = true;
        }
        if (m_ui->spinBox_L1cpEnd_2->value()-m_ui->spinBox_L1cpStart_2->value() < t5)
        {
            qDebug() << "Ambient-1 t5 error";
            m_ui->label_error->setToolTip(m_ui->label_error->toolTip()+", "+"A1t5");
            total_errors++;
            error_flg = true;
        }

    } else
    {
        m_ui->tab_1->setEnabled(true);
        m_ui->tab_2->setEnabled(true);
        m_ui->tab_3->setEnabled(true);
        m_ui->tab_4->setEnabled(true);
        duration = m_ui->LED3_dur->value();
        if (25.0/dtTE > 0.2*(duration+1))
            t1 = 25/dtTE;
        else t1 = 0.2*(duration+1);
        if (m_ui->spinBox_L3onEnd->value()-m_ui->spinBox_L3onStart->value() < t1)
        {
            qDebug() << "LED3 t1 error";
            m_ui->label_error->setToolTip(m_ui->label_error->toolTip()+", "+"L3t1");
            total_errors++;
            error_flg = true;
        }
        if (m_ui->ADC1start->value()-m_ui->spinBox_L3onEnd->value() < 2)
        {
            qDebug() << "LED3 t2 error";
            m_ui->label_error->setToolTip(m_ui->label_error->toolTip()+", "+"L3t2");
            total_errors++;
            error_flg = true;
        }
        if (m_ui->ADC1end->value()-m_ui->ADC1start->value() < 6)
        {
            qDebug() << "LED3 t3 error";
            m_ui->label_error->setToolTip(m_ui->label_error->toolTip()+", "+"L3t3");
            total_errors++;
            error_flg = true;
        }
        if (m_ui->spinBox_L3cpStart->value()-m_ui->ADC1end->value() < 2)
        {
            qDebug() << "LED3 t4 error";
            m_ui->label_error->setToolTip(m_ui->label_error->toolTip()+", "+"L3t4");
            total_errors++;
            error_flg = true;
        }
        if (m_ui->spinBox_L3cpEnd->value()-m_ui->spinBox_L3cpStart->value() < t5)
        {
            qDebug() << "LED3 t5 error";
            m_ui->label_error->setToolTip(m_ui->label_error->toolTip()+", "+"L3t5");
            total_errors++;
            error_flg = true;
        }
        duration = m_ui->LED1_dur->value();
        if (25.0/dtTE > 0.2*(duration+1))
            t1 = 25/dtTE;
        else t1 = 0.2*(duration+1);
        if (m_ui->spinBox_L1onEnd->value()-m_ui->spinBox_L1onStart->value() < t1)
        {
            qDebug() << "LED1 t1 error";
            m_ui->label_error->setToolTip(m_ui->label_error->toolTip()+", "+"L1t1");
            total_errors++;
            error_flg = true;
        }
        if (m_ui->ADC2start->value()-m_ui->spinBox_L1onEnd->value() < 2)
        {
            qDebug() << "LED1 t2 error";
            m_ui->label_error->setToolTip(m_ui->label_error->toolTip()+", "+"L1t2");
            total_errors++;
            error_flg = true;
        }
        if (m_ui->ADC2end->value()-m_ui->ADC2start->value() < 6)
        {
            qDebug() << "LED1 t3 error";
            m_ui->label_error->setToolTip(m_ui->label_error->toolTip()+", "+"L1t3");
            total_errors++;
            error_flg = true;
        }
        if (m_ui->spinBox_L1cpStart->value()-m_ui->ADC2end->value() < 2)
        {
            qDebug() << "LED1 t4 error";
            m_ui->label_error->setToolTip(m_ui->label_error->toolTip()+", "+"L1t4");
            total_errors++;
            error_flg = true;
        }
        if (m_ui->spinBox_L1cpEnd->value()-m_ui->spinBox_L1cpStart->value() < t5)
        {
            qDebug() << "LED1 t5 error";
            m_ui->label_error->setToolTip(m_ui->label_error->toolTip()+", "+"L1t5");
            total_errors++;
            error_flg = true;
        }

        if (m_ui->ADC3start->value()-m_ui->spinBox_L1sampEnd_2->value() < 2)
        {
            qDebug() << "Ambient-1 t2 error";
            m_ui->label_error->setToolTip(m_ui->label_error->toolTip()+", "+"A1t2");
            total_errors++;
            error_flg = true;
        }
        if (m_ui->ADC3end->value()-m_ui->ADC3start->value() < 6)
        {
            qDebug() << "Ambient-1 t3 error";
            m_ui->label_error->setToolTip(m_ui->label_error->toolTip()+", "+"A1t3");
            total_errors++;
            error_flg = true;
        }
        if (m_ui->spinBox_L1cpStart_2->value()-m_ui->ADC1end->value() < 2)
        {
            qDebug() << "Ambient-1 t4 error";
            m_ui->label_error->setToolTip(m_ui->label_error->toolTip()+", "+"A1t4");
            total_errors++;
            error_flg = true;
        }
        if (m_ui->spinBox_L1cpEnd_2->value()-m_ui->spinBox_L1cpStart_2->value() < t5)
        {
            qDebug() << "Ambient-1 t5 error";
            m_ui->label_error->setToolTip(m_ui->label_error->toolTip()+", "+"A1t5");
            total_errors++;
            error_flg = true;
        }
        if (m_ui->PDNstart->value()-m_ui->spinBox_L1cpEnd_2->value() < ceil(200/dtTE))
        {
            qDebug() << "PDN t8 error";
            m_ui->label_error->setToolTip(m_ui->label_error->toolTip()+", "+"PDNt8");
            total_errors++;
            error_flg = true;
        }
        if (m_ui->PRFcounter->value()-m_ui->PDNend->value() < ceil(200/dtTE))
        {
            qDebug() << "PDN t9 error";
            m_ui->label_error->setToolTip(m_ui->label_error->toolTip()+", "+"PDNt9");
            total_errors++;
            error_flg = true;
        }
    }
    if (m_ui->PDNend->value() < m_ui->PDNstart->value())
    {
        total_errors++;
        m_ui->label_error->setToolTip(m_ui->label_error->toolTip()+", "+"PDN_ERROR");
    }
    m_ui->label_error->setText(QString::number(total_errors));
    if (!error_flg)
    {
        m_ui->buttonBox->button(QDialogButtonBox::Apply)->setEnabled(true);
        m_ui->label_error->setToolTip("");
        dataChanged = false;
    }


}

int* AdvancedSettings::getTimings(int duration)
{

    int NUMAV = m_ui->NUMAV_comboBox->currentIndex();
    int DIV = m_ui->CLKDIV_PRF->currentIndex();
    DIV = pow(2, DIV);
    //NUMAV = pow(2, DIV);
    double fTE = 4.0/DIV, dtTE = DIV/4.0;
    double fADC = 4.0, fs = 4.0;
    duration = duration/dtTE;
    int *t = new int[10];
    if (duration == 0)
        t[1] = 0;
    else {
    if (25.0/dtTE > 0.2*(duration+1))
        t[0] = 25/dtTE;
    else t[0] = 0.2*(duration+1);
    t[1] = 2;
    t[2] = 6;
    t[3] = 2;
    t[4] = ceil(((NUMAV+2)*200/fADC+15)/dtTE);
    t[5] = ceil((NUMAV+1)*200/(fADC*dtTE));
    t[6] = ceil(fs/fADC);
    t[7] = ceil(200/dtTE);
    t[8] = t[7];
    t[9] = duration;
    }
    return t;
}
void AdvancedSettings::on_pushButton_clicked(bool enabled) //simulate button
{
    int PRF = m_ui->PRFcounter->value();
    bool led1=m_ui->checkBox->isChecked();
    bool led2=m_ui->checkBox_2->isChecked();
    bool led3=m_ui->checkBox_3->isChecked();
    int tmult = m_ui->multiplex->value();
    if (tmult == 0)
        tmult = 2;
    if (PRF)
    {
        int *t1 = new int[10];
        int *t2 = new int[10];
        int *t3 = new int[10];
        int *t4 = new int[10];
        if (led1&!led2&!led3 | led2&!led1&!led3 | led1&led2&!led3) //L2->A2->L1->A1
        {
            m_ui->tab_1->setEnabled(true);
            m_ui->tab_2->setEnabled(true);
            m_ui->tab_4->setEnabled(true);
            m_ui->tab_5->setEnabled(true);
            if (m_ui->LED2_dur->value() == 0)
            {
                m_ui->LED2_dur->setValue(100); // phantom duration for calculation
            }
            t1 = getTimings(m_ui->LED2_dur->value()); // LED2
            t1[9]--;
            t1[4]--;
            m_ui->spinBox_L2onDur->setValue(t1[9]);
            m_ui->spinBox_L2onStart->setValue(0);
            m_ui->spinBox_L2onEnd->setValue(t1[9]);
            m_ui->spinBox_L2sampStart->setValue(t1[0]);
            m_ui->spinBox_L2sampEnd->setValue(m_ui->spinBox_L2onEnd->value());
            m_ui->spinBox_L2sampDur->setValue(m_ui->spinBox_L2sampEnd->value()-m_ui->spinBox_L2sampStart->value());
            m_ui->ADC0start->setValue(m_ui->spinBox_L2sampEnd->value()+t1[1]);
            m_ui->ADC0end->setValue(m_ui->ADC0start->value()+t1[2]);
            m_ui->spinBox_L2cpStart->setValue(m_ui->ADC0end->value()+t1[3]);
            m_ui->spinBox_L2cpEnd->setValue(m_ui->spinBox_L2cpStart->value()+t1[4]);
            m_ui->spinBox_L2cpDur->setValue(m_ui->spinBox_L2cpEnd->value()-m_ui->spinBox_L2cpStart->value());
            t2 = getTimings(m_ui->LED2_dur->value()); // AMBIENT 2
            t2[9]--;
            t2[4]--;
            m_ui->spinBox_L1sampStart_3->setValue(m_ui->spinBox_L2sampEnd->value()+tmult+t2[0]); //+2
            m_ui->spinBox_L1sampEnd_3->setValue(m_ui->spinBox_L1sampStart_3->value()+t2[9]-t2[0]);
            m_ui->spinBox_L1sampDur_3->setValue(m_ui->spinBox_L1sampEnd_3->value()-m_ui->spinBox_L1sampStart_3->value());
            m_ui->ADC1start->setValue(m_ui->spinBox_L2cpEnd->value()+t2[1]);
            m_ui->ADC1end->setValue(m_ui->ADC1start->value()+t2[2]);
            m_ui->spinBox_L1cpStart_3->setValue(m_ui->ADC1end->value()+t2[3]);
            m_ui->spinBox_L1cpEnd_3->setValue(m_ui->spinBox_L1cpStart_3->value()+t2[4]);
            m_ui->spinBox_L1cpDur_3->setValue(m_ui->spinBox_L1cpEnd_3->value()-m_ui->spinBox_L1cpStart_3->value());
            if (m_ui->LED1_dur->value() == 0)
            {
                m_ui->LED1_dur->setValue(100); // phantom duration for calculation
            }
            t3 = getTimings(m_ui->LED1_dur->value()); // LED1
            t3[9]--;
            t3[4]--;
            m_ui->spinBox_L1onDur->setValue(t3[9]);
            m_ui->spinBox_L1onStart->setValue(m_ui->spinBox_L1sampEnd_3->value()+tmult);
            m_ui->spinBox_L1onEnd->setValue(m_ui->spinBox_L1onStart->value()+t3[9]);
            m_ui->spinBox_L1sampStart->setValue(m_ui->spinBox_L1onStart->value()+t3[0]);
            m_ui->spinBox_L1sampEnd->setValue(m_ui->spinBox_L1onEnd->value());
            m_ui->spinBox_L1sampDur->setValue(m_ui->spinBox_L1sampEnd->value()-m_ui->spinBox_L1sampStart->value());
            m_ui->ADC2start->setValue(m_ui->spinBox_L1cpEnd_3->value()+t3[1]);
            m_ui->ADC2end->setValue(m_ui->ADC2start->value()+t3[2]);
            m_ui->spinBox_L1cpStart->setValue(m_ui->ADC2end->value()+t3[3]);
            m_ui->spinBox_L1cpEnd->setValue(m_ui->spinBox_L1cpStart->value()+t3[4]);
            m_ui->spinBox_L1cpDur->setValue(m_ui->spinBox_L1cpEnd->value()-m_ui->spinBox_L1cpStart->value());
            t4 = getTimings(m_ui->LED2_dur->value()); // AMBIENT 1
            t4[9]--;
            t4[4]--;
            m_ui->spinBox_L1sampStart_2->setValue(m_ui->spinBox_L1sampEnd->value()+tmult+t4[0]);
            m_ui->spinBox_L1sampEnd_2->setValue(m_ui->spinBox_L1sampStart_2->value()+t4[9]-t2[0]);
            m_ui->spinBox_L1sampDur_2->setValue(m_ui->spinBox_L1sampEnd_2->value()-m_ui->spinBox_L1sampStart_2->value());
            m_ui->ADC3start->setValue(m_ui->spinBox_L1cpEnd->value()+t4[1]);
            m_ui->ADC3end->setValue(m_ui->ADC3start->value()+t4[2]);
            m_ui->spinBox_L1cpStart_2->setValue(m_ui->ADC3end->value()+t4[3]);
            m_ui->spinBox_L1cpEnd_2->setValue(m_ui->spinBox_L1cpStart_2->value()+t4[4]);
            m_ui->spinBox_L1cpDur_2->setValue(m_ui->spinBox_L1cpEnd_2->value()-m_ui->spinBox_L1cpStart_2->value());
            m_ui->PDNstart->setValue(m_ui->spinBox_L1cpEnd_2->value()+t4[7]);
            m_ui->PDNend->setValue(PRF-t4[8]);
        } else // L2->L3->L1->A1
        {
            m_ui->tab_1->setEnabled(true);
            m_ui->tab_2->setEnabled(true);
            m_ui->tab_3->setEnabled(true);
            m_ui->tab_4->setEnabled(true);
            if (m_ui->LED2_dur->value() == 0)
            {
                m_ui->LED2_dur->setValue(100); // phantom duration for calculation
            }
            t1 = getTimings(m_ui->LED2_dur->value()); // LED2
            t1[9]--;
            t1[4]--;
            m_ui->spinBox_L2onDur->setValue(t1[9]);
            m_ui->spinBox_L2onStart->setValue(0);
            m_ui->spinBox_L2onEnd->setValue(t1[9]);
            m_ui->spinBox_L2sampStart->setValue(t1[0]);
            m_ui->spinBox_L2sampEnd->setValue(m_ui->spinBox_L2onEnd->value());
            m_ui->spinBox_L2sampDur->setValue(t1[9]-t1[0]);
            m_ui->ADC0start->setValue(m_ui->spinBox_L2sampEnd->value()+t1[1]);
            m_ui->ADC0end->setValue(m_ui->ADC0start->value()+t1[2]);
            m_ui->spinBox_L2cpStart->setValue(m_ui->ADC0end->value()+t1[3]);
            m_ui->spinBox_L2cpEnd->setValue(m_ui->spinBox_L2cpStart->value()+t1[4]);
            m_ui->spinBox_L2cpDur->setValue(m_ui->spinBox_L2cpEnd->value()-m_ui->spinBox_L2cpStart->value());
            if (m_ui->LED3_dur->value() == 0)
            {
                m_ui->LED3_dur->setValue(100); // phantom duration for calculation
            }
            t2 = getTimings(m_ui->LED3_dur->value()); // LED3
            t2[9]--;
            t2[4]--;
            m_ui->spinBox_L3onDur->setValue(t2[9]);
            m_ui->spinBox_L3onStart->setValue(m_ui->spinBox_L2sampEnd->value()+tmult);
            m_ui->spinBox_L3onEnd->setValue(m_ui->spinBox_L3onStart->value()+t2[9]);
            m_ui->spinBox_L3sampStart->setValue(m_ui->spinBox_L3onStart->value()+t2[0]);
            m_ui->spinBox_L3sampEnd->setValue(m_ui->spinBox_L3onEnd->value());
            m_ui->spinBox_L3sampDur->setValue(m_ui->spinBox_L3sampEnd->value()-m_ui->spinBox_L3sampStart->value());
            m_ui->ADC1start->setValue(m_ui->spinBox_L2cpEnd->value()+t2[1]);
            m_ui->ADC1end->setValue(m_ui->ADC1start->value()+t2[2]);
            m_ui->spinBox_L3cpStart->setValue(m_ui->ADC1end->value()+t2[3]);
            m_ui->spinBox_L3cpEnd->setValue(m_ui->spinBox_L3cpStart->value()+t2[4]);
            m_ui->spinBox_L3cpDur->setValue(m_ui->spinBox_L3cpEnd->value()-m_ui->spinBox_L3cpStart->value());
            if (m_ui->LED1_dur->value() == 0)
            {
                m_ui->LED1_dur->setValue(100); // phantom duration for calculation
            }
            t3 = getTimings(m_ui->LED1_dur->value()); // LED1
            t3[9]--;
            t3[4]--;
            m_ui->spinBox_L1onDur->setValue(t3[9]);
            m_ui->spinBox_L1onStart->setValue(m_ui->spinBox_L3sampEnd->value()+tmult);
            m_ui->spinBox_L1onEnd->setValue(m_ui->spinBox_L1onStart->value()+t3[9]);
            m_ui->spinBox_L1sampStart->setValue(m_ui->spinBox_L1onStart->value()+t3[0]);
            m_ui->spinBox_L1sampEnd->setValue(m_ui->spinBox_L1onEnd->value());
            m_ui->spinBox_L1sampDur->setValue(m_ui->spinBox_L1sampEnd->value()-m_ui->spinBox_L1sampStart->value());
            m_ui->ADC2start->setValue(m_ui->spinBox_L3cpEnd->value()+t3[1]);
            m_ui->ADC2end->setValue(m_ui->ADC2start->value()+t3[2]);
            m_ui->spinBox_L1cpStart->setValue(m_ui->ADC2end->value()+t3[3]);
            m_ui->spinBox_L1cpEnd->setValue(m_ui->spinBox_L1cpStart->value()+t3[4]);
            m_ui->spinBox_L1cpDur->setValue(m_ui->spinBox_L1cpEnd->value()-m_ui->spinBox_L1cpStart->value());
            t4 = getTimings(m_ui->LED1_dur->value()); // AMBIENT 1
            t4[9]--;
            t4[4]--;
            m_ui->spinBox_L1sampStart_2->setValue(m_ui->spinBox_L1sampEnd->value()+tmult+t4[0]);
            m_ui->spinBox_L1sampEnd_2->setValue(m_ui->spinBox_L1sampStart_2->value()+t4[9]);
            m_ui->spinBox_L1sampDur_2->setValue(m_ui->spinBox_L1sampEnd_2->value()-m_ui->spinBox_L1sampStart_2->value());
            m_ui->ADC3start->setValue(m_ui->spinBox_L1cpEnd->value()+t4[1]);
            m_ui->ADC3end->setValue(m_ui->ADC3start->value()+t4[2]);
            m_ui->spinBox_L1cpStart_2->setValue(m_ui->ADC3end->value()+t4[3]);
            m_ui->spinBox_L1cpEnd_2->setValue(m_ui->spinBox_L1cpStart_2->value()+t4[4]);
            m_ui->spinBox_L1cpDur_2->setValue(m_ui->spinBox_L1cpEnd_2->value()-m_ui->spinBox_L1cpStart_2->value());
            m_ui->PDNstart->setValue(m_ui->spinBox_L1cpEnd_2->value()+t4[7]);
            m_ui->PDNend->setValue(PRF-t4[8]);
        }

    }

}


void AdvancedSettings::on_checkBox_Dec_checkStateChanged(const Qt::CheckState &arg1)
{
    bool enabled = (arg1 == Qt::Checked);
    m_ui->decim_comboBox->setEnabled(enabled);
}


void AdvancedSettings::on_multiplex_valueChanged(int value)
{
    QString txt;
    txt=QString::number(value);
    m_ui->label_multiplex->setText(txt);
}

