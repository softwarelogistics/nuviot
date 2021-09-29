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

class BLE
{
public:
  BLE(Console *console, Hal *hal, NuvIoTState *state, IOConfig *ioConfig, SysConfig *sysConfig, RelayManager *relayManager, MessagePayload *payload)
  {
    pConsole = console;
    pHal = hal;
    pSysConfig = sysConfig;
    pIOConfig = ioConfig;
    pState = state;
    pRelayManager = relayManager;
    pPayload = payload;
    _characteristicCallback = new otaCallback(this, console);
    pConsole->registerBLEWriter(bleWriteConsoleValue);
    __BLEInstance = this;
  }
  ~BLE(void);

  bool begin(const char *localName, const char *deviceModelId);

  void refreshCharacteristics();

  void writeConsoleOutput(String msg);

  void handleReadCharacteristic(BLECharacteristic *characteristic);
  void handleWriteCharacteristic(BLECharacteristic *characteristic);
  void handleNotifyCharacteristic(BLECharacteristic *characteristic);

  void clientConnected(int connectionId)
  {
    m_connectionId = connectionId;
    pConsole->println("ble=connected; // Conenction Id: " + String(connectionId));
    refreshCharacteristics();
    m_lastClientActivity = millis();
    m_isConnected = true;    
  }

  void clientDisconnected(int connectionId)
  {
    m_isConnected = false;
    pConsole->println("ble=disconnected; // Conenction Id: " + String(connectionId));    
  }

  bool getIsConnected(){
    return m_isConnected;
  }

  void update();
  void stop();

private:
  BLEAdvertising *m_pAdvertising;
  otaCallback *_characteristicCallback;
  String outBuffer;
  MessagePayload *pPayload;

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

  bool m_isConnected = false;

  String local_name;

  BLEServer *pServer = NULL;

  BLEService *pService = NULL;
  BLECharacteristic *pCharState = NULL;
  BLECharacteristic *pCharConfig = NULL;
  BLECharacteristic *pCharIOConfig = NULL;
  BLECharacteristic *pCharIOValue = NULL;
  BLECharacteristic *pCharConsole = NULL;
};

#endif
