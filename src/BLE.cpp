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
  delay(250);
  pBle->clientConnected(pServer->getConnId());
};

void BLECustomServerCallbacks::onDisconnect(BLEServer *pServer)
{
  BLEDevice::startAdvertising();
  pBle->clientDisconnected(pServer->getConnId());
}

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

        uint16_t len = outBuffer.length() > 600 ? 600 : outBuffer.length();

        pCharConsole->setValue((uint8_t *)outBuffer.c_str(), len);
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
      pState->getFirmwareSKU() + "," +                   // 0
      pState->getFirmwareVersion() + "," +               // 1
      pState->getHardwareRevision() + "," +              // 2
      (pSysConfig->Commissioned ? "1," : "0,") +         // 3
      String(pState->getWiFiState()) + "," +             // 4
      String(pState->getWiFiRSSI()) + "," +              // 5
      pState->getWiFiIPAddress()+  "," +                 // 6
      (pState->getIsCellConnected() ? "1," : "0,") +     // 7
      String(pState->getCellRSSI()) + "," +              // 8
      pState->getCellIPAddress()+  "," +                 // 9
      (pState->getIsCloudConnected() ? "1," : "0,") +    // 10
      String(pState->getInputVoltage()) + "," +          // 11
      (pState->getExternalPower() ? "1," : "0,") +       // 12
      String(pState->OTAState) + "," +                   // 13
      String(pState->OTAParam);                          // 14

  pCharState->setValue((uint8_t *)state.c_str(), state.length());

  String config =
      pSysConfig->DeviceId + "," +
      pSysConfig->OrgId + "," +
      pSysConfig->RepoId + "," +
      pSysConfig->Id + "," +
      String(_deviceModelId) + "," +
      pSysConfig->SrvrHostName + "," +      
      pSysConfig->SrvrUID + "," +      
      pSysConfig->SrvrPWD + "," +      
      String(pSysConfig->Port) + "," +
      pSysConfig->SrvrType + "," +
      pSysConfig->DeviceAccessKey + "," +
      (pSysConfig->Commissioned ? "1," : "0,") +
      (pSysConfig->CellEnabled ? "1," : "0,") +
      (pSysConfig->WiFiEnabled ? "1," : "0,") +
      pSysConfig->WiFiSSID + "," +
      pSysConfig->WiFiPWD + "," +
      String(pSysConfig->PingRate) + "," +
      String(pSysConfig->SendUpdateRate) + "," +
      (pSysConfig->GPSEnabled ? "1," : "0,") +
      String(pSysConfig->GPSUpdateRate);

  pCharConfig->setValue(config.c_str());

  String ioConfig = m_currentConfigPort + ',';

  if (m_currentConfigPort == "adcall")
    ioConfig += String(pIOConfig->ADC1Config) + "," + String(pIOConfig->ADC1Name) + "," + String(pIOConfig->ADC2Config) + "," + String(pIOConfig->ADC2Name) + "," +
                String(pIOConfig->ADC3Config) + "," + String(pIOConfig->ADC3Name) + "," + String(pIOConfig->ADC4Config) + "," + String(pIOConfig->ADC4Name) + "," +
                String(pIOConfig->ADC5Config) + "," + String(pIOConfig->ADC5Name) + "," + String(pIOConfig->ADC6Config) + "," + String(pIOConfig->ADC6Name) + "," +
                String(pIOConfig->ADC7Config) + "," + String(pIOConfig->ADC7Name) + "," + String(pIOConfig->ADC8Config) + "," + String(pIOConfig->ADC8Name) + ",";

  else if (m_currentConfigPort == "ioall")
    ioConfig += String(pIOConfig->GPIO1Config) + "," + String(pIOConfig->GPIO1Name) + "," + String(pIOConfig->GPIO2Config) + "," + String(pIOConfig->GPIO2Name) + "," +
                String(pIOConfig->GPIO3Config) + "," + String(pIOConfig->GPIO3Name) + "," + String(pIOConfig->GPIO4Config) + "," + String(pIOConfig->GPIO4Name) + "," +
                String(pIOConfig->GPIO5Config) + "," + String(pIOConfig->GPIO5Name) + "," + String(pIOConfig->GPIO6Config) + "," + String(pIOConfig->GPIO6Name) + "," +
                String(pIOConfig->GPIO7Config) + "," + String(pIOConfig->GPIO7Name) + "," + String(pIOConfig->GPIO8Config) + "," + String(pIOConfig->GPIO8Name) + ",";

  else if (m_currentConfigPort == "relayall")
    ioConfig += pIOConfig->Relay1Name + "," + pIOConfig->Relay2Name + "," + pIOConfig->Relay3Name + "," + pIOConfig->Relay4Name + "," +
                pIOConfig->Relay5Name + "," + pIOConfig->Relay6Name + "," + pIOConfig->Relay7Name + "," + pIOConfig->Relay8Name;

  else if (m_currentConfigPort == "adc1")
    ioConfig += String(pIOConfig->ADC1Name) + "," + String(pIOConfig->ADC1Config) + "," + String(pIOConfig->ADC1Scaler) + "," + String(pIOConfig->ADC1Calibration) + "," + String(pIOConfig->ADC1Zero) + ",";

  else if (m_currentConfigPort == "adc2")
    ioConfig += String(pIOConfig->ADC2Name) + "," + String(pIOConfig->ADC2Config) + "," + String(pIOConfig->ADC2Scaler) + "," + String(pIOConfig->ADC2Calibration) + "," + String(pIOConfig->ADC2Zero) + ",";

  else if (m_currentConfigPort == "adc3")
    ioConfig += String(pIOConfig->ADC3Name) + "," + String(pIOConfig->ADC3Config) + "," + String(pIOConfig->ADC3Scaler) + "," + String(pIOConfig->ADC3Calibration) + "," + String(pIOConfig->ADC3Zero) + ",";

  else if (m_currentConfigPort == "adc4")
    ioConfig += String(pIOConfig->ADC4Name) + "," + String(pIOConfig->ADC4Config) + "," + String(pIOConfig->ADC4Scaler) + "," + String(pIOConfig->ADC4Calibration) + "," + String(pIOConfig->ADC4Zero) + ",";

  else if (m_currentConfigPort == "adc5")
    ioConfig += String(pIOConfig->ADC5Name) + "," + String(pIOConfig->ADC5Config) + "," + String(pIOConfig->ADC5Scaler) + "," + String(pIOConfig->ADC5Calibration) + "," + String(pIOConfig->ADC5Zero) + ",";

  else if (m_currentConfigPort == "adc6")
    ioConfig += String(pIOConfig->ADC6Name) + "," + String(pIOConfig->ADC6Config) + "," + String(pIOConfig->ADC6Scaler) + "," + String(pIOConfig->ADC6Calibration) + "," + String(pIOConfig->ADC6Zero) + ",";

  else if (m_currentConfigPort == "adc7")
    ioConfig += String(pIOConfig->ADC7Name) + "," + String(pIOConfig->ADC7Config) + "," + String(pIOConfig->ADC7Scaler) + "," + String(pIOConfig->ADC7Calibration) + "," + String(pIOConfig->ADC7Zero) + ",";

  else if (m_currentConfigPort == "adc8")
    ioConfig += String(pIOConfig->ADC8Name) + "," + String(pIOConfig->ADC8Config) + "," + String(pIOConfig->ADC8Scaler) + "," + String(pIOConfig->ADC8Calibration) + "," + String(pIOConfig->ADC8Zero) + ",";

  else if (m_currentConfigPort == "io1")
    ioConfig += String(pIOConfig->GPIO1Name) + "," + String(pIOConfig->GPIO1Config) + "," + String(pIOConfig->GPIO1Calibration) + "," + String(pIOConfig->GPIO1Scaler) + "," + String(pIOConfig->GPIO1Zero) + ",";

  else if (m_currentConfigPort == "io2")
    ioConfig += String(pIOConfig->GPIO2Name) + "," + String(pIOConfig->GPIO2Config) + "," + String(pIOConfig->GPIO2Calibration) + "," + String(pIOConfig->GPIO2Scaler) + "," + String(pIOConfig->GPIO2Zero) + ",";

  else if (m_currentConfigPort == "io3")
    ioConfig += String(pIOConfig->GPIO3Name) + "," + String(pIOConfig->GPIO3Config) + "," + String(pIOConfig->GPIO3Calibration) + "," + String(pIOConfig->GPIO3Scaler) + "," + String(pIOConfig->GPIO3Zero) + ",";

  else if (m_currentConfigPort == "io4")
    ioConfig += String(pIOConfig->GPIO4Name) + "," + String(pIOConfig->GPIO4Config) + "," + String(pIOConfig->GPIO4Calibration) + "," + String(pIOConfig->GPIO4Scaler) + "," + String(pIOConfig->GPIO4Zero) + ",";

  else if (m_currentConfigPort == "io5")
    ioConfig += String(pIOConfig->GPIO5Name) + "," + String(pIOConfig->GPIO5Config) + "," + String(pIOConfig->GPIO5Calibration) + "," + String(pIOConfig->GPIO5Scaler) + "," + String(pIOConfig->GPIO5Zero) + ",";

  else if (m_currentConfigPort == "io6")
    ioConfig += String(pIOConfig->GPIO6Name) + "," + String(pIOConfig->GPIO6Config) + "," + String(pIOConfig->GPIO6Calibration) + "," + String(pIOConfig->GPIO6Scaler) + "," + String(pIOConfig->GPIO6Zero) + ",";

  else if (m_currentConfigPort == "io7")
    ioConfig += String(pIOConfig->GPIO7Name) + "," + String(pIOConfig->GPIO7Config) + "," + String(pIOConfig->GPIO7Calibration) + "," + String(pIOConfig->GPIO7Scaler) + "," + String(pIOConfig->GPIO7Zero) + ",";

  else if (m_currentConfigPort == "io8")
    ioConfig += String(pIOConfig->GPIO8Name) + "," + String(pIOConfig->GPIO8Config) + "," + String(pIOConfig->GPIO8Calibration) + "," + String(pIOConfig->GPIO8Scaler) + "," + String(pIOConfig->GPIO8Zero) + ",";

  pCharIOConfig->setValue(ioConfig.c_str());

  String relay =
      String(pRelayManager->getRelayState(0) ? "1," : "0,") +
      String(pRelayManager->getRelayState(1) ? "1," : "0,") +
      String(pRelayManager->getRelayState(2) ? "1," : "0,") +
      String(pRelayManager->getRelayState(3) ? "1," : "0,") +
      String(pRelayManager->getRelayState(4) ? "1" : "0");

  pCharRelay->setValue(relay.c_str());

  pCharIOValue->setValue(pPayload->ioValues->toString().c_str());
}

