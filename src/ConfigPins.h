#ifndef PINS_CONFIG_H
#define PINS_CONFIG_H

#include <Arduino.h>

#define GP0O1_BRD_V2_K4_CTL 13
#define GP0O1_BRD_V2_K3_CTL 12
#define GP0O1_BRD_V2_K2_CTL 14
#define GP0O1_BRD_V2_K1_CTL 27

#define GP0O1_BRD_V2_GPRS_RX 23
#define GP0O1_BRD_V2_GPRS_TX 19

#define CHARGE_BOARD_RX 19
#define CHARGE_BOARD_TX 19

#define CONSOLE_RX 5
#define CONSOLE_TX 17

#define CAN_RX 5
#define CAN_TX 17

#define GP0O1_BRD_V1_SCL1 15
#define GP0O1_BRD_V1_SDA1 4

#define GP0O1_BRD_V2_SCL1 22
#define GP0O1_BRD_V2_SDA1 21

#define GP0O1_BRD_UART_NUMBER 1
#define GP0O1_BRD_V2_IO1 17
#define GP0O1_BRD_V2_IO2 5
#define GP0O1_BRD_V2_IO3 19
#define GP0O1_BRD_V2_IO4 18
#define GP0O1_BRD_V2_IO5 38
#define GP0O1_BRD_V2_IO6 37
#define GP0O1_BRD_V2_IO7 25
#define GP0O1_BRD_V2_IO8 18

#define GP0O1_BRD_V1_IO1 17
#define GP0O1_BRD_V1_IO2 14
#define GP0O1_BRD_V1_IO3 19
#define GP0O1_BRD_V1_IO4 18
#define GP0O1_BRD_V1_IO5 38
#define GP0O1_BRD_V1_IO6 37
#define GP0O1_BRD_V1_IO7 25
#define GP0O1_BRD_V1_IO8 18

#define GP0O1_BRD_V2_ADC1 0
#define GP0O1_BRD_V2_ADC2 1
#define GP0O1_BRD_V2_ADC3 2
#define GP0O1_BRD_V2_ADC4 3
#define GP0O1_BRD_V2_ADC5 4
#define GP0O1_BRD_V2_ADC6 5
#define GP0O1_BRD_V2_ADC7 6
#define GP0O1_BRD_V2_ADC8 7
#define MODEM_RESET_V2 26
#define GP0O1_BRD_V2_RUNNING_LED 5
#define GP0O1_BRD_V2_ONLINE_LED 5
#define GP0O1_BRD_V2_ERR_LED 5

/* Production Board V1 */
#define PROD_BRD_V1_GPRS_RX 3
#define PROD_BRD_V1_GPRS_TX 1

#define PROD_BRD_V1_SCL1 22
#define PROD_BRD_V1_SDA1 21

#define PROD_BRD_MODEM_RESET_V2 18

#define PROD_BRD_UART_NUMBER 0
#define PROD_BRD_V1_RX 34
#define PROD_BRD_V1_TX 35

#define PROD_BRD_V1_IO1 16
#define PROD_BRD_V1_IO2 04
#define PROD_BRD_V1_IO3 05
#define PROD_BRD_V1_IO4 36
#define PROD_BRD_V1_IO5 39
#define PROD_BRD_V1_IO6 34
#define PROD_BRD_V1_IO7 35
#define PROD_BRD_V1_IO8 32

#define PROD_BRD_V1_ADC1 5 // GOOD
#define PROD_BRD_V1_ADC2 4 // GOOD
#define PROD_BRD_V1_ADC3 6 // GOOD
#define PROD_BRD_V1_ADC4 2 // GOOD 
#define PROD_BRD_V1_ADC5 0 // GOOD
#define PROD_BRD_V1_ADC6 1
#define PROD_BRD_V1_ADC7 7
#define PROD_BRD_V1_ADC8 3 // GOOD

#define PROD_BRD_V1_RUNNING_LED 5
#define PROD_BRD_V1_ONLINE_LED 5
#define PROD_BRD_V1_ERR_LED 5

typedef enum ADCProviders {
    NoADC,
    ADS111X,
    ESP32OnBoard
} ADCProviders_t;

class ConfigPins
{
public:
    int8_t NumberRelays = 0;
    int8_t K1Ctl = -1;
    int8_t K2Ctl = -1;
    int8_t K3Ctl = -1;
    int8_t K4Ctl = -1;
    int8_t K5Ctl = -1;
    int8_t K6Ctl = -1;
    int8_t K7Ctl = -1;
    int8_t K8Ctl = -1;

