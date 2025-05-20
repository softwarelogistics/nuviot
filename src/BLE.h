#ifndef _BLE_H_
#define _BLE_H_

#include "Arduino.h"

#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>

#include "esp_ota_ops.h"
#include <RelayManager.h>
#include <Console.h>
#include <IOConfig.h>
#include <SysConfig.h>
#include <Hal.h>
#include <NuvIoTState.h>
#include <MessagePayload.h>
#include <OtaServices.h>
#include "WiFiConnectionHelper.h"

#define SOFTWARE_VERSION_MAJOR 0
#define SOFTWARE_VERSION_MINOR 1
#define SOFTWARE_VERSION_PATCH 0
#define HARDWARE_VERSION_MAJOR 1
#define HARDWARE_VERSION_MINOR 2

class BLE;

extern void bleWriteConsoleValue(String msg);
extern BLE *__BLEInstance;

class BLECustomServerCallbacks : public BLEServerCallbacks
{
private:
  Console *pConsole;
  BLE *pBle;

public:
  BLECustomServerCallbacks(BLE *ble, Console *console)
  {
    pConsole = console;
    pBle = ble;
  }

  void onConnect(BLEServer *pServer);
  void onDisconnect(BLEServer *pServer);
};

class otaCallback : public BLECharacteristicCallbacks
{
private:
  BLE *pBle;
  Console *pConsole;    

public:
  otaCallback(BLE *ble, Console *console)
  {
    pBle = ble;
    pConsole = console;
  }

  void onWrite(BLECharacteristic *pCharacteristic);
  void onRead(BLECharacteristic *pCharacteristic);
  void onNotify(BLECharacteristic *pCharacteristic);
};

#define NUVIOT_BLE_FLAGS_SITE_SCAN 0x01
#define NUVIOT_BLE_FLAGS_WIFI_RECONNECT 0x02

class BLE
{
public:
  BLE(Console *console, Hal *hal, NuvIoTState *state, IOConfig *ioConfig, WiFiConnectionHelper *wifi, SysConfig *sysConfig, RelayManager *relayManager, OtaServices *ota, MessagePayload *payload)
  {
    pWifi = wifi;
    pConsole = console;
    pHal = hal;
    pSysConfig = sysConfig;
    pIOConfig = ioConfig;
    pState = state;
    pOta = ota;
    pRelayManager = relayManager;
    pPayload = payload;
    _characteristicCallback = new otaCallback(this, console);
    pConsole->registerBLEWriter(bleWriteConsoleValue);
    __BLEInstance = this;
  }
  ~BLE(void);

 
  bool begin(const char *localName, const char *deviceModelId);

  void refreshCharacteristics(bool notifyOnly);

  void writeConsoleOutput(String msg);
  void writeWiFiMessage(String msg);
  void writeCANMessage(uint32_t msgId, uint8_t msg[], uint8_t len);

  void wifiDisconnect();

  void handleReadCharacteristic(BLECharacteristic *characteristic);
  void handleWriteCharacteristic(BLECharacteristic *characteristic, String value);
  void handleNotifyCharacteristic(BLECharacteristic *characteristic);

  void clientConnected(int connectionId)
  {
    m_connectionId = connectionId;
    pConsole->println("ble=connected; // Connection Id: " + String(connectionId));
    refreshCharacteristics(false);
    m_lastClientActivity = millis();
    m_isConnected = true;    
    pState->setIsBleConnected(true);
  }

  void clientDisconnected(int connectionId)
  {
    m_isConnected = false;
    pState->setIsBleConnected(false);
    pConsole->println("ble=disconnected; // Connection Id: " + String(connectionId));    
  }

  bool getIsConnected(){
    return m_isConnected;
  }

  void update();
  void stop();

private:
  uint8_t m_flags = 0;

   BLEAdvertising *m_pAdvertising;
  otaCallback *_characteristicCallback;
  String outBuffer;
  MessagePayload *pPayload;
  OtaServices *pOta;

  WiFiConnectionHelper *pWifi;
  Console *pConsole;
  SysConfig *pSysConfig;
  IOConfig *pIOConfig;
  Hal *pHal;
  NuvIoTState *pState;
  RelayManager *pRelayManager;

  long m_lastConsoleNotify = 0;
  long m_lastClientActivity = 0;
  long m_nextNotify = 0;
  int m_connectionId;
  const char *_deviceModelId;

  char gpioConfigOut[1];
  char gpioLabelsOut[1];
  char gpioNamesOut[1];
  char gpioScalerOut[1];
  char gpioZeroOut[1];

  char adcConfigOut[1];
  char adcLabelsOut[1];
  char adcNamesOut[1];
  char adcScalerOut[1];
  char adcZeroOut[1];

  void scanWiFiNetworks();

  bool m_isConnected = false;

  String m_currentConfigPort = "adc1";
  String local_name;
  String siteSurvey = "";

  BLEServer *pServer = NULL;

  BLEService *pService = NULL;

  BLECharacteristic *pCharState = NULL;
  
  BLECharacteristic *pCharConfig = NULL;

  BLECharacteristic *pCharIOConfig = NULL;
  BLECharacteristic *pCharIOValue = NULL;

  BLECharacteristic *pCharConsole = NULL;
  BLECharacteristic *pCharRelay = NULL;  

  BLECharacteristic *pCharCAN = NULL;  

  BLECharacteristic *pCharWiFi = NULL;
};

#endif
