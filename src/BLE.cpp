#include "BLE.h"
#include <NuvIotState.h>

// List of BLE Services:
// https://www.bluetooth.com/specifications/specs/
//

#define SVC_UUID_NUVIOT "d804b639-6ce7-4e80-9f8a-ce0f699085eb"

#define CHAR_UUID_STATE "d804b639-6ce7-5e81-9f8a-ce0f699085eb"
/* 
 * State characteristic will encompass 
 * Read/Write and Will Notify
 *
 * xxxx => F/W SKU
 * xxx.xxx.xxx, F/W Version =>
 * xxx.xxx.xxx, H/W Version =>
 
 * (1/0) => Commissioned
 * (1/0) => BT Connectivity
 * (1/0) => WiFi Connectivity
 * (XX) => WiFiRSSI Connectivity
 * (1/0) => Cell Connectivity
 * (1/0) => CellRSSI
 * (1/0) => GPS Connectivity
 * (1/0) => GPS Satelites
 * (1/0) => Server Connectivity
 * xxx => OTA State
 * xxx => OTA Param
 */

#define CHAR_UUID_SYS_CONFIG "d804b639-6ce7-5e82-9f8a-ce0f699085eb"
/* 
  * Sys Config characteristic
  * Read/Write
  * xxxxx, Device Id <= =>
  * xxxxx, B64 Device Key (128 characters) =>
  * (0/1) Cell Enable <= =>
  * (0/1) WiFi Enable <= =>
  * xxxxxx WiFi SSID <= =>
  * xxxxxx WiFi Password =>
  * xxxx Ping Rate (sec)
  * xxxx Send Rate (sec)
  * (0/1) GPS Enable
  * xxxx GPS Rate (sec),
  */

#define CHAR_UUID_IOCONFIG "d804b639-6ce7-5e83-9f8a-ce0f699085eb"
/* IO Config
   * 
   * 8 Slots
   * 3 Params per slot
   * x = Configuration
   * xxx = scale
   * xxx = zero
   *
   */

#define CHAR_UUID_ADC_IOCONFIG "d804b639-6ce7-5e84-9f8a-ce0f699085eb"
/* ADC Config
   * 
   * 8 Slots
   * 3 Params per slot
   * x = Configuration
   * xxx = scale
   * xxx = zero
   *
   */

#define CHAR_UUID_IO_VALUE "d804b639-6ce7-5e85-9f8a-ce0f699085eb"
/* IO Config
   * 
   * 8 Slots
   * 3 Params per slot
   * x = Configuration
   * xxx = scale
   * xxx = zero
   *
   */

#define CHAR_UUID_ADC_VALUE "d804b639-6ce7-5e86-9f8a-ce0f699085eb"
/* ADC Config
   * 
   * 8 Slots
   * 3 Params per slot
   * x = Configuration
   * xxx = scale
   * xxx = zero
   *
   */

#define CHAR_UUID_RELAY "d804b639-6ce7-5e87-9f87-ce0f699085eb"
/* RELAY Config
   * 
   * 16 slots
   * (1,0) <= => Relay State
   *
   */

#define CHAR_UUID_CONSOLE "d804b639-6ce7-5e88-9f88-ce0f699085eb"
/* RELAY Config
   * 
   * 16 slots
   * (1,0) <= => Relay State
   *
   */

#define FULL_PACKET 512
#define CHARPOS_UPDATE_FLAG 5

esp_ota_handle_t otaHandler = 0;

bool updateFlag = false;
bool readyFlag = false;
int bytesReceived = 0;
int timesWritten = 0;

BLE *__BLEInstance;

void bleWriteConsoleValue(String msg)
{
  __BLEInstance->writeConsoleOutput(msg);
}

void otaCallback::onRead(BLECharacteristic *pCharacteristic)
{
  pBle->handleReadCharacteristic(pCharacteristic);
}