    int8_t SimRx = -1;
    int8_t SimTx = -1;    

    int8_t ConsoleRx = CONSOLE_RX;
    int8_t ConsoleTx = CONSOLE_TX;    

    int8_t CANTx = -1;
    int8_t CANRx = -1;

    int8_t Scl1 = GP0O1_BRD_V2_SCL1;
    int8_t Sda1 = GP0O1_BRD_V2_SDA1;

    int8_t CTChannel1 = -1;
    int8_t CTChannel2 = -1;
    int8_t CTChannel3 = -1;

    ADCProviders_t ADCProvider = NoADC;
    uint8_t ADCChannels = 0;
    
    int8_t ADCChannel1 = -1;
    int8_t ADCChannel2 = -1;
    int8_t ADCChannel3 = -1;
    int8_t ADCChannel4 = -1;
    int8_t ADCChannel5 = -1;
    int8_t ADCChannel6 = -1;
    int8_t ADCChannel7 = -1;
    int8_t ADCChannel8 = -1;

    uint8_t GpioChannels = 0;
    int8_t Gpio1 = -1;
    int8_t Gpio2 = -1;
    int8_t Gpio3 = -1;
    int8_t Gpio4 = -1;
    int8_t Gpio5 = -1;
    int8_t Gpio6 = -1;
    int8_t Gpio7 = -1;
    int8_t Gpio8 = -1;

    
    
    bool InvertGpio1 = false;
    bool InvertGpio2 = false;
    bool InvertGpio3 = false;
    bool InvertGpio4 = false;
    bool InvertGpio5 = false;
    bool InvertGpio6 = false;
    bool InvertGpio7 = false;
    bool InvertGpio8 = false;

    int8_t ErrorLED = -1;
    int8_t OnlineLED = -1;
    int8_t Buzzer = -1;
    int8_t CANActivity = -1;

    bool InvertLED = false;

    int8_t ModemResetPin = -1;
    int8_t ModemSleepPin = -1;
    bool  InvertModemPower = false;

    bool HasDisplay;
    bool HasRelays;
    bool HasLeds = false;
    bool HasI2C = true;

public:
    ConfigPins()
    {
    }