void BLE::handleReadCharacteristic(BLECharacteristic *characteristic)
{
  refreshCharacteristics();
}

void BLE::handleWriteCharacteristic(BLECharacteristic *characteristic)
{
  m_lastClientActivity = millis();

  String input = String(characteristic->getValue().c_str());
  const char *uuid = characteristic->getUUID().toString().c_str();
  String charId = String(characteristic->getUUID().toString().c_str());

  if (0 == strcmp(uuid, CHAR_UUID_STATE))
  {
    pConsole->println("ble=read; // char=sysstate; value=" + input + ";");
    // pSysConfig->Commissioned = characteristic->getData()[0] == '1';

    refreshCharacteristics();
  }
  else if (0 == strcmp(uuid, CHAR_UUID_SYS_CONFIG)) {
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

      pConsole->println("blewrite=write; // char=sysconfig; Set: " + key + "=" + value + " => " + String(equalDelimiter) + " " + String(valueEnd) + " " + String(final) + ";");

      // if we don't have a , that means we are past the final item
      done = valueEnd == -1;
      lastEnd = valueEnd + 1;

      if (key == "host")
        pSysConfig->SrvrHostName = value;
      else if (key == "port")
        pSysConfig->Port = atoi(value.c_str());
      else if (key == "srvrtype")
        pSysConfig->SrvrType = value;
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
        pSysConfig->DeviceId = value.c_str();
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
      else if (key == "orgid")
        pSysConfig->OrgId = value;
      else if (key == "id")
        pSysConfig->Id = value;
      else if (key == "repoid")
        pSysConfig->RepoId = value;
      else if (key == "dfu") 
        pOta->downloadOverWiFi("http://firmware.nuviot.com:14236/api/firmware/download/" + value);
      else if(key == "reboot" && value == "1")
        pHal->restart();      
      else if(key == "factoryreset" && value == "1") {
        pSysConfig->setDefaults();
        pSysConfig->write();
        pIOConfig->setDefaults();
        pIOConfig->write();
        pHal->queueRestart(1000);
      }
      else
        pConsole->printError("UKNOWN KEY TYPE: " + key);
    }

    pSysConfig->write();
    
  }
  else if (0 == strcmp(uuid, CHAR_UUID_IOCONFIG)) {
      int equalDelimiter = input.indexOf("=", 0);
      int valueEnd = input.indexOf(",", equalDelimiter);
      int final = input.indexOf(";", equalDelimiter);

      String cmd = input.substring(0, equalDelimiter);
      
      if(cmd == "setioview") {
        m_currentConfigPort = input.substring(equalDelimiter + 1);
        pConsole->println("blewrite=setioview; // set io port: " + m_currentConfigPort);
        refreshCharacteristics();
      }
      else if(cmd == "setioconfig") {
        int keyEnd = input.indexOf(",", equalDelimiter);
        String channel = input.substring(equalDelimiter + 1, keyEnd);        
        pConsole->println("blewrite=setioconfig; // channel=" + channel);
        int valueEnd = input.indexOf(",", keyEnd + 1);
        String name = input.substring(keyEnd + 1, valueEnd);
        int valueStart = valueEnd + 1;
        valueEnd = input.indexOf(",", valueStart);
        int config = atoi(input.substring(valueStart, valueEnd).c_str());
        valueStart = valueEnd + 1;
        valueEnd = input.indexOf(",", valueStart);
        float scaler = atof(input.substring(valueStart, valueEnd).c_str());
        valueStart = valueEnd + 1;
        valueEnd = input.indexOf(",", valueStart);
        float calibration = atof(input.substring(valueStart, valueEnd).c_str());
        valueStart = valueEnd + 1;
        valueEnd = input.indexOf(",", valueStart);
        float zero = atof(input.substring(valueStart, valueEnd).c_str());


        if(channel == "adc1"){  pIOConfig->ADC1Name = name; pIOConfig->ADC1Config = config; pIOConfig->ADC1Scaler = scaler; pIOConfig->ADC1Calibration = calibration;  pIOConfig->ADC1Zero = zero;}
        if(channel == "adc2"){  pIOConfig->ADC2Name = name; pIOConfig->ADC2Config = config; pIOConfig->ADC2Scaler = scaler; pIOConfig->ADC2Calibration = calibration;  pIOConfig->ADC2Zero = zero;}
        if(channel == "adc3"){  pIOConfig->ADC3Name = name; pIOConfig->ADC3Config = config; pIOConfig->ADC3Scaler = scaler; pIOConfig->ADC3Calibration = calibration;  pIOConfig->ADC3Zero = zero;}
        if(channel == "adc4"){  pIOConfig->ADC4Name = name; pIOConfig->ADC4Config = config; pIOConfig->ADC4Scaler = scaler; pIOConfig->ADC4Calibration = calibration;  pIOConfig->ADC4Zero = zero;}
        if(channel == "adc5"){  pIOConfig->ADC5Name = name; pIOConfig->ADC5Config = config; pIOConfig->ADC5Scaler = scaler; pIOConfig->ADC5Calibration = calibration;  pIOConfig->ADC5Zero = zero;}
        if(channel == "adc6"){  pIOConfig->ADC6Name = name; pIOConfig->ADC6Config = config; pIOConfig->ADC6Scaler = scaler; pIOConfig->ADC6Calibration = calibration;  pIOConfig->ADC6Zero = zero;}
        if(channel == "adc7"){  pIOConfig->ADC7Name = name; pIOConfig->ADC7Config = config; pIOConfig->ADC7Scaler = scaler; pIOConfig->ADC7Calibration = calibration;  pIOConfig->ADC7Zero = zero;}
        if(channel == "adc8"){  pIOConfig->ADC8Name = name; pIOConfig->ADC8Config = config; pIOConfig->ADC8Scaler = scaler; pIOConfig->ADC8Calibration = calibration;  pIOConfig->ADC8Zero = zero;}
        

        if(channel == "io1"){  pIOConfig->GPIO1Name = name; pIOConfig->GPIO1Config = config; pIOConfig->GPIO1Scaler = scaler; pIOConfig->GPIO1Calibration = calibration;  pIOConfig->GPIO1Zero = zero;}
        if(channel == "io2"){  pIOConfig->GPIO2Name = name; pIOConfig->GPIO2Config = config; pIOConfig->GPIO2Scaler = scaler; pIOConfig->GPIO2Calibration = calibration;  pIOConfig->GPIO2Zero = zero;}
        if(channel == "io3"){  pIOConfig->GPIO3Name = name; pIOConfig->GPIO3Config = config; pIOConfig->GPIO3Scaler = scaler; pIOConfig->GPIO3Calibration = calibration;  pIOConfig->GPIO3Zero = zero;}
        if(channel == "io4"){  pIOConfig->GPIO4Name = name; pIOConfig->GPIO4Config = config; pIOConfig->GPIO4Scaler = scaler; pIOConfig->GPIO4Calibration = calibration;  pIOConfig->GPIO4Zero = zero;}
        if(channel == "io5"){  pIOConfig->GPIO5Name = name; pIOConfig->GPIO5Config = config; pIOConfig->GPIO5Scaler = scaler; pIOConfig->GPIO5Calibration = calibration;  pIOConfig->GPIO5Zero = zero;}
        if(channel == "io6"){  pIOConfig->GPIO6Name = name; pIOConfig->GPIO6Config = config; pIOConfig->GPIO6Scaler = scaler; pIOConfig->GPIO6Calibration = calibration;  pIOConfig->GPIO6Zero = zero;}
        if(channel == "io7"){  pIOConfig->GPIO7Name = name; pIOConfig->GPIO7Config = config; pIOConfig->GPIO7Scaler = scaler; pIOConfig->GPIO7Calibration = calibration;  pIOConfig->GPIO7Zero = zero;}
        if(channel == "io8"){  pIOConfig->GPIO8Name = name; pIOConfig->GPIO8Config = config; pIOConfig->GPIO8Scaler = scaler; pIOConfig->GPIO8Calibration = calibration;  pIOConfig->GPIO8Zero = zero;}
        
        pConsole->println("blewrite=setioconfig; // value=" + name + "," + String(config) + "," + String(scaler) + "," + String(calibration) + "," + String(zero));
        pIOConfig->write();
      }
  }

  pState->loop();
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
  pCharConfig->addDescriptor(new BLE2902());

  pCharIOConfig = pService->createCharacteristic(CHAR_UUID_IOCONFIG, BLECharacteristic::PROPERTY_READ | BLECharacteristic::PROPERTY_WRITE);
  pCharIOConfig->setCallbacks(_characteristicCallback);
  pCharIOConfig->addDescriptor(new BLE2902());

  pCharIOValue = pService->createCharacteristic(CHAR_UUID_IO_VALUE, BLECharacteristic::PROPERTY_READ | BLECharacteristic::PROPERTY_WRITE | BLECharacteristic::PROPERTY_NOTIFY);
  pCharIOValue->setCallbacks(_characteristicCallback);
  pCharIOValue->addDescriptor(new BLE2902());

  pCharConsole = pService->createCharacteristic(CHAR_UUID_CONSOLE, BLECharacteristic::PROPERTY_READ | BLECharacteristic::PROPERTY_WRITE | BLECharacteristic::PROPERTY_NOTIFY);
  pCharConsole->setCallbacks(_characteristicCallback);
  pCharConsole->addDescriptor(new BLE2902());

  pCharRelay = pService->createCharacteristic(CHAR_UUID_RELAY, BLECharacteristic::PROPERTY_READ | BLECharacteristic::PROPERTY_WRITE | BLECharacteristic::PROPERTY_NOTIFY);
  pCharRelay->setCallbacks(_characteristicCallback);
  pCharRelay->addDescriptor(new BLE2902());

  BLEAdvertising *pAdvertising = pServer->getAdvertising();

  pAdvertising->addServiceUUID(SVC_UUID_NUVIOT);
  pAdvertising->setScanResponse(true);
  pAdvertising->setMinPreferred(0x06);
  pAdvertising->setMinPreferred(0x12);

  pService->start();
  pAdvertising->start();

  pServer->getConnectedCount();

  double bleBytes = (freeHeep - ESP.getFreeHeap()) / 1024.0;
  double freeBytes = ESP.getFreeHeap() / 1024.0;

  pConsole->println("ble=allocated; // allocated for BLE: " + String(bleBytes) + "KB; free: " + String(freeBytes) + "KB");

  refreshCharacteristics();

  return true;
}

void BLE::update()
{
  this->refreshCharacteristics();

  if (m_isConnected)
  {
    if (m_nextNotify < millis())
    {
      m_nextNotify = pSysConfig->SendUpdateRate + millis();
      pCharState->notify(true);
      pCharIOValue->notify(true);
      pConsole->println("send");
    }

    /*if (millis() - m_lastClientActivity > 5000)
    {
      pServer->disconnect(m_connectionId);
      pConsole->printWarning(F("ble=activitytimeout;"));
    }*/
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