void otaCallback::onNotify(BLECharacteristic *pCharacteristic)
{
  pBle->handleNotifyCharacteristic(pCharacteristic);
}

void otaCallback::onWrite(BLECharacteristic *pCharacteristic)
{
  pBle->handleWriteCharacteristic(pCharacteristic);
}

void BLECustomServerCallbacks::onConnect(BLEServer *pServer)
{
  delay(1000);
  pBle->clientConnected(pServer->getConnId());
};

int tempIndex = 0;

void BLE::writeConsoleOutput(String msg)
{
  if (pCharConsole != NULL && m_isConnected)
  {
    const char *str = msg.c_str();    

    for (uint8_t idx = 0; idx < msg.length(); idx++)
    {
      char ch = str[idx];
      if (ch == '\n')
      {
        uint32_t deltaT = millis() - m_lastConsoleNotify;
        if (deltaT < 80)
          delay(80 - deltaT);

        pCharConsole->setValue((uint8_t *)outBuffer.c_str(), outBuffer.length());
        pCharConsole->notify();
        m_lastConsoleNotify = millis();
        outBuffer = "";
      }
      else
      {
        outBuffer += ch;
      }
    }
  }
}

void BLE::refreshCharacteristics()
{
  String state = 
      pState->getFirmwareSKU() + "," +      
      pState->getFirmwareVersion() + "," +
      pState->getHardwareRevision() + "," +
      (pSysConfig->Commissioned ? "1," : "0,") +
      (pState->getIsWiFiConnected() ? "1," : "0,") +
      String(pState->getWiFiRSSI()) + "," +
      (pState->getIsCellConnected() ? "1," : "0,") +
      String(pState->getCellRSSI()) + "," +
      (pState->getIsCloudConnected() ? "1," : "0,") +
      String(pState->OTAState) + "," +
      String(pState->OTAParam);

  pCharState->setValue((uint8_t *)state.c_str(), state.length());

  String config =
      pSysConfig->DeviceId + "," +
      String(_deviceModelId) + "," + 
      pSysConfig->DeviceAccessKey + "," +
      (pSysConfig->CellEnabled ? "1," : "0,") +
      (pSysConfig->WiFiEnabled ? "1," : "0,") +
      pSysConfig->WiFiSSID + "," +
      pSysConfig->WiFiPWD + "," +
      String(pSysConfig->PingRate) + "," +
      String(pSysConfig->SendUpdateRate) + "," +
      (pSysConfig->GPSEnabled ? "1," : "0,")  + 
      String(pSysConfig->GPSUpdateRate);

  pCharConfig->setValue(config.c_str());

  String relay =
      String(pRelayManager->getRelayState(0) ? "1," : "0,") +
      String(pRelayManager->getRelayState(1) ? "1," : "0,") +
      (pRelayManager->getRelayState(2) ? "1," : "0,") +
      (pRelayManager->getRelayState(3) ? "1," : "0,") +
      (pRelayManager->getRelayState(4) ? "1" : "0");

  pCharRelay->setValue(relay.c_str());
}

void BLECustomServerCallbacks::onDisconnect(BLEServer *pServer)
{
  BLEDevice::startAdvertising();
  pConsole->println("ble=disconnected;");
  pBle->clientDisconnected();
}

void BLE::handleReadCharacteristic(BLECharacteristic *characteristic)
{
  refreshCharacteristics();
}

