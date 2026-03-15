#ifndef LEDMANAGER_H
#define LEDMANAGER_H

#include <QtGlobal>

struct LED_settings
{
    bool enabled;
    quint16 STC, ENDC; //Start, End of Sample
    quint16 LEDSTC, LEDENDC; // Start, End of LED (on and off)
    quint16 CONVST, CONVEND; // Start, End of Convertion Phase
    quint8 bright;
};
struct AMB_settings
{
    bool enabled;
    quint16 STC, ENDC; //Start, End of Sample
    quint16 CONVST, CONVEND; //Start, End of Ambient Convertion Phase
};

struct ADC_converter
{
    quint16 STC0, END0;
    quint16 STC1, END1;
    quint16 STC2, END2;
    quint16 STC3, END3;
    quint8 NUMAV;
    bool decEn;
    quint8 decFactor;
};
struct features
{
    quint16 PRF;
    quint8 DIV_PRF;
    quint16 PDNSTC, PDNENDC;
    quint8 GAIN;
    quint8 CF;
    bool SepGain; //enable separate gain
    quint8 GAIN_SEP;
    quint8 CF_SEP;
};
struct DAC
{
    quint8 POL1, POL2, POL3, POL4;
    quint8 LED1, LED2, LED3, AMB1;
};

// Объявление глобальных переменных
extern LED_settings LED[3];
extern AMB_settings AMB[2];
extern ADC_converter ADC;
extern features LEDfeatures;
extern DAC Offset;
#endif // LEDMANAGER_H
