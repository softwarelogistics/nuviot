#ifndef PINS_CONFIG_H
#define PINS_CONFIG_H

#include <Arduino.h>

#define GP0O1_BRD_V2_K4_CTL 13
#define GP0O1_BRD_V2_K3_CTL 12
#define GP0O1_BRD_V2_K2_CTL 14
#define GP0O1_BRD_V2_K1_CTL 27

#define GP0O1_BRD_V2_GPRS_RX 23
#define GP0O1_BRD_V2_GPRS_TX 19

#define GP0O1_BRD_V2_SCL1 22
#define GP0O1_BRD_V2_SDA1 21

#define GP0O1_BRD_V2_IO1 17
#define GP0O1_BRD_V2_IO2 5
#define GP0O1_BRD_V2_IO3 34
#define GP0O1_BRD_V2_IO4 18
#define GP0O1_BRD_V2_IO5 38
#define GP0O1_BRD_V2_IO6 37
#define GP0O1_BRD_V2_IO7 25
#define GP0O1_BRD_V2_IO8 18

#define GP0O1_BRD_V2_ADC1 7
#define GP0O1_BRD_V2_ADC2 6
#define GP0O1_BRD_V2_ADC3 1
#define GP0O1_BRD_V2_ADC4 2
#define GP0O1_BRD_V2_ADC5 4
#define GP0O1_BRD_V2_ADC6 5
#define GP0O1_BRD_V2_ADC7 3
#define GP0O1_BRD_V2_ADC8 0

#define GP0O1_BRD_V2_RUNNING_LED 5
#define GP0O1_BRD_V2_ONLINE_LED 5
#define GP0O1_BRD_V2_ERR_LED 5

class ConfigPins
{
public:
    uint8_t K1Ctl = GP0O1_BRD_V2_K4_CTL;
    uint8_t K2Ctl = GP0O1_BRD_V2_K3_CTL;
    uint8_t K3Ctl = GP0O1_BRD_V2_K2_CTL;
    uint8_t K4Ctl = GP0O1_BRD_V2_K1_CTL;

    uint8_t SimRx = GP0O1_BRD_V2_GPRS_RX;
    uint8_t SimTx = GP0O1_BRD_V2_GPRS_TX;

    uint8_t Scl1 = GP0O1_BRD_V2_SCL1;
    uint8_t Sda1 = GP0O1_BRD_V2_SDA1;

    uint8_t CTChannel1 = GP0O1_BRD_V2_ADC1;
    uint8_t CTChannel2 = GP0O1_BRD_V2_ADC2;
    uint8_t CTChannel3 = GP0O1_BRD_V2_ADC3;

    uint8_t ADCChannel1 = GP0O1_BRD_V2_ADC1;
    uint8_t ADCChannel2 = GP0O1_BRD_V2_ADC2;
    uint8_t ADCChannel3 = GP0O1_BRD_V2_ADC3;
    uint8_t ADCChannel4 = GP0O1_BRD_V2_ADC4;
    uint8_t ADCChannel5 = GP0O1_BRD_V2_ADC5;
    uint8_t ADCChannel6 = GP0O1_BRD_V2_ADC6;
    uint8_t ADCChannel7 = GP0O1_BRD_V2_ADC7;
    uint8_t ADCChannel8 = GP0O1_BRD_V2_ADC8;

    uint8_t Gpio0 = GP0O1_BRD_V2_IO1;
    uint8_t Gpio1 = GP0O1_BRD_V2_IO2;
    uint8_t Gpio2 = GP0O1_BRD_V2_IO2;
    uint8_t Gpio3 = GP0O1_BRD_V2_IO2;
    uint8_t Gpio4 = GP0O1_BRD_V2_IO2;
    uint8_t Gpio5 = GP0O1_BRD_V2_IO2;
    uint8_t Gpio6 = GP0O1_BRD_V2_IO2;
    uint8_t Gpio7 = GP0O1_BRD_V2_IO2;

public:
    ConfigPins()
    {
    }

    void init(uint8_t boardType)
    {
        K1Ctl = GP0O1_BRD_V2_K4_CTL;
        K2Ctl = GP0O1_BRD_V2_K3_CTL;
        K3Ctl = GP0O1_BRD_V2_K2_CTL;
        K4Ctl = GP0O1_BRD_V2_K1_CTL;

        SimRx = GP0O1_BRD_V2_GPRS_RX;
        SimTx = GP0O1_BRD_V2_GPRS_TX;

        Scl1 = GP0O1_BRD_V2_SCL1;
        Sda1 = GP0O1_BRD_V2_SDA1;

        ADCChannel1 = GP0O1_BRD_V2_ADC1;
        ADCChannel2 = GP0O1_BRD_V2_ADC2;
        ADCChannel3 = GP0O1_BRD_V2_ADC3;
        ADCChannel4 = GP0O1_BRD_V2_ADC4;
        ADCChannel5 = GP0O1_BRD_V2_ADC5;
        ADCChannel6 = GP0O1_BRD_V2_ADC6;
        ADCChannel7 = GP0O1_BRD_V2_ADC7;
        ADCChannel8 = GP0O1_BRD_V2_ADC8;

        Gpio0 = GP0O1_BRD_V2_IO1;
        Gpio1 = GP0O1_BRD_V2_IO2;
        Gpio2 = GP0O1_BRD_V2_IO3;
        Gpio3 = GP0O1_BRD_V2_IO4;
        Gpio4 = GP0O1_BRD_V2_IO5;
        Gpio5 = GP0O1_BRD_V2_IO6;
        Gpio6 = GP0O1_BRD_V2_IO7;
        Gpio7 = GP0O1_BRD_V2_IO8;
    }
};

#endif
