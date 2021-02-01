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

#define DEFAULT_BRD

#ifdef PROD_BRD_V1
#undef DEFAULT_BRD
HardwareSerial gprsPort(1);
HardwareSerial consoleSerial(0);
TwoWire twoWire(1);
#define BOARD_CONFIG 2
#endif

#ifdef GPIO_BRD_V1
#undef DEFAULT_BRD
HardwareSerial gprsPort(1);
HardwareSerial consoleSerial(0);
TwoWire twoWire(1);
#define BOARD_CONFIG 1
#endif

#ifdef GPIO_BRD_V2
#undef DEFAULT_BRD
HardwareSerial gprsPort(1);
HardwareSerial consoleSerial(0);
TwoWire twoWire(1);
#define BOARD_CONFIG 1
#endif

#ifdef DEFAULT_BRD
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

#ifdef CELLULAR
Channel channel(&gprsPort, &console);
SIMModem modem(&display, &channel, &console, &hal);
OtaServices ota(&display, &console, &modem, &hal);
MQTT mqtt(&channel, &console);
NuvIoTClient client(&modem, &mqtt, &console, &display, &ledManager, &state, &sysConfig, &ota, &hal);
#endif

#ifdef WIFI
WiFiClient wifiClient;
OtaServices ota(&display, &console, &hal);
WiFiConnectionHelper wifiMgr(&wifiClient, &display, &state, &console, &sysConfig);
NuvIoTMQTT mqtt(&wifiMgr, &console, &wifiClient, &display, &ota, &hal, &state, &sysConfig);
NuvIoTClient client(&wifiMgr, &mqtt, &console, &display, &ledManager, &state, &sysConfig, &ota, &hal);
#endif

Telemetry telemetry(&btSerial);

// drivers
PulseCounter pulseCounter(&console, &configPins);
ADC adc(&twoWire, &state, &configPins, &console, &display, payload);
TemperatureProbes probes(&console, &configPins, payload);

RelayManager relayManager(&console, &configPins);
OnOffDetector onOffDetector(&console, &configPins);

PowerSensor powerSensor(&adc, &configPins, &console, &display, payload, &state);

void configureI2C()
{
  if (!twoWire.begin(configPins.Sda1, configPins.Scl1, 400000))
  {
    while (true)
    {
      console.println("ic2=initfail; Pins SDA=" + String(configPins.Sda1) + ", SCL=" + String(configPins.Scl1) + ".");
      delay(1000);
    }
  }
  else
  {
    console.println("i2c=initialized; Pins SDA=" + String(configPins.Sda1) + ", SCL=" + String(configPins.Scl1) + ".");
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

void configureModem(unsigned long baudRate = 115200)
{  
  console.println("modem=configuring; // initial baud rate: " + String(baudRate));
  delay(500);
  gprsPort.begin(baudRate, SERIAL_8N1, configPins.SimRx, configPins.SimTx);
  delay(500);
  gprsPort.setRxBufferSize(16 * 1024);
  console.println("modem=configured; // initial baud rate: " + String(baudRate));
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

void loadConfigurations()
{
  ioConfig.load();
  sysConfig.load();
}

void initDisplay(String firmwareSKU, String version)
{
  display.enable(configPins.HasDisplay);
  if (configPins.HasDisplay)
  {
    display.prepare();
    display.setTextSize(1);
    display.clearBuffer();
    display.drawStr("WELCOME", firmwareSKU.c_str(), version.c_str());
    console.println("headless=false;");
  }
  else
  {
    console.println("headless=true;");
  }
}

void spinWhileNotCommissioned()
{
  long lastMsg = 0;

  while (!sysConfig.Commissioned)
  {
    if (millis() - lastMsg > 1000)
    {
      console.println("commissioned=false;");
      lastMsg = millis();
    }
    state.loop();
  }
}

#define MAX_RETRY_ATTEMPTS 4

//TODO: Not a lot of value here...
void connect(bool reconnect = false, unsigned long baud = 115200)
{
  #ifdef WIFI
    if (!state.getIsConfigurationModeActive() && client.WifiConnect(reconnect))
    {
      mqtt.addSubscriptions("nuviot/paw/" + sysConfig.DeviceId + "/#");
      return;
    }
  #endif
  #ifdef CELLULAR
  while (state.isValid())
  {
    if (!state.getIsConfigurationModeActive() && client.CellularConnect(reconnect, baud))
    {
      mqtt.subscribe("nuviot/paw/" + sysConfig.DeviceId + "/#", QOS0);

      if (sysConfig.GPSEnabled)
      {
        modem.startGPS();
      }

      return;
    }
  }
  #endif
}

#ifdef BOARD_CONFIG
void initPins() {
  configPins.init(BOARD_CONFIG);
}
#endif

/**
 * \brief Setup the console.
 * 
 * \param baud Baud rate for serial console (default 115200)
 * \param serialEnabled Use the confifugred serial port for transmitting data.
 * \param btEnabled Use Bluetooth Serial to send data.
 * 
 **/
void configureConsole(unsigned long baud = 115200, bool serialEnabled = true, bool btEnabled= true)
{
  consoleSerial.begin(baud, SERIAL_8N1);
  console.enableSerialOut(serialEnabled);
  console.enableBTOut(btEnabled);
}

void sendStatusUpdate(String currentState, String nextAction, String title = "Commo Starting", int afterDelay = 0)
{
  display.drawStr(title.c_str(), currentState.c_str());
  delay(1000);
  display.drawStr(title.c_str(), nextAction.c_str());

  if (afterDelay > 0)
  {
    delay(afterDelay);
  }
}

void reconnect()
{
  console.println("connection=lost;");
  console.println("connection=reconnecting;");

  sendStatusUpdate("No MQTT Connection", "Restarting", "Message Loop", 1000);

  int retryCount = 0;
  while (!client.CellularConnect(true, sysConfig.GPRSModemBaudRate))
  {
    retryCount++;
    console.println("connection=failreconnect; // connection attempt " + String(retryCount));
    if (retryCount == MAX_RETRY_ATTEMPTS)
    {
      hal.restart();
    }
    state.loop();
  }

  console.println("connection=reestablished;"); 
  return;
}
#endif