#ifndef OBJECTS_H
#define OBJECTS_H

#include <Arduino.h>
#include "Display.h"
#include "NuvIoTState.h"
#include "Hal.h"
#include "IOConfig.h"
#include "SysConfig.h"
#include "PowerSensor.h"
#include "OtaServices.h"
#include "RelayManager.h"
#include "Console.h"
#include "Channel.h"
#include "MQTT_GPRS.h"
#include "SIMModem.h"
#include "NuvIoTClient.h"

#include "ADC.h"
#include "TemperatureProbes.h"
#include "PulseCounter.h"
#include "OnOffDetector.h"

#include "WiFiConnectionHelper.h"

#include "Telemetry.h"
#include "RelayManager.h"
#include "BluetoothSerial.h"
#include "ConfigPins.h"
#include "LedManager.h"
#include "NuvIoTMQTT.h"
#include <PubSubClient.h>

#ifdef PROD_BRD_V1
HardwareSerial gprsPort(0);
HardwareSerial consoleSerial(1);
TwoWire twoWire(1);
#define BOARD_CONFIG 2
#endif

#ifdef GPIO_BRD_V1
HardwareSerial gprsPort(1);
HardwareSerial consoleSerial(0);
TwoWire twoWire(1);
#define BOARD_CONFIG 0
#endif

#ifdef GPIO_BRD_V2
HardwareSerial gprsPort(1);
HardwareSerial consoleSerial(0);
TwoWire twoWire(1);
#define BOARD_CONFIG 1
#endif

IOConfig ioConfig;
SysConfig sysConfig;
ConfigPins configPins;

Hal hal;
BluetoothSerial btSerial;
Console console(&btSerial, &consoleSerial);

Display display(DISPLAY_U8G);
LedManager ledManager(&console, &configPins);
NuvIoTState state(&display, &ioConfig, &sysConfig, &ledManager, &btSerial, &SPIFFS, &hal, &console);

MessagePayload *payload = new MessagePayload();
GPSData *gps = NULL;

Channel channel(&gprsPort, &console);

MQTT mqtt(&channel, &console);

SIMModem modem(&display, &channel, &console);

WiFiClient wifiClient;
WiFiConnectionHelper wifiMgr(&wifiClient, &display, &state, &sysConfig);

OtaServices ota(&display, &console, &modem, &hal);
NuvIoTClient client(&modem, &mqtt, &console, &display, &ledManager, &state, &sysConfig, &ota, &hal);

NuvIoTMQTT wifiMqtt(&wifiMgr, &console, &wifiClient, &display, &ota, &hal, &state, &sysConfig);

Telemetry telemetry(&btSerial);

// drivers
PulseCounter pulseCounter(&console, &configPins);
ADC adc(&twoWire, &configPins, &console, payload);
TemperatureProbes probes(&console, &configPins, payload);

RelayManager relayManager(&console, &configPins);
OnOffDetector onOffDetector(&console, &configPins);

PowerSensor powerSensor(&adc, &configPins, &console, payload, &state);

void configureI2C()
{
    if (!twoWire.begin(configPins.Sda1, configPins.Scl1, 400000))
    {
        display.drawStr("Could not start I2C.");
        console.println("Could not start I2C on pins 21 and 22");

        while (true)
        {
            console.println("ic2=initfail;");
            delay(1000);
        }
    }
    else
    {
        console.println("sda=" + String(configPins.Sda1) + ";");
        console.println("scl=" + String(configPins.Scl1) + ";");
        console.println("i2c=initialized;");
    }
}

void configureFileSystem()
{
    if (!SPIFFS.begin(true))
    {
        display.drawStr("Could not initialize SPIFFS.");
        console.println("spiff=fail;");
        while (true)
        {
            console.println("spiff=fail;");
            delay(1000);
        }
    }
    else
    {
        console.println("spiff=initialized;");
    }
}

void welcome(String firmwareSKU, String version)
{
    console.println("WELCOME");
    console.println("SOFTWARE LOGISTICS FIRMWARE");
    console.println("SKU    : " + firmwareSKU);
    console.println("VERSION: " + version);
    delay(1000);
    console.println("Continue Startup");
}

void initDisplay()
{
    display.enable(configPins.HasDisplay);
    if (configPins.HasDisplay)
    {
        display.prepare();
        display.setTextSize(1);
        display.clearBuffer();
        display.drawStr("WELCOME");
        console.println("headless=false;");
    }
    else
    {
        console.println("headless=true;");
    }
}
#endif