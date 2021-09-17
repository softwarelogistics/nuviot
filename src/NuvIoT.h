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
#include "BLE.h"
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
#include "BluetoothServer.h"

#include "ConfigPins.h"
#include "LedManager.h"
#include "NuvIoTMQTT.h"
#include "Rest.h"
#include <PubSubClient.h>

#define DEFAULT_BRD

#ifdef PROD_BRD_V1
#undef DEFAULT_BRD
HardwareSerial gprsPort(0);
HardwareSerial consoleSerial(1);
TwoWire twoWire(1);
#define BOARD_CONFIG 2
#endif

#ifdef RELAY_BRD_V1
#undef DEFAULT_BRD
HardwareSerial gprsPort(1);
HardwareSerial consoleSerial(0);
TwoWire twoWire(1);
#define BOARD_CONFIG 3
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

Console console(&consoleSerial);

IOConfig ioConfig(&console);
SysConfig sysConfig(&console);
ConfigPins configPins;

Hal hal;


Display display(DISPLAY_U8G);
LedManager ledManager(&console, &configPins);
NuvIoTState state(&display, &ioConfig, &sysConfig, &ledManager, &SPIFFS, &hal, &console);

MessagePayload *payload = new MessagePayload();
GPSData *gps = NULL;

Channel channel(&gprsPort, &console);
SIMModem modem(&display, &channel, &console, &hal);
OtaServices ota(&display, &console, &modem, &hal);

WiFiClient wifiClient;
WiFiConnectionHelper wifiMgr(&wifiClient, &display, &ledManager, &state, &hal, &console, &sysConfig);

MQTT cellMQTT(&channel, &console);
NuvIoTMQTT wifiMQTT(&wifiMgr, &console, &wifiClient, &display, &ota, &hal, &state, &sysConfig);

NuvIoTClient client(&modem, &wifiMgr, &cellMQTT, &wifiMQTT, &console, &display, &ledManager, &state, &sysConfig, &ota, &hal);

Rest rest(&client, &display, &modem, &wifiMgr, &sysConfig, &console);

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

void handleConsoleCommand(String cmd)
{
  console.println("SENDING OFF COMMAND [" + cmd + "]");
  state.handleConsoleCommand(cmd);
}

void configureModem(unsigned long baudRate = 115200)
{  
  console.println("modem=configuring; // initial baud rate: " + String(baudRate) + ", RX: " + String(configPins.SimRx) + ", TX" + String(configPins.SimTx));

  delay(500);
  //gprsPort.begin(baudRate, SERIAL_8N1, configPins.SimRx, configPins.SimTx);
  gprsPort.begin(baudRate, SERIAL_8N1);//, configPins.SimRx, configPins.SimTx);

  console.println("channel:resizebuffer; // " + String(gprsPort.setRxBufferSize(32000))) ;
  delay(500);
//  gprsPort.setRxBufferSize(16 * 1024);

  if(configPins.ModemResetPin != -1){
    console.println("modem=settingpowerkey; // pin: " + String(configPins.ModemResetPin));

    pinMode(configPins.ModemResetPin, OUTPUT);
    digitalWrite(configPins.ModemResetPin, LOW);
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
  if (sysConfig.WiFiEnabled)
  {
    if (!state.getIsConfigurationModeActive() && client.WifiConnect(reconnect))
    {
      wifiMQTT.addSubscriptions("nuviot/paw/" + sysConfig.DeviceId + "/#");
      return;
    }
  }
  else if (sysConfig.CellEnabled)
  {
    while (state.isValid())
    {
      if (!state.getIsConfigurationModeActive() && client.CellularConnect(reconnect, baud))
      {
        cellMQTT.subscribe("nuviot/paw/" + sysConfig.DeviceId + "/#", QOS0);

        if (sysConfig.GPSEnabled)
        {
          modem.startGPS();
        }

        return;
      }
    }
  }
  else if(sysConfig.GPSEnabled) {
    modem.startGPS();
  }  
  #endif
}

GPSData *readGPS() {
  return modem.readGPS();
}

#ifdef BOARD_CONFIG
void initPins()
{
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
void configureConsole(unsigned long baud = 115200, bool serialEnabled = true, bool btEnabled = true)
{
  consoleSerial.begin(baud, SERIAL_8N1, configPins.ConsoleRx, configPins.ConsoleTx);
  console.println("Pins on startup: " + String(configPins.ConsoleRx) + " " + String(configPins.ConsoleTx));
  console.enableSerialOut(serialEnabled);
  console.enableBTOut(btEnabled);
  console.registerCallback(handleConsoleCommand);
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

  connect(true);

  console.println("connection=reestablished;");
}

void handleError(String err, String details)
{
  if (sysConfig.WiFiEnabled)
  {
    wifiMQTT.publish("nuviot/srvr/dvcsrvc/" + sysConfig.DeviceId + "/err/" + err + "/raise", details);
  }
  else if (sysConfig.CellEnabled)
  {
    cellMQTT.publish("nuviot/srvr/dvcsrvc/" + sysConfig.DeviceId + "/err/" + err + "/raise", details, QOS0);
  }
}

void clearError(String err, String details)
{
  if (sysConfig.WiFiEnabled)
  {
    wifiMQTT.publish("nuviot/srvr/dvcsrvc/" + sysConfig.DeviceId + "/err/" + err + "/clear", details);
  }
  else if (sysConfig.CellEnabled)
  {
    cellMQTT.publish("nuviot/srvr/dvcsrvc/" + sysConfig.DeviceId + "/err/" + err + "/clear", details, QOS0);
  }
}

long lastPing = 0;

void ping()
{
  if (lastPing == 0 || ((millis() - lastPing) > sysConfig.PingRate * 1000))
  {
    lastPing = millis();

    if (!cellMQTT.ping())
    {
      reconnect();
      lastPing = 0;
    }

    console.println("transmit=ping;");
    ledManager.setOnlineFlashRate(-1);
  }

  if (cellMQTT.getIsClosed())
  {
    // Reconnect will either return connected or resteart the device.
    reconnect();
    lastPing = 0;
  }
}

void mqttCallback(String topic, byte *buffer, size_t len) {
  console.println("RECIVE TOPIC" + topic);
}

void commonLoop()
{
  if (sysConfig.WiFiEnabled)
  {
    wifiMgr.loop();
    if( wifiMgr.isConnected() && sysConfig.SrvrHostName != NULL && sysConfig.SrvrHostName.length() > 0) {      
      wifiMQTT.loop();    
    }
  }
  else if (sysConfig.CellEnabled)
  {
    cellMQTT.loop();    
    ping();
  }
  
  state.loop();
}

void mqttSubscribe(String topic) {
  if (sysConfig.WiFiEnabled)
  {
    wifiMQTT.addSubscriptions(topic);
  }
  else if (sysConfig.CellEnabled)
  {
    cellMQTT.subscribe(topic, QOS0);
  }

  wifiMQTT.setMessageReceivedCallback(mqttCallback);
  cellMQTT.setMessageReceivedCallback(mqttCallback);
}

void mqttPublish(String topic, String value)
{
  if (sysConfig.WiFiEnabled)
  {
    wifiMQTT.publish(topic, value);
  }
  else if (sysConfig.CellEnabled)
  {
    cellMQTT.publish(topic, value, QOS0);
  }
}

void mqttPublish(String topic){
  mqttPublish(topic, "");
}