void BLE::handleWriteCharacteristic(BLECharacteristic *characteristic)
{
  m_lastClientActviity = millis();

  String input = String(characteristic->getValue().c_str());
  const char *uuid = characteristic->getUUID().toString().c_str();
  String charId = String(characteristic->getUUID().toString().c_str());

  if (0 == strcmp(uuid, CHAR_UUID_STATE))
  {
    pConsole->println("ble=read; // char=sysstate; value=" + input);
    // pSysConfig->Commissioned = characteristic->getData()[0] == '1';

    refreshCharacteristics();
  }

  if (0 == strcmp(uuid, CHAR_UUID_IOCONFIG))
  {
    int equalDelimiter = input.indexOf("=");
    int valueEnd = input.indexOf(";", equalDelimiter);
    String key = input.substring(0, equalDelimiter);
    String value = input.substring(equalDelimiter + 1, valueEnd);

    pConsole->println(key + " - " + value);

    char tmp[120];

    if(key == "readadc") {
      pIOConfig->getADC(tmp, 1);
      pConsole->println("BACK");
      delay(15);
      pConsole->println(String(tmp));
      delay(15);
      pCharIOConfig->setValue((uint8_t*)tmp,strlen(tmp));
    }
    else if(key == "readgpio") {
      
      pIOConfig->getADC(tmp, 1);
      pCharIOConfig->setValue((uint8_t*)tmp,strlen(tmp));
    }
    else if(key == "writeadc") {
      strcpy(tmp, value.c_str());
      pIOConfig->setADC(tmp);
      pIOConfig->write();
    }
    else if(key == "writegpio") {
      strcpy(tmp, value.c_str());
      pIOConfig->setGPIO(tmp);
      pIOConfig->write();
    }
  }

  if (0 == strcmp(uuid, CHAR_UUID_SYS_CONFIG))
  {
    pConsole->println("ble=read; // char=sysconfig; value=" + input);
    boolean done = false;
    int lastEnd = 0;
    while (!done)
    {
      int equalDelimiter = input.indexOf("=", lastEnd);
      int valueEnd = input.indexOf(",", equalDelimiter);
      int final = input.indexOf(";", equalDelimiter);

      String key = input.substring(lastEnd, equalDelimiter);
      String value = input.substring(equalDelimiter + 1, valueEnd == -1 ? final : valueEnd);

      pConsole->println(key + "=" + value + " => " + String(equalDelimiter) + " " + String(valueEnd) + " " + String(final));

      // if we don't have a , that means we are past the final item
      done = valueEnd == -1;
      lastEnd = valueEnd + 1;

      if (key == "host")
        pSysConfig->SrvrHostName = value;
      else if (key == "anonymous")
        pSysConfig->Anonymous = value != "0";
      else if (key == "uid")
        pSysConfig->SrvrUID = value;
      else if (key == "pwd")
        pSysConfig->SrvrPWD = value;
      else if (key == "wifissid")
        pSysConfig->WiFiSSID = value;
      else if (key == "wifipwd")
        pSysConfig->WiFiPWD = value;
      else if (key == "verboselog")
        pSysConfig->VerboseLogging = value != "0";
      else if (key == "gps")
        pSysConfig->GPSEnabled = value != "0";
      else if (key == "cell")
        pSysConfig->CellEnabled = value != "0";
      else if (key == "wifi")
        pSysConfig->WiFiEnabled = value != "0";
      else if (key == "deviceid")
        pSysConfig->DeviceId = atoi(value.c_str());
      else if (key == "key")
        pSysConfig->DeviceAccessKey = value;
      else if (key == "gpsrate")
        pSysConfig->GPSUpdateRate = atoi(value.c_str());
      else if (key == "pingrate")
        pSysConfig->PingRate = atoi(value.c_str());
      else if (key == "sendrate")
        pSysConfig->SendUpdateRate = atoi(value.c_str());
      else if (key == "commissioned")
        pSysConfig->Commissioned = value != "0";
    }    

    pSysConfig->write();
    pState->loop();    
  }
}

void BLE::handleNotifyCharacteristic(BLECharacteristic *characteristic)
{
  /*if (characteristic->getUUID().toString() == CHAR_UUID_PROVISION_DEVICE_ID)
  {
    pConsole->println("handle notify device id");
  }
  else
  {
    pConsole->println("handle notify OTHER");
  }*/
}

