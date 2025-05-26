  #ifndef OBJECTS_H
#define OBJECTS_H

#include <Arduino.h>

#ifdef LCD_DISPLAY
#include "Display.h"
#endif

#include "NuvIoTState.h"
#include "Hal.h"
#include "IOConfig.h"
#include "IOValues.h"
#include "SysConfig.h"
#include "PowerSensor.h"
#include "OtaServices.h"
#include "RelayManager.h"
#include "BLE.h"
#include "Console.h"
#include "Channel.h"
#ifdef CELLULAR_ENABLED
#include "MQTT_GPRS.h"
#include "SIMModem.h"
#include "Rest.h"
#endif

#include "NuvIoTClient.h"

#include "ADC.h"
#include "TemperatureProbes.h"
#include "PulseCounter.h"
#include "OnOffDetector.h"

#include "CANBus.h"
#include "WiFiConnectionHelper.h"

#include "Telemetry.h"
#include "RelayManager.h"
#include "BluetoothServer.h"

#include "LedManager.h"
#include "NuvIoTMQTT.h"

#include "Pir.h"
#include <PubSubClient.h>

#define DEFAULT_BRD
#define SERIAL_CONSOLE_ENABLED

#ifdef PROD_BRD_V1
#undef DEFAULT_BRD
HardwareSerial gprsPort(0);
HardwareSerial consoleSerial(1);
TwoWire twoWire(1);
#define BOARD_CONFIG 2
#endif

#ifdef PROD_BRD_V1_CAN
#undef DEFAULT_BRD
#ifdef CAN_ENABLED
HardwareSerial gprsPort(1);
HardwareSerial consoleSerial(0);
#undef SERIAL_CONSOLE_ENABLED
#else
HardwareSerial gprsPort(0);
HardwareSerial consoleSerial(1);
#endif
TwoWire twoWire(1);
#define BOARD_CONFIG 2
#endif

#ifdef CAN_BRD_V1
#undef DEFAULT_BRD
HardwareSerial gprsPort(1);
HardwareSerial consoleSerial(0);
TwoWire twoWire(1);
#define BOARD_CONFIG 7
#endif


#ifdef GENERIC
#undef DEFAULT_BRD
HardwareSerial gprsPort(1);
HardwareSerial consoleSerial(0);
TwoWire twoWire(1);
#define BOARD_CONFIG 8
#endif

#ifdef RELAY_BRD_V1
#undef DEFAULT_BRD
HardwareSerial gprsPort(1);
HardwareSerial consoleSerial(0);
TwoWire twoWire(0);
#define BOARD_CONFIG 3
#endif

#ifdef TEMP_SNSR_BOARD_V3
#undef DEFAULT_BRD
HardwareSerial gprsPort(1);
HardwareSerial consoleSerial(0);
TwoWire twoWire(0);
#define BOARD_CONFIG 4
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

#ifdef CHARGE_BOARD_V1
#undef DEFAULT_BRD
HardwareSerial gprsPort(1);
HardwareSerial consoleSerial(0);
TwoWire twoWire(0);
#define BOARD_CONFIG 6
#endif

#ifdef ESP32_Relay_X8
#undef DEFAULT_BRD
HardwareSerial gprsPort(1);
HardwareSerial consoleSerial(0);
TwoWire twoWire(1);
#define BOARD_CONFIG 5
#endif

#ifdef ESP32C3_MINI
#undef DEFAULT_BRD
HardwareSerial gprsPort(1);
HardwareSerial consoleSerial(0);
TwoWire twoWire(1);
#define BOARD_CONFIG 9
#endif

#ifdef HARSH_ENV_V1
#undef DEFAULT_BRD
HardwareSerial gprsPort(1);
HardwareSerial consoleSerial(0);
#define BOARD_CONFIG 10
TwoWire twoWire(1);
#endif

#ifdef DEFAULT_BRD
HardwareSerial gprsPort(1);
HardwareSerial consoleSerial(0);
TwoWire twoWire(1);
#define BOARD_CONFIG 1
#endif

