#ifndef OBJECTS_H
#define OBJECTS_H

#include "Display.h"
#include "TemperatureProbes.h"
#include "NuvIoTState.h"
#include "Hal.h"
#include "ADC.h"
#include "IOConfig.h"
#include "PowerSensor.h"
#include "OtaServices.h"
#include "RelayManager.h"
#include "Console.h"
#include "Channel.h"
#include "MQTT_GPRS.h"
#include "SIMModem.h"
#include "NuvIoTClient.h"
#include "PulseCounter.h"
#include "Telemetry.h"
#include "OnOffDetector.h"
#include "RelayManager.h"
#include "IOConfig.h"
#include "SysConfig.h"
#include "BluetoothSerial.h"
#include "ConfigPins.h"

IOConfig ioConfig;
SysConfig sysConfig;
ConfigPins configPins(1);

Hal hal;
BluetoothSerial btSerial;
Console console(&btSerial, &Serial);

HardwareSerial gprsPort(1);
Display display(DISPLAY_U8G);
NuvIoTState state(&display, &ioConfig, &btSerial, &SPIFFS, &hal, &console);
TwoWire twoWire(1);

MessagePayload *payload = new MessagePayload();
GPSData *gps = NULL;

Channel channel(&gprsPort, &console);
MQTT mqtt(&channel, &console);
SIMModem modem(&display, &channel, &console);

OtaServices ota(&display, &console, &modem, &hal);
NuvIoTClient client(&modem, &mqtt, &console, &display, &state, &ota, &hal);

Telemetry telemetry(&btSerial);

PulseCounter pulseCounter(&console, &configPins);
ADC adc(&twoWire, &configPins, &console, payload);
TemperatureProbes probes(&console, &configPins, payload);

RelayManager relayManager(&console, &configPins);
OnOffDetector onOffDetector(&console, &configPins);

#endif