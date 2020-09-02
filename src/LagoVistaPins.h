#ifndef PINS_CONFIG_H
#define PINS_CONFIG_H

#include <Arduino.h>

#ifdef GPIO_BRD_V2
#define K4_CTL 13
#define K3_CTL 12
#define K2_CTL 14
#define K1_CTL 27

#define SIM_RX 23
#define SIM_TX 19

#define SCL1 22
#define SDA1 21

#define DHT1 17
#define DHT2 5
#define DHT3 18

#define MS18B 2

// Bank One
#define ADC8 0
#define ADC1 7
#define ADC4 2

// Bank Two
#define ADC5 4
#define ADC6 5
#define ADC2 6
#define ADC3 1
#define ADC7 3

#define GPIO0 0
#define GPIO1 1
#define GPIO2 2
#define GPIO3 3
#define GPIO4 4
#define GPIO5 5
#define GPIO6 6
#define GPIO7 7

#endif

#ifdef PROD_BRD_V1

#define ADC_BATT 0
#define ADC1 5
#define ADC2 4
#define ADC3 6
#define ADC4_CT1 0
#define ADC5_CT2 1
#define ADC6_CT3 2


#endif


class PinsConfig {
    uint8_t K1Ctl = K1_CTL;
    uint8_t K2Ctl = K2_CTL;
    uint8_t K3Ctl = K3_CTL;
    uint8_t K4Ctl = K4_CTL;

    uint8_t SimRx = SIM_RX;
    uint8_t SimTx = SIM_TX;

    uint8_t Scl1 = SCL1;
    uint8_t Sda1 = SDA1;

    uint8_t ADCChannel1 = ADC1;
    uint8_t ADCChannel2 = ADC2;
    uint8_t ADCCHannel3 = ADC3;
    uint8_t ADCChannel4 = ADC4;
    uint8_t ADCChannel5 = ADC5;
    uint8_t ADCChanenl6 = ADC6;
    uint8_t ADCChannel7 = ADC7;
    uint8_t ADCChannel8 = ADC8;    

    uint8_t Gpio0 = GPIO0;
    uint8_t Gpio1 = GPIO1;
    uint8_t Gpio2 = GPIO2;
    uint8_t Gpio3 = GPIO3;
    uint8_t Gpio4 = GPIO4;
    uint8_t Gpio5 = GPIO5;
    uint8_t Gpio6 = GPIO6;
    uint8_t Gpio7 = GPIO7;
};

#endif