#include "ConfigPins.h"

Console console(&consoleSerial);

IOValues ioValues(&console);

IOConfig ioConfig(&console);
SysConfig sysConfig(&console);
ConfigPins configPins;

Hal hal;

#ifdef LCD_DISPLAY
Display display(DISPLAY_U8G);
#endif



LedManager ledManager(&console, &configPins);

#ifdef LCD_DISPLAY
NuvIoTState state(&display, &ioConfig, &sysConfig, &ledManager, &SPIFFS, &hal, &console);
#else
NuvIoTState state(&ioConfig, &sysConfig, &ledManager, &SPIFFS, &hal, &console);
#endif
MessagePayload *payload = new MessagePayload(&ioValues);

WiFiClient wifiClient;


#ifdef CELLULAR_ENABLED
GPSData *gps = NULL;
Channel channel(&gprsPort, &console);
MQTT cellMQTT(&channel, &console);
#endif


PulseCounter pulseCounter(&console, &configPins, payload);
TemperatureProbes probes(&console, &configPins, payload);
RelayManager relayManager(&console, &configPins);
OnOffDetector onOffDetector(&console, &configPins, payload);

#ifdef LCD_DISPLAY
  #ifdef CELLULAR_ENABLED 
    SIMModem modem(&display, &channel, &console, &hal, &configPins);
  #endif    
    OtaServices ota(&display, &console, &state, &modem, &hal);
WiFiConnectionHelper wifiMgr(&wifiClient, &display, &ledManager, &state, &hal, &console, &sysConfig);
NuvIoTMQTT wifiMQTT(&wifiMgr, &console, &wifiClient, &display, &ota, &hal, &state, &sysConfig);
NuvIoTClient client(&modem, &wifiMgr, &cellMQTT, &wifiMQTT, &console, &display, &ledManager, &state, &sysConfig, &ota, &hal);
#else
  #ifdef CELLULAR_ENABLED 
    SIMModem modem(&channel, &console, &hal, &configPins);
    OtaServices ota(&console, &state, &modem, &hal);
  #else
    OtaServices ota(&console, &state, &hal);
  #endif


WiFiConnectionHelper wifiMgr(&wifiClient, &ledManager, &state, &hal, &console, &sysConfig);
NuvIoTMQTT wifiMQTT(&wifiMgr, &console, &wifiClient, &ota, &hal, &state, &sysConfig);
  #ifdef CELLULAR_ENABLED
    NuvIoTClient client(&modem, &wifiMgr, &cellMQTT, &wifiMQTT, &console, &ledManager, &state, &sysConfig, &ota, &hal);
  #else
    NuvIoTClient client(&wifiMgr, &wifiMQTT, &console, &ledManager, &state, &sysConfig, &ota, &hal);
  #endif
#endif

#ifdef CELLULAR_ENABLED
Rest rest(&client, &modem, &wifiMgr, &sysConfig, &console);
#endif

// drivers
ADC adc(&twoWire, &state, &configPins, &console, payload);
PowerSensor powerSensor(&adc, &configPins, &console, payload, &state);

#ifdef BT_ENABLED
BLE BT(&console, &hal, &state, &ioConfig, &wifiMgr, &sysConfig, &relayManager, &ota, payload);
#endif

PIRSensor pir(&adc, &console, &configPins, payload);

#ifdef CAN_ENABLED
CANBus canBus(&console, &configPins, &BT);
#endif

bool _i2cConfigure = false;

void configureI2C()
{
  if (!twoWire.setPins(configPins.Sda1, configPins.Scl1))
  {
    while (true)
    {
      console.println("i2c=initfail; Pins SDA=" + String(configPins.Sda1) + ", SCL=" + String(configPins.Scl1) + ".");
      delay(1000);
    }
  }
  else
  {
    twoWire.begin();
    console.println("i2c=initialized; Pins SDA=" + String(configPins.Sda1) + ", SCL=" + String(configPins.Scl1) + ".");
    _i2cConfigure = true;
  }
}