    void init(uint8_t boardType){
        if(boardType == 0) {
            NumberRelays = 0;
            K1Ctl = GP0O1_BRD_V2_K4_CTL;
            K2Ctl = GP0O1_BRD_V2_K3_CTL;
            K3Ctl = GP0O1_BRD_V2_K2_CTL;
            K4Ctl = GP0O1_BRD_V2_K1_CTL;
            K5Ctl = -1;

            SimRx = GP0O1_BRD_V2_GPRS_RX;
            SimTx = GP0O1_BRD_V2_GPRS_TX;

            Scl1 = GP0O1_BRD_V1_SCL1;
            Sda1 = GP0O1_BRD_V1_SDA1;

            ADCProvider = ADS111X;
            ADCChannels = 8;
            ADCChannel1 = GP0O1_BRD_V2_ADC1;
            ADCChannel2 = GP0O1_BRD_V2_ADC2;
            ADCChannel3 = GP0O1_BRD_V2_ADC3;
            ADCChannel4 = GP0O1_BRD_V2_ADC4;
            ADCChannel5 = GP0O1_BRD_V2_ADC5;
            ADCChannel6 = GP0O1_BRD_V2_ADC6;
            ADCChannel7 = GP0O1_BRD_V2_ADC7;
            ADCChannel8 = GP0O1_BRD_V2_ADC8;

            GpioChannels = 8;
            Gpio1 = GP0O1_BRD_V1_IO1;
            Gpio2 = GP0O1_BRD_V1_IO2;
            Gpio3 = GP0O1_BRD_V1_IO3;
            Gpio4 = GP0O1_BRD_V1_IO4;
            Gpio5 = GP0O1_BRD_V1_IO5;
            Gpio6 = GP0O1_BRD_V1_IO6;
            Gpio7 = GP0O1_BRD_V1_IO7;
            Gpio8 = GP0O1_BRD_V1_IO8;

            HasDisplay = true;
            HasRelays = true;
        }
        else if(boardType == 1) {
            K1Ctl = GP0O1_BRD_V2_K4_CTL;
            K2Ctl = GP0O1_BRD_V2_K3_CTL;
            K3Ctl = GP0O1_BRD_V2_K2_CTL;
            K4Ctl = GP0O1_BRD_V2_K1_CTL;

            SimRx = GP0O1_BRD_V2_GPRS_RX;
            SimTx = GP0O1_BRD_V2_GPRS_TX;

            Scl1 = GP0O1_BRD_V2_SCL1;
            Sda1 = GP0O1_BRD_V2_SDA1;

            ADCProvider = ADS111X;
            ADCChannels = 8;
            ADCChannel1 = GP0O1_BRD_V2_ADC1;
            ADCChannel2 = GP0O1_BRD_V2_ADC2;
            ADCChannel3 = GP0O1_BRD_V2_ADC3;
            ADCChannel4 = GP0O1_BRD_V2_ADC4;
            ADCChannel5 = GP0O1_BRD_V2_ADC5;
            ADCChannel6 = GP0O1_BRD_V2_ADC6;
            ADCChannel7 = GP0O1_BRD_V2_ADC7;
            ADCChannel8 = GP0O1_BRD_V2_ADC8;

            ModemResetPin = PROD_BRD_MODEM_RESET_V2;
            InvertModemPower = true;

            GpioChannels = 8;
            Gpio1 = GP0O1_BRD_V2_IO1;
            Gpio2 = GP0O1_BRD_V2_IO2;
            Gpio3 = GP0O1_BRD_V2_IO3;
            Gpio4 = GP0O1_BRD_V2_IO4;
            Gpio5 = GP0O1_BRD_V2_IO5;
            Gpio6 = GP0O1_BRD_V2_IO6;
            Gpio7 = GP0O1_BRD_V2_IO7;
            Gpio8 = GP0O1_BRD_V2_IO8;

            HasDisplay = true;
            HasRelays = true;
        }
        else if(boardType == 2) {
            Scl1 = PROD_BRD_V1_SCL1;
            Sda1 = PROD_BRD_V1_SDA1;


#ifdef CAN_ENABLED
            SimRx  = PROD_BRD_V1_GPRS_RX;
            SimTx = PROD_BRD_V1_GPRS_TX;
            CANRx = 5;
            CANTx = 17;
#else
            SimRx = PROD_BRD_V1_GPRS_RX;
            SimTx = PROD_BRD_V1_GPRS_TX;       
            ConsoleRx = CONSOLE_RX;
            ConsoleTx = CONSOLE_TX;         
#endif

            ADCProvider = ADS111X;
            ADCChannels = 8;
            ADCChannel1 = PROD_BRD_V1_ADC1;
            ADCChannel2 = PROD_BRD_V1_ADC2;
            ADCChannel3 = PROD_BRD_V1_ADC3;
            ADCChannel4 = PROD_BRD_V1_ADC4;
            ADCChannel5 = PROD_BRD_V1_ADC5;
            ADCChannel6 = PROD_BRD_V1_ADC6;
            ADCChannel7 = PROD_BRD_V1_ADC7;
            ADCChannel8 = PROD_BRD_V1_ADC8;

            GpioChannels = 8;
            Gpio1 = PROD_BRD_V1_IO1;
            Gpio2 = PROD_BRD_V1_IO2;
            Gpio3 = PROD_BRD_V1_IO3;
            Gpio4 = PROD_BRD_V1_IO4;
            Gpio5 = PROD_BRD_V1_IO5;
            Gpio6 = PROD_BRD_V1_IO6;
            Gpio7 = PROD_BRD_V1_IO7;
            Gpio8 = PROD_BRD_V1_IO8;

            ModemResetPin = PROD_BRD_MODEM_RESET_V2;

            HasDisplay = false;
            HasRelays = false;
            HasLeds = true;

            ErrorLED = 19;
            OnlineLED = 23;
            Buzzer = 15;
        }
        else if(boardType == 3) { /* large relay board */
            NumberRelays = 5;

            K1Ctl = 25;
            K2Ctl = 33;
            K3Ctl = 26;
            K4Ctl = 27;
            K5Ctl = 15;

            ConsoleRx = CONSOLE_RX;
            ConsoleTx = CONSOLE_TX;    

            Scl1 = PROD_BRD_V1_SCL1;
            Sda1 = PROD_BRD_V1_SDA1;

            SimRx = PROD_BRD_V1_RX;
            SimTx = PROD_BRD_V1_TX;

            ADCProvider = ADS111X;
            ADCChannels = 8;
            ADCChannel1 = PROD_BRD_V1_ADC1;
            ADCChannel2 = PROD_BRD_V1_ADC2;
            ADCChannel3 = PROD_BRD_V1_ADC3;
            ADCChannel4 = PROD_BRD_V1_ADC4;
            ADCChannel5 = PROD_BRD_V1_ADC5;
            ADCChannel6 = PROD_BRD_V1_ADC6;
            ADCChannel7 = PROD_BRD_V1_ADC7;
            ADCChannel8 = PROD_BRD_V1_ADC8;

            ModemResetPin = PROD_BRD_MODEM_RESET_V2;
            
            GpioChannels = 8;
            Gpio1 = PROD_BRD_V1_IO1;
            Gpio2 = PROD_BRD_V1_IO2;
            Gpio3 = PROD_BRD_V1_IO3;
            Gpio4 = PROD_BRD_V1_IO4;
            Gpio5 = PROD_BRD_V1_IO5;
            Gpio6 = PROD_BRD_V1_IO6;
            Gpio7 = PROD_BRD_V1_IO7;
            Gpio8 = PROD_BRD_V1_IO8;

            HasDisplay = false;
            HasRelays = true;
            HasLeds = true;

            ErrorLED = 19;
            OnlineLED = 23;
            Buzzer = 15;
        } else if(boardType == 4) {
            NumberRelays = 0;

            ConsoleRx = CONSOLE_RX;
            ConsoleTx = CONSOLE_TX;    

            SimRx = 18;
            SimTx = 17;

            Gpio1 = 25;
            Gpio2 = 26;

            ModemResetPin = 02;
            
            ADCProvider = NoADC;
            HasDisplay = false;
            HasRelays = false;
            HasLeds = true;
            HasI2C = false;

            InvertLED = false;
            ErrorLED = 33;
            OnlineLED = 32;
        } else if(boardType == 5) {
            ConsoleRx = CONSOLE_RX;
            ConsoleTx = CONSOLE_TX;    

            HasI2C = false;
            HasDisplay = false;
            HasRelays = true;
            HasLeds = false;
            NumberRelays = 8;

            ADCProvider = NoADC;
            K1Ctl = 32;
            K2Ctl = 33;
            K3Ctl = 25;
            K4Ctl = 26;
            K5Ctl = 27;
            K6Ctl = 14;
            K7Ctl = 12;
            K8Ctl = 13;

            Gpio1 = 4;
            Gpio2 = 17;
            Gpio3 = 18;

            OnlineLED = 23;
            ErrorLED = -1;
        } else if(boardType == 6) {
            NumberRelays = 0;

            ConsoleRx = CONSOLE_RX;
            ConsoleTx = CONSOLE_TX;    

            SimRx = 19;
            SimTx = 18;

            Gpio1 = 25;
            Gpio2 = 26;

            ModemResetPin = 02;
            
            ADCProvider = NoADC;
            HasDisplay = false;
            HasRelays = false;
            HasLeds = true;
            HasI2C = false;

            InvertLED = false;
            ErrorLED = 33;
            OnlineLED = 32;
         } else if(boardType == 7) {
            ConsoleRx = 3;
            ConsoleTx = 1;    

            GpioChannels = 3;
            Gpio1 = 25;
            Gpio2 = 26;
            Gpio3 = 27;

            ADCProvider = ESP32OnBoard;
            ADCChannels = 3;
            ADCChannel1 = 34;
            ADCChannel2 = 35;
            ADCChannel3 = 32;

            ModemResetPin = 14;
            ModemSleepPin = 16;

            SimRx = 18;
            SimTx = 17;

            NumberRelays = 0;
            
            HasDisplay = false;
            HasRelays = false;
            HasLeds = true;
            HasI2C = false;

            InvertLED = false;
            ErrorLED = 33;
            OnlineLED = 13;
            CANActivity = 21;

            CANTx = 19;
            CANRx = 5;
        } else if(boardType == 8) {
            NumberRelays = 0;

            ConsoleRx = CONSOLE_RX;
            ConsoleTx = CONSOLE_TX;    
        
            ADCProvider = NoADC;
            HasDisplay = false;
            HasRelays = false;
            HasLeds = false;
            HasI2C = false;
        }
    }
};

#endif
