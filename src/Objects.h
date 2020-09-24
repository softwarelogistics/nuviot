#ifndef OBJECTS_H
#define OBJECTS_H

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
#endif

#ifdef GPIO_BRD_V2
HardwareSerial gprsPort(1);
HardwareSerial consoleSerial(0);
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
TwoWire twoWire(1);

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

PulseCounter pulseCounter(&console, &configPins);
ADC adc(&twoWire, &configPins, &console, payload);
TemperatureProbes probes(&console, &configPins, payload);

RelayManager relayManager(&console, &configPins);
OnOffDetector onOffDetector(&console, &configPins);



#endif