void configureFileSystem()
{
  if (!SPIFFS.begin(true))
  {  
#ifdef LCD_DISPLAY    
    display.drawStr("Could not initialize SPIFFS.");
#endif
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

void writeCurrentState() {
    String charValue =
      "currentstate=" + 
      state.getFirmwareSKU() + "," +                   // 0
      state.getDeviceModelKey() + "," +                // 1
      state.getFirmwareVersion() + "," +               // 2
      state.getHardwareRevision() + "," +              // 3
      (sysConfig.Commissioned ? "1," : "0,") +         // 4
      String(state.getWiFiState()) + "," +             // 5
      String(state.getWiFiRSSI()) + "," +              // 6
      state.getWiFiIPAddress()+  "," +                 // 7
      (state.getIsCellConnected() ? "1," : "0,") +     // 8
      String(state.getCellRSSI()) + "," +              // 9
      state.getCellIPAddress()+  "," +                 // 10
      (state.getIsCloudConnected() ? "1," : "0,") +    // 11
      String(state.getInputVoltage()) + "," +          // 12
      (state.getExternalPower() ? "1," : "0,") +       // 13
      String(state.OTAState) + "," +                   // 14
      String(state.OTAParam) + ";";                          // 15
      console.println(charValue);
}

void handleConsoleCommand(String cmd)
{
  if(cmd=="wifi-survey")
    wifiMgr.siteSurvey();
  else if(cmd == "send-state")
    writeCurrentState();
  else if(cmd == "send-sysconfig")
    sysConfig.print();
  else if(cmd == "send-iovalues") 
    console.println("iovalues=" + payload->ioValues->toString() + ";");
  else
    state.handleConsoleCommand(cmd);
}


#ifdef CELLULAR_ENABLED
bool __isModemConfigured = false;

void configureModem(unsigned long baudRate = 115200)
{
  if(__isModemConfigured)
    return;

  console.println("modem=configuring; // initial baud rate: " + String(baudRate) + ", RX: " + String(configPins.SimRx) + ", TX: " + String(configPins.SimTx));
  console.println("channel:resizebuffer; // " + String(gprsPort.setRxBufferSize(32*1024)));
  gprsPort.begin(baudRate, SERIAL_8N1, configPins.SimRx, configPins.SimTx);

  delay(1500);
  modem.alloc();

  if (modem.isModemOnline()) 
    modem.init();

  __isModemConfigured = true;

  delay(1500);
}
#endif

void welcome(String firmwareSKU, String version)
{
  console.println("WELCOME");
  console.println("SOFTWARE LOGISTICS FIRMWARE");
  console.println("SKU        : " + firmwareSKU);
  console.println("APP-VERSION: " + version);
  console.println("LIB-VERSION: " + state.getLibraryVersion());
  switch (BOARD_CONFIG)
  {
  case 1:
    console.println("BOARD 1: GPIO_BRD_V1, GPIO_BRD_V2, DEFAULT");
    break;
  case 2:
    console.println("BOARD 2: PROD_BRD_V1");
    break;
  case 3:
    console.println("BOARD 3: RELAY_BRD_V1");
    break;
  case 4:
    console.println("BOARD 4: REMOTE_TEMP_SENSOR");
    break;
  case 6:
    console.println("BOARD 6: CHARGE_BOARD_V1");
    break;
  case 7:
    console.println("BOARD 7: CAN_BRD_V1");
    break;
  default:
    console.println("BOARD ?: UNKNOWN");
    break;
  }
  console.println("Continue Startup");
  if (sysConfig.DeviceId != NULL && sysConfig.DeviceId.length() > 0)
    console.println("Device Id        : " + String(sysConfig.DeviceId));
  else
    console.println("Device Id        : Not Configured");
  console.println("Config Level     : " + String(sysConfig.ConfigurationLevel));
  console.println("Cellular Enabled : " + String(sysConfig.CellEnabled));
  console.println("WiFi Enabled     : " + String(sysConfig.WiFiEnabled));
  console.println("Commissioned     : " + String(sysConfig.Commissioned));
  console.println("GPS Enabled      : " + String(sysConfig.GPSEnabled));
  console.println("Org Id           : " + String(sysConfig.OrgId));
  console.println("Repo Id          : " + String(sysConfig.RepoId));
  console.println("Id               : " + String(sysConfig.Id));
  console.println("Device Type      : " + String(sysConfig.DeviceTypeId));
  console.println("Customer Id      : " + String(sysConfig.Id));
  console.println("GPS Enabled      : " + String(sysConfig.GPSEnabled));
  
  if (sysConfig.SrvrType != NULL && sysConfig.SrvrType.length() > 0)
    console.println("Server Type     : " + String(sysConfig.SrvrType));
  else
    console.println("Server Type     : Not Configured");

  delay(1000);
}

void loadConfigurations()
{
  ioConfig.load();
  sysConfig.load();
}

#ifdef LCD_DISPLAY
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
#endif

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

// TODO: Not a lot of value here...
void connect(bool reconnect = false, unsigned long baud = 115200)
{
  uint8_t retryCount = 0;
  if (sysConfig.WiFiEnabled)
  {
    if (!state.getIsConfigurationModeActive() && client.WifiConnect(reconnect))
    {
      return;
    }
  }
  
  #ifdef CELLULAR_ENABLED
  else if (sysConfig.CellEnabled)
  {
    while (state.isValid() && retryCount++ < 5)
    {
      if (modem.isModemOnline() && !state.getIsConfigurationModeActive() && client.CellularConnect(reconnect, baud)){
        ledManager.setOnlineFlashRate(-1);
        ledManager.setErrFlashRate(0);
        console.println("cellconnection=established;");
        if (sysConfig.GPSEnabled)
        {
          console.println("cellconnection=startinggps;");
          modem.startGPS();
        }    
        return;
      }
      else 
          delay(500);
    }
  }
  else if (sysConfig.GPSEnabled) // GPS Only
  {
    modem.startGPS();
  }
  #endif
}

#ifdef CELLULAR_ENABLED
GPSData *readGPS()
{
  return modem.readGPS();
}
#endif

#ifdef BOARD_CONFIG
void initPins(){
  configPins.init(BOARD_CONFIG);
}
#endif

/**
 * \brief Setup the c.
 *
 * \param baud Baud rate for serial console (default 115200)
 * \param serialEnabled Use the configured serial port for transmitting data.
 * \param btEnabled Use Bluetooth Serial to send data.
 *
 **/
void configureConsole(unsigned long baud = 115200, bool serialEnabled = true, bool btEnabled = true)
{
#ifdef SERIAL_CONSOLE_ENABLED
  consoleSerial.begin(baud, SERIAL_8N1, configPins.ConsoleRx, configPins.ConsoleTx);
  console.enableSerialOut(serialEnabled);
  console.println("console=initialized; // Baud Rate: " + String(baud) + ", RX: " + String(configPins.ConsoleRx) + ", TX: " + String(configPins.ConsoleTx));
  console.registerCallback(handleConsoleCommand);
#else
  console.enableSerialOut(false);
#endif

  console.enableBTOut(btEnabled);
}

void sendStatusUpdate(String currentState, String nextAction, String title = "Commo Starting", int afterDelay = 0)
{
  #ifdef LCD_DISPLAY
  display.drawStr(title.c_str(), currentState.c_str());
  delay(1000);
  display.drawStr(title.c_str(), nextAction.c_str());
  #endif

  if (afterDelay > 0)
  {
    delay(afterDelay);
  }
}

void reconnect()
{
  console.println("connection=lost;");
  console.println("connection=reconnecting;");
  ledManager.setOnlineFlashRate(1);
  connect(true);

  console.println("connection=reestablished;");
}

void handleError(String err, String details)
{
  if (sysConfig.WiFiEnabled)
  {
    wifiMQTT.publish("nuviot/srvr/dvcsrvc/" + sysConfig.DeviceId + "/err/" + err + "/raise", details);
  }
  #ifdef CELLULAR_ENABLED
  else if (sysConfig.CellEnabled)
  {
    cellMQTT.publish("nuviot/srvr/dvcsrvc/" + sysConfig.DeviceId + "/err/" + err + "/raise", details, QOS0);
  }
  #endif
}

void clearError(String err, String details)
{
  if (sysConfig.WiFiEnabled)
  {
    wifiMQTT.publish("nuviot/srvr/dvcsrvc/" + sysConfig.DeviceId + "/err/" + err + "/clear", details);
  }
#ifdef CELLULAR_ENABLED  
  else if (sysConfig.CellEnabled)
  {
    cellMQTT.publish("nuviot/srvr/dvcsrvc/" + sysConfig.DeviceId + "/err/" + err + "/clear", details, QOS0);
  }
#endif
}

long __nextPing = 0;
long __nextLoop = 0;

#ifdef CELLULAR_ENABLED
bool httpGetNoContent(String url)
{
  return modem.httpGetNoContent(url);
}

String httpGet(String url)
{
  return modem.httpGet(url);
}

String httpPost(String url, String body)
{
  return modem.httpPost(url, body);
}
#endif

#ifdef CELLULAR_ENABLED
void ping()
{
  if (__nextPing < millis())
  {
    __nextPing = millis() + sysConfig.PingRateSecond * 1000;

    if (!cellMQTT.ping())
    {
      state.setIsCloudConnected(false);
      reconnect();
      __nextPing = 0;
    }
    else
    {
      state.setIsCloudConnected(true);
    }

    ledManager.setOnlineFlashRate(-1);
  }
 
  if (cellMQTT.getIsClosed())
  {
    // Reconnect will either return connected or restart the device.
    reconnect();
    __nextPing = 0;
  }
}
#endif

void mqttCallback(String topic, byte *buffer, size_t len)
{
  console.println("mqtt=handltopic; // Topic: " + topic);
}

long __nextSend = 0;
long __nextGPS = 0;

void sendIOValues()
{
  if (sysConfig.Commissioned && __nextSend < millis())
  {

    String pathOrTopic = "nuviot/srvr/dvcsrvc/" + sysConfig.DeviceId + "/iovalues";
  
    __nextSend = millis() + sysConfig.SendUpdateRateMS;

    if (sysConfig.WiFiEnabled && wifiMgr.isConnected())
    {
      if (sysConfig.SrvrType == "mqtt")
      {
        wifiMQTT.publish(pathOrTopic, ioValues.toString());
        console.println("wifimqtt=publish; //" + pathOrTopic);

#ifdef CAN_ENABLED
        CANMessage *pMsg = canBus.getMessageHead();
        while (pMsg != NULL)
        {
          if (!pMsg->transmitted)
          {
            String topic = "can/" + pMsg->getHEXMessageId() + "/" + sysConfig.DeviceId;
            wifiMQTT.publish(topic, pMsg->msg, pMsg->msgLen);
            pMsg->transmitted = true;
          }

          pMsg = pMsg->pNext;
        }
#endif
      }
      else if (sysConfig.SrvrType == "rest")
      {
        wifiMgr.post(sysConfig.SrvrHostName, sysConfig.Port, pathOrTopic, ioValues.toString());
      }
    }
#ifdef  CELLULAR_ENABLED
    else if (sysConfig.CellEnabled)
    {
      if (sysConfig.SrvrType == "mqtt")
      {
        if (!cellMQTT.publish(pathOrTopic, ioValues.toString(), QOS0))
        {
        }

#ifdef CAN_ENABLED
        CANMessage *pMsg = canBus.getMessageHead();
        while (pMsg != NULL)
        {
          if (!pMsg->transmitted)
          {
            String topic = "can/" + pMsg->getHEXMessageId() + "/" + sysConfig.DeviceId;
            cellMQTT.publish(topic, pMsg->msg, pMsg->msgLen, QOS0);
            pMsg->transmitted = true;
          }
          pMsg = pMsg->pNext;
        }
#endif
      }
      else if (sysConfig.SrvrType == "rest")
      {
        String url = "http://" + sysConfig.SrvrHostName + ":" + sysConfig.Port + "/" + pathOrTopic;
        httpPost(url, ioValues.toString());
      }
    }
#endif

    // we may do something to automatically send a message when motion in the future
    // until then we will clear the motion state once it's sent to the server.
    // this will indicate the device saw motion in the interval between messages.
    pir.clearMotion();
  }
}

void communicationsTask(void *param)
{
    if (sysConfig.Commissioned)
    {
      if (sysConfig.WiFiEnabled)
      {
        wifiMgr.loop();
        if (sysConfig.SrvrType == "mqtt"){
          if (wifiMgr.isConnected() && sysConfig.SrvrHostName != NULL && sysConfig.SrvrHostName.length() > 0){
            wifiMQTT.loop();
          }
        }
      }
#ifdef CELLULAR_ENABLED
      else if (sysConfig.CellEnabled)
      {
        if (sysConfig.SrvrType == "mqtt"){
          cellMQTT.loop();
        }
      }      
      else if (sysConfig.CellEnabled)
      {
        if(!__isModemConfigured)
          configureModem();

        if (sysConfig.SrvrType == "mqtt"){
          cellMQTT.loop();
          ping();
        }
      }
      #endif
    }

    #ifdef CELLULAR_ENABLED
    if (sysConfig.GPSEnabled && sysConfig.CellEnabled)
    {
      if (__nextGPS < millis())
      {
        __nextGPS = millis() + sysConfig.GPSUpdateRateMS;

        GPSData *gps = modem.readGPS();
        if (gps != NULL){
          console.println("gps=valid; // " + gps->toCSV());
          if (sysConfig.WiFiEnabled){
            wifiMQTT.publish("nuviot/srvr/dvcsrvc/" + sysConfig.DeviceId + "/geo", gps->toCSV());
          }
          else if (sysConfig.CellEnabled){
            cellMQTT.publish("nuviot/srvr/dvcsrvc/" + sysConfig.DeviceId + "/geo", gps->toCSV(), QOS0);
          }
        }
        else{
          console.println("gps=invalid;");
        }
      }
    }
    #endif
    
    sendIOValues(); 
}

#ifdef BT_ENABLED
void bleTask(void *param) {
  BT.update();
}
#endif

void commonLoop(){
  // timing on these is handled in the method for sending.
  if (sysConfig.getWriteTime() != -1 && 
      millis() > sysConfig.getWriteTime()){
    sysConfig.write();
  }

  if (state.OTAState == 100)
  {
    if (wifiMgr.isConnected()) {
      wifiMQTT.disconnect();
      ota.downloadOverWiFi();
    }
#ifdef CELLEULAR_ENABLED
      ota.downloadWithModem();
#endif
  }

  communicationsTask(NULL);

  #ifdef BT_ENABLED
  bleTask(NULL);
#endif

#ifdef CAN_ENABLED
  canBus.loop();
#endif

  console.loop();

  if (__nextLoop < millis())
  {
    __nextLoop = millis() + sysConfig.LoopUpdateRateMS;
    hal.loop();
    
    state.loop();
    ledManager.loop();
    probes.loop();
    if(_i2cConfigure)
      adc.loop();
    onOffDetector.loop();
    pulseCounter.loop();
    powerSensor.loop();
    relayManager.loop();
    pir.loop();
  }
}

void mqttSubscribe(String topic)
{
  if (sysConfig.WiFiEnabled)
  {
    wifiMQTT.addSubscriptions(topic);
  }
#ifdef CELLULAR_ENABLED
  else if (sysConfig.CellEnabled)
  {
    cellMQTT.subscribe(topic, QOS0);
  }  
  else if (sysConfig.CellEnabled)
  {
    cellMQTT.subscribe(topic, QOS0);
  }

  wifiMQTT.setMessageReceivedCallback(mqttCallback);
  cellMQTT.setMessageReceivedCallback(mqttCallback);
#endif
}

void mqttPublish(String topic, String value)
{
  if (sysConfig.WiFiEnabled)
  {
    wifiMQTT.publish(topic, value);
  }
#ifdef  CELLULAR_ENABLED 
  else if (sysConfig.CellEnabled)
  {
    cellMQTT.publish(topic, value, QOS0);
  }
#endif
}

void mqttPublish(String topic, byte *buffer, uint16_t size, byte qos)
{
  if (sysConfig.WiFiEnabled)
  {
    wifiMQTT.publish(topic, buffer, size);
  }
#ifdef  CELLULAR_ENABLED
  else if (sysConfig.CellEnabled)
  {
    cellMQTT.publish(topic, buffer, size, qos);
  }
#endif
}

void mqttPublish(String topic)
{
  mqttPublish(topic, "");
}

void setCommandHandler(void (*callback)(String topic, byte *buffer, size_t len)){
  client.setCommandHandlerCallback(callback);
}

void writeConfigPins()
{
  console.println("Has Display :" + configPins.HasDisplay ? "Yes" : "No");
  console.println("Has Leds    :" + configPins.HasLeds ? "Yes" : "No");
  console.println("LED-ONLINE  :" + String(configPins.OnlineLED));
  console.println("LED-ERROR   :" + String(configPins.ErrorLED));
  console.println("INVERT LEDS :" + configPins.InvertLED ? "Yes" : "No");
  console.println("BUZZER      :" + String(configPins.Buzzer));
  console.println("Has Relays  :" + configPins.HasI2C ? "Yes" : "No");
  console.println("I2C-SDA     :" + String(configPins.Sda1));
  console.println("I2C-SCL     :" + String(configPins.Scl1));
  console.println("CONSOLE-RX  :" + String(configPins.ConsoleRx));
  console.println("CONSOLE-TX  :" + String(configPins.ConsoleTx));
  console.println("MODEM-RESET :" + String(configPins.ModemResetPin));
  console.println("SIM-RX      :" + String(configPins.SimRx));
  console.println("SIM-TX      :" + String(configPins.SimTx));
  console.println("ADC1        :" + String(configPins.ADCChannel1));
  console.println("ADC2        :" + String(configPins.ADCChannel2));
  console.println("ADC3        :" + String(configPins.ADCChannel3));
  console.println("ADC4        :" + String(configPins.ADCChannel4));
  console.println("ADC5        :" + String(configPins.ADCChannel5));
  console.println("ADC6        :" + String(configPins.ADCChannel6));
  console.println("ADC7        :" + String(configPins.ADCChannel7));
  console.println("ADC8        :" + String(configPins.ADCChannel8));
  console.println("IO1         :" + String(configPins.Gpio1));
  console.println("IO2         :" + String(configPins.Gpio2));
  console.println("IO3         :" + String(configPins.Gpio3));
  console.println("IO4         :" + String(configPins.Gpio4));
  console.println("IO5         :" + String(configPins.Gpio5));
  console.println("IO6         :" + String(configPins.Gpio6));
  console.println("IO7         :" + String(configPins.Gpio7));
  console.println("IO8         :" + String(configPins.Gpio8));
  console.println("Buzzer      :" + String(configPins.Buzzer));
  console.println("Has Relays  :" + configPins.HasRelays ? "Yes" : "No");
  console.println("K1CTL       :" + String(configPins.K1Ctl));
  console.println("K2CTL       :" + String(configPins.K2Ctl));
  console.println("K3CTL       :" + String(configPins.K3Ctl));
  console.println("K4CTL       :" + String(configPins.K4Ctl));
  console.println("K5CTL       :" + String(configPins.K5Ctl));
}

#endif