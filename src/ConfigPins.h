#ifndef PINS_CONFIG_H
#define PINS_CONFIG_H

#include <Arduino.h>

#define GP0O1_BRD_V2_K4_CTL 13
#define GP0O1_BRD_V2_K3_CTL 12
#define GP0O1_BRD_V2_K2_CTL 14
#define GP0O1_BRD_V2_K1_CTL 27

#define GP0O1_BRD_V2_GPRS_RX 23
#define GP0O1_BRD_V2_GPRS_TX 19

#define GP0O1_BRD_V1_SCL1 15
#define GP0O1_BRD_V1_SDA1 4

#define GP0O1_BRD_V2_SCL1 22
#define GP0O1_BRD_V2_SDA1 21

#define GP0O1_BRD_UART_NUMBER 1
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

/* Production Board V1 */
//#define PROD_BRD_V1_GPRS_RX 23
//#define PROD_BRD_V1_GPRS_TX 19

#define PROD_BRD_V1_SCL1 22
#define PROD_BRD_V1_SDA1 21

#define PROD_BRD_UART_NUMBER 0
#define PROD_BRD_V1_RX 34
#define PROD_BRD_V1_TX 35

#define PROD_BRD_V1_IO1 17
#define PROD_BRD_V1_IO2 5
#define PROD_BRD_V1_IO3 27
#define PROD_BRD_V1_IO4 4
#define PROD_BRD_V1_IO5 16
#define PROD_BRD_V1_IO6 -1
#define PROD_BRD_V1_IO7 -1
#define PROD_BRD_V1_IO8 -1

#define PROD_BRD_V1_ADC1 5
#define PROD_BRD_V1_ADC2 4
#define PROD_BRD_V1_ADC3 6
#define PROD_BRD_V1_ADC4 0
#define PROD_BRD_V1_ADC5 1
#define PROD_BRD_V1_ADC6 2
#define PROD_BRD_V1_ADC7 7
#define PROD_BRD_V1_ADC8 3

#define PROD_BRD_V1_RUNNING_LED 5
#define PROD_BRD_V1_ONLINE_LED 5
#define PROD_BRD_V1_ERR_LED 5

class ConfigPins
{
public:
    int8_t K1Ctl = -1;
    int8_t K2Ctl = -1;
    int8_t K3Ctl = -1;
    int8_t K4Ctl = -1;

    int8_t SimRx = -1;
    int8_t SimTx = -1;    

    int8_t UartNumber = -1;

    uint8_t Scl1 = GP0O1_BRD_V2_SCL1;
    uint8_t Sda1 = GP0O1_BRD_V2_SDA1;

    uint8_t CTChannel1 = GP0O1_BRD_V2_ADC4;
    uint8_t CTChannel2 = GP0O1_BRD_V2_ADC5;
    uint8_t CTChannel3 = GP0O1_BRD_V2_ADC6;

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

    uint8_t SerialPort = 0;

    int8_t ErrorLED = -1;
    int8_t OnlineLED = -1;

    bool HasDisplay;
    bool HasRelays;

public:
    ConfigPins()
    {
    }

    void init(uint8_t boardType){
        if(boardType == 0) {
            K1Ctl = GP0O1_BRD_V2_K4_CTL;
            K2Ctl = GP0O1_BRD_V2_K3_CTL;
            K3Ctl = GP0O1_BRD_V2_K2_CTL;
            K4Ctl = GP0O1_BRD_V2_K1_CTL;
            UartNumber = GP0O1_BRD_UART_NUMBER;

            SimRx = GP0O1_BRD_V2_GPRS_RX;
            SimTx = GP0O1_BRD_V2_GPRS_TX;

            Scl1 = GP0O1_BRD_V1_SCL1;
            Sda1 = GP0O1_BRD_V1_SDA1;

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

            HasDisplay = true;
            HasRelays = true;

            SerialPort = 1;
        }
        else if(boardType == 1) {
            K1Ctl = GP0O1_BRD_V2_K4_CTL;
            K2Ctl = GP0O1_BRD_V2_K3_CTL;
            K3Ctl = GP0O1_BRD_V2_K2_CTL;
            K4Ctl = GP0O1_BRD_V2_K1_CTL;
            UartNumber = GP0O1_BRD_UART_NUMBER;

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

            HasDisplay = true;
            HasRelays = true;

            SerialPort = 1;
        }
        else if(boardType == 2) {
            Scl1 = PROD_BRD_V1_SCL1;
            Sda1 = PROD_BRD_V1_SDA1;

            UartNumber = PROD_BRD_UART_NUMBER;
            SimRx = PROD_BRD_V1_RX;
            SimTx = PROD_BRD_V1_TX;

            ADCChannel1 = PROD_BRD_V1_ADC1;
            ADCChannel2 = PROD_BRD_V1_ADC2;
            ADCChannel3 = PROD_BRD_V1_ADC3;
            ADCChannel4 = PROD_BRD_V1_ADC4;
            ADCChannel5 = PROD_BRD_V1_ADC5;
            ADCChannel6 = PROD_BRD_V1_ADC6;
            ADCChannel7 = PROD_BRD_V1_ADC7;
            ADCChannel8 = PROD_BRD_V1_ADC8;

            Gpio0 = PROD_BRD_V1_IO1;
            Gpio1 = PROD_BRD_V1_IO2;
            Gpio2 = PROD_BRD_V1_IO3;
            Gpio3 = PROD_BRD_V1_IO4;
            Gpio4 = PROD_BRD_V1_IO5;
            Gpio5 = PROD_BRD_V1_IO6;
            Gpio6 = PROD_BRD_V1_IO7;
            Gpio7 = PROD_BRD_V1_IO8;

            HasDisplay = false;
            HasRelays = false;

            ErrorLED = 19;
            OnlineLED = 23;

            SerialPort = 1;
        }
    }
};

#endif