//
// begin
bool BLE::begin(const char *localName, const char *deviceModelId)
{
  _deviceModelId = deviceModelId;  

  uint32_t freeHeep = ESP.getFreeHeap();

  // Create the BLE Device
  BLEDevice::init(localName);

  // Create the BLE Server
  pServer = BLEDevice::createServer();
  pServer->setCallbacks(new BLECustomServerCallbacks(this, pConsole));

  pService = pServer->createService(BLEUUID(SVC_UUID_NUVIOT));

  pCharState = pService->createCharacteristic(CHAR_UUID_STATE, BLECharacteristic::PROPERTY_READ | BLECharacteristic::PROPERTY_WRITE | BLECharacteristic::PROPERTY_NOTIFY);
  pCharState->setCallbacks(_characteristicCallback);
  pCharState->addDescriptor(new BLE2902());

  pCharConfig = pService->createCharacteristic(CHAR_UUID_SYS_CONFIG, BLECharacteristic::PROPERTY_READ | BLECharacteristic::PROPERTY_WRITE);
  pCharConfig->setCallbacks(_characteristicCallback);

  pCharIOValue = pService->createCharacteristic(CHAR_UUID_IO_VALUE, BLECharacteristic::PROPERTY_READ | BLECharacteristic::PROPERTY_WRITE | BLECharacteristic::PROPERTY_NOTIFY);
  pCharIOValue->setCallbacks(_characteristicCallback);
  pCharIOValue->addDescriptor(new BLE2902());

  pCharIOConfig = pService->createCharacteristic(CHAR_UUID_IOCONFIG, BLECharacteristic::PROPERTY_READ | BLECharacteristic::PROPERTY_WRITE);
  pCharIOConfig->setCallbacks(_characteristicCallback);

  pCharRelay = pService->createCharacteristic(CHAR_UUID_RELAY, BLECharacteristic::PROPERTY_READ | BLECharacteristic::PROPERTY_WRITE | BLECharacteristic::PROPERTY_NOTIFY);
  pCharRelay->setCallbacks(_characteristicCallback);
  pCharRelay->addDescriptor(new BLE2902());

  pCharConsole = pService->createCharacteristic(CHAR_UUID_CONSOLE, BLECharacteristic::PROPERTY_READ | BLECharacteristic::PROPERTY_WRITE | BLECharacteristic::PROPERTY_NOTIFY | BLECharacteristic::PROPERTY_INDICATE);
  pCharConsole->setCallbacks(_characteristicCallback);
  pCharConsole->addDescriptor(new BLE2902());

  BLEAdvertising *pAdvertising = pServer->getAdvertising();
  pAdvertising->addServiceUUID(SVC_UUID_NUVIOT);
  pAdvertising->setScanResponse(true);
  pAdvertising->setMinPreferred(0x06);
  pAdvertising->setMinPreferred(0x12);

  pService->start();
  pAdvertising->start();

  pServer->getConnectedCount();
  

  pConsole->println("ALLOCATED FOR BLE " + String(freeHeep - ESP.getFreeHeap()));

  refreshCharacteristics();

  return true;
}

void BLE::update()
{
  idx++;

  if (m_isConnected)
  {
    pCharState->notify(true);
    pCharRelay->notify(true);
    pCharIOValue->notify(true);

    if(millis() - m_lastClientActviity > 3000) {
      pServer->disconnect(m_connectionId);
      pConsole->println("ble=activitytimeout;");
    }
    else
    {
      pConsole->println(F("ble=connected;"));
    }
  }
  else
  {
//    pConsole->println(F("ble=disconnected;"));
  }
}

void BLE::stop()
{
  if (pService != NULL)
  {
    pService->stop();
    pService = NULL;
  }

  if (m_pAdvertising != NULL)
  {
    m_pAdvertising->stop();
    m_pAdvertising = NULL;
  }
}

//
// Destructor
BLE::~BLE(void)
{
}
