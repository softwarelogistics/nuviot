#include "NuvIoTState.h"
#include "esp_bt_device.h"
#include "math.h"
#include <Update.h>

#define IS_CONFIG_LOCATION 0

#define INITIALIZED_ID 100
#define COMMISSIONED_ID 101

#define STRUCTURE_VERSION_START 2
#define DATA_VERSION_START 4

#define ANONYOUS_SERVER_CONN_START 6
#define SECURE_TRANSPORT_START 7

#define INT_SET_MASK_START 10
#define FLT_SET_MASK_START INT_SET_MASK_START + sizeof(long)
#define BOOL_SET_MASK_START FLT_SET_MASK_START + sizeof(long)

#define DEVICEID_START 50
#define DEVICEID_LEN 50 //100

#define DEVICE_ACCESS_CODE_START DEVICEID_START + DEVICEID_LEN
#define DEVICE_ACCESS_CODE_LEN 256 // 356

#define SERVER_HOST DEVICE_ACCESS_CODE_START + DEVICE_ACCESS_CODE_LEN
#define SERVER_HOST_LEN 100 // 238

#define SERVER_USER_NAME_START SERVER_HOST + SERVER_HOST_LEN
#define SERVER_USER_NAME_LEN 100 // 270

#define SERVER_PASSWORD SERVER_USER_NAME_START + SERVER_USER_NAME_LEN
#define SERVER_PASSWORD_LEN 100 // 302

#define WIFI_SSID_START SERVER_PASSWORD + SERVER_PASSWORD_LEN
#define WIFI_SSID_LEN 100 // 334

#define WIFI_PASSWORD_START WIFI_SSID_START + WIFI_SSID_LEN
#define WIFI_PASSWORD_LEN 100 // 366

#define INT_BLOCK_START 1024
#define INT_BLOCK_MAX 50 * sizeof(int) // (50 4 byte ints)

#define FLT_BLOCK_START 1600
#define FLT_BLOCK_SIZE 50 * sizeof(flt) // 4 bytes per float

#define BOOL_BLOCK_START 1900
#define BOOL_BLOCK_LEN 100 * sizeof(bool)

/* 
  Memory Map:
    0 CONFIG LOCATION - Used to determine the state of the configuration, can be 
        Unkonwn
        Inintialized - Default parameters have been set, and can read/write from EEPROM 
        Commissioned - Device has all the parameters it needs, upon reboot it will be come live
    2 STRUCTURE VERSION - is used to denote the memory map used to start device parameters, if you increment this it will wipe the defaults
    4 DATA VERSION - Each time the parameters are written, this is incremented, this potentially will be used with the device twin concept.
    6 Anonymous Connection, if true uid/pwd will not be sent with connection request (note this is independent of the transport)
    7 Secure Connection, if this is set the connection that is established should be HTTPS or MQTTS, basically some sort of TLS

    Block of parameters that are used on all configurations
    ========================================================
    10 - 50 - Device ID
    60 - 256 - Access Code, Key used for autentication 
    316 - 80 - Server Host Name or IP Address
    396 - 60 - User name used if not Anonymous
    456 - 60 - Password used if not Anonymous
    516 - 50 - WiFi SSID
    566 - 50 - WiFi Password
   
    Block of parameters that are specific to an application
    values are syncronized either with a textual key, this
    value store is responsible for mapping keys to storage
    locations.

    Example
    key = "calfactor"   index = 0 type = float
    key = "updaterate"  index = 0 type = short
    
    Note that keys and storage locations need to be mapped
    for specific types, that way it knows which bucket the
    and slot within that bucket the value should be stored
    and retreived.
    ========================================================
    
    Parameters storage starts at byte 600
    600 - 50 x 2 byte values SHORT 
    700 - 10 x 8 byte values LONG
    800 - 20 x 8 byte values FLOAT
    960 - 20 x 1 byte value BOOL
 */

NuvIoTState::NuvIoTState(Display *display, BluetoothSerial *btSerial, Hal *hal, Console *console)
{
    m_display = display;
    m_hal = hal;
    m_console = console;
    m_btSerial = btSerial;
}

void NuvIoTState::init(String firmwareSku, String firmwareVersion, String deviceConfigKey, uint16_t structureVersion)
{
    m_firmwareSku = firmwareSku;
    m_firmwareVersion = firmwareVersion;

    EEPROM.begin(2048);

    // are we fully ready to be online?
    m_isCommissioned = EEPROM.readUShort(IS_CONFIG_LOCATION) == COMMISSIONED_ID;
    if (m_isCommissioned)
    {
        // if so we must be initialized.
        m_isInitialized = true;
    }
    else
    {
        short initValue = EEPROM.readUShort(IS_CONFIG_LOCATION);
        // have the data values been initialized in EEPROM?
        m_isInitialized = EEPROM.readUShort(IS_CONFIG_LOCATION) == INITIALIZED_ID;
    }

    m_DeviceId = (!m_isInitialized) ? "?" : readString(DEVICEID_START, DEVICEID_LEN);
    String btSerialName = "NuvIoT - " + (m_isInitialized ? m_DeviceId : firmwareSku);
    m_btSerial->begin(btSerialName); //Name of your Bluetooth Signal

    const uint8_t *bt_addr = esp_bt_dev_get_address();

    sprintf(m_btAddress, "%02X:%02X:%02X:%02X:%02X:%02X",
            (int)bt_addr[0],
            (int)bt_addr[1],
            (int)bt_addr[2],
            (int)bt_addr[3],
            (int)bt_addr[4],
            (int)bt_addr[5]);

    m_btAddress[17] = 0x00;

    if (!m_isInitialized)
    {
        m_console->printVerbose("State -> Not init.");

        m_anonymous = true;
        m_secureTransport = false;
        m_HostName = "?";
        m_HostUserName = "?";
        m_HostPassword = "?";
        m_DeviceAccessKey = "?";
        m_WiFiSSID = "?";
        m_WiFiPassword = "?";

        m_intSetValueMask = 0x00;
        m_fltSetValueMask = 0x00;
        m_boolSetValueMask = 0x00;

        EEPROM.writeLong64(INT_SET_MASK_START, m_intSetValueMask);
        EEPROM.writeLong64(FLT_SET_MASK_START, m_fltSetValueMask);
        EEPROM.writeLong64(BOOL_SET_MASK_START, m_boolSetValueMask);

        writeString(DEVICEID_START, m_DeviceId);
        writeString(DEVICE_ACCESS_CODE_START, m_DeviceAccessKey);
        EEPROM.writeBool(ANONYOUS_SERVER_CONN_START, m_anonymous);
        EEPROM.writeBool(SECURE_TRANSPORT_START, m_secureTransport);
        writeString(SERVER_USER_NAME_START, m_HostUserName);
        writeString(SERVER_PASSWORD, m_HostPassword);
        writeString(SERVER_HOST, m_HostName);
        writeString(WIFI_SSID_START, m_WiFiSSID);
        writeString(WIFI_PASSWORD_START, m_WiFiPassword);
        EEPROM.writeUShort(IS_CONFIG_LOCATION, INITIALIZED_ID);
        short initValue = EEPROM.readUShort(IS_CONFIG_LOCATION);
    }
    else
    {
        m_console->printVerbose("State - Has Datat.");

        m_intSetValueMask = EEPROM.readLong64(INT_SET_MASK_START);
        m_fltSetValueMask = EEPROM.readLong64(FLT_SET_MASK_START);
        m_boolSetValueMask = EEPROM.readLong64(BOOL_SET_MASK_START);

        m_HostName = readString(SERVER_HOST, SERVER_HOST_LEN);
        m_anonymous = EEPROM.readBool(ANONYOUS_SERVER_CONN_START);
        m_HostUserName = readString(SERVER_USER_NAME_START, SERVER_USER_NAME_LEN);
        m_HostPassword = readString(SERVER_PASSWORD, SERVER_PASSWORD_LEN);
        m_DeviceAccessKey = readString(DEVICE_ACCESS_CODE_START, DEVICE_ACCESS_CODE_LEN);
        m_WiFiSSID = readString(WIFI_SSID_START, WIFI_SSID_LEN);
        m_WiFiPassword = readString(WIFI_PASSWORD_START, WIFI_PASSWORD_LEN);
    }

    createDefaults();

    Param *pNode = m_pIntParamHead;
    while (pNode != NULL)
    {
        m_console->println(String(pNode->getIndex() + ". " + String(pNode->getKey())));
        pNode = pNode->pNext;
    }

    if (!m_isCommissioned)
    {
        m_display->clearBuffer();
        if (m_DeviceId != "?")
        {
            m_display->println(m_DeviceId.c_str());
        }
        m_display->println("Please Config");
        m_display->println("BT Addr:");
        m_display->println(m_btAddress);
        m_display->sendBuffer();
    }

    EEPROM.commit();
}

bool NuvIoTState::isValid()
{
    return m_isCommissioned;
}

bool NuvIoTState::getVerboseLogging()
{
    return m_verboseLogging;
}

bool NuvIoTState::getDebugMode()
{
    return m_debugMode;
}

void NuvIoTState::setDebugMode(bool mode)
{
    m_debugMode = mode;
}

String NuvIoTState::getWiFiSSID()
{
    return m_WiFiSSID;
}

String NuvIoTState::getWiFiPassword()
{
    return m_WiFiPassword;
}

String NuvIoTState::getDeviceId()
{
    return m_DeviceId;
}

String NuvIoTState::getHostName()
{
    return m_HostName;
}

String NuvIoTState::getDeviceAccessKey()
{
    return m_DeviceAccessKey;
}

bool NuvIoTState::getIsAnonymous()
{
    return m_anonymous;
}

bool NuvIoTState::getSecureTransport()
{
    return m_secureTransport;
}

String NuvIoTState::getHostUserName()
{
    return m_anonymous ? "" : m_HostUserName;
}

String NuvIoTState::getHostPassword()
{
    return m_anonymous ? "" : m_HostPassword;
}

void NuvIoTState::createDefaults()
{
    Param *pNext = m_pFloatParamHead;
    while (pNext != NULL)
    {
        uint64_t fieldMask = (uint64_t)pow(2, pNext->getIndex());
        if ((m_fltSetValueMask & fieldMask) == fieldMask)
        {
#ifdef STATE_VERBOSE
            m_console->printVerbose("Flt [" + String(pNext->getKey()) + "] set at " + String(pNext->getIndex()) + " msk: " + String((int)fieldMask));
#endif
        }
        else
        {
#ifdef STATE_VERBOSE
            m_console->printVerbose("Flt [" + String(pNext->getKey()) + "] not set, default: " + String(pNext->getFltDefault()) + " at " + String(pNext->getIndex()) + " msk: " + String((int)fieldMask));
#endif
            EEPROM.writeFloat(FLT_BLOCK_START + pNext->getIndex() * sizeof(float), pNext->getFltDefault());
            m_fltSetValueMask = m_fltSetValueMask | fieldMask;
        }
        pNext = pNext->pNext;
    }

    pNext = m_pIntParamHead;
    while (pNext != NULL)
    {
        uint64_t fieldMask = (uint64_t)pow(2, pNext->getIndex());
        if ((m_intSetValueMask & fieldMask) == fieldMask)
        {
#ifdef STATE_VERBOSE
            m_console->printVerbose("Int [" + String(pNext->getKey()) + "] at " + String(pNext->getIndex()) + " msk: " + String((int)fieldMask));
#endif
        }
        else
        {
#ifdef STATE_VERBOSE
            m_console->printVerbose("Int [" + String(pNext->getKey()) + "] not set, default: " + String(pNext->getIntDefault()) + " at " + String(pNext->getIndex()) + " msk: " + String((int)fieldMask));
#endif
            EEPROM.writeInt(INT_BLOCK_START + pNext->getIndex() * sizeof(int), pNext->getIntDefault());

            m_intSetValueMask = m_intSetValueMask | fieldMask;
        }
        pNext = pNext->pNext;
    }

    pNext = m_pBoolParamHead;
    while (pNext != NULL)
    {
        uint64_t fieldMask = (uint64_t)pow(2, pNext->getIndex());
        if ((m_boolSetValueMask & fieldMask) == fieldMask)
        {
#ifdef STATE_VERBOSE
            m_console->printVerbose("Bool [" + String(pNext->getKey()) + "] at " + String(pNext->getIndex()) + " msk: " + String((int)fieldMask));
#endif
        }
        else
        {
#ifdef STATE_VERBOSE
            m_console->printVerbose("Bool [" + String(pNext->getKey()) + "] not set, default: " + String(pNext->getBoolDefault()) + " at " + String(pNext->getIndex()) + " msk: " + String((int)fieldMask));
#endif
            EEPROM.writeInt(BOOL_BLOCK_START + pNext->getIndex() * sizeof(bool), pNext->getBoolDefault());
            m_boolSetValueMask = m_boolSetValueMask | fieldMask;
        }
        pNext = pNext->pNext;
    }

    EEPROM.writeLong64(INT_SET_MASK_START, m_intSetValueMask);
    EEPROM.writeLong64(FLT_SET_MASK_START, m_fltSetValueMask);
    EEPROM.writeLong64(BOOL_BLOCK_START, m_boolSetValueMask);
}

String NuvIoTState::queryState()
{
    String state =
        "firmwareSku=" + m_firmwareSku + ", " +
        "firmwareVersion=" + m_firmwareVersion;

    Param *pNext = m_pBoolParamHead;
    while (pNext != NULL)
    {
        state += ", " + String(pNext->getKey()) + "=" + getBool(pNext->getKey());
        pNext = pNext->pNext;
    }

    pNext = m_pIntParamHead;
    while (pNext != NULL)
    {
        state += ", " + String(pNext->getKey()) + "=" + getInt(pNext->getKey());
        pNext = pNext->pNext;
    }

    pNext = m_pFloatParamHead;
    while (pNext != NULL)
    {
        state += ", " + String(pNext->getKey()) + "=" + getFlt(pNext->getKey());
        pNext = pNext->pNext;
    }

    state += ";";
    return state;
}

String NuvIoTState::getRemoteProperties()
{
    String state =
        "PROPERTIES:readonly-firmwareSku=" + m_firmwareSku + "," +
        "readonly-firmwareVersion=" + m_firmwareVersion;

    Param *pNext = m_pBoolParamHead;
    while (pNext != NULL)
    {
        state += ",Boolean-" + String(pNext->getKey()) + "=" + getBool(pNext->getKey());
        pNext = pNext->pNext;
    }

    pNext = m_pIntParamHead;
    while (pNext != NULL)
    {
        state += ",Integer-" + String(pNext->getKey()) + "=" + getInt(pNext->getKey());
        pNext = pNext->pNext;
    }

    pNext = m_pFloatParamHead;
    while (pNext != NULL)
    {
        state += ",Decimal-" + String(pNext->getKey()) + "=" + getFlt(pNext->getKey());
        pNext = pNext->pNext;
    }

    state += ";";
    return state;
}

int _lstSend = 0;

void NuvIoTState::readFirmware()
{
    m_console->enableBTOut(false);

    m_console->println("Start DFU");
    m_btSerial->print("ok-start\n");

    while (m_btSerial->available() < 4)
    {
        delay(1);
    }

    char progressBar[110];

    int32_t total = m_btSerial->read() << 24 | m_btSerial->read() << 16 | m_btSerial->read() << 8 | m_btSerial->read();

    m_console->println("Expecting [" + String(total) + "] bytes");
    m_btSerial->print("ok-size:" + String(total) + "\n");

    while (m_btSerial->available() < 2)
    {
        delay(1);
    }

    short blockCount = m_btSerial->read() << 8 | m_btSerial->read();
    byte buffer[512];

    m_console->println("Started reading [" + String(blockCount) + "] blocks");
    m_btSerial->print("ok-blocks:" + String(blockCount) + "\n");

    int blocksReceived = 0;

    if (!Update.begin(total, U_FLASH))
    {
        m_btSerial->print("fail-start\n");
        m_console->println("Could not start DFU process.");
        m_display->println("ERR: fail start DFU");
        delay(2000);
        return;
    }

    for (int idx = 0; idx < blockCount; ++idx)
    {
        while (m_btSerial->available() < 2)
            delay(1);

        short blockSize = m_btSerial->read() << 8 | m_btSerial->read();
        Serial.println("Expecting block size of " + String(blockSize));

        while (m_btSerial->available() < blockSize + 1)
        {
            if (millis() - _lstSend > 1000)
            {
                _lstSend = millis();
                m_console->println("Bytes in hopper [" + String(m_btSerial->available()) + "].");
            }
            delay(1);
        }

        byte calcCheckSum = 0;

        for (int ch = 0; ch < blockSize; ++ch)
        {
            buffer[ch] = (byte)m_btSerial->read();
            calcCheckSum += buffer[ch];
        }

        byte actualCheckSum = m_btSerial->read();
        m_console->printVerbose("Block: (" + String(idx) + "/" + String(blockCount) + ") " + String(blockSize) + " " + calcCheckSum + " " + actualCheckSum + " " + buffer[0] + " " + buffer[1] + "  " + buffer[498] + " " + buffer[499]);

        if (actualCheckSum != calcCheckSum)
        {
            m_btSerial->print("fail-checksum:" + String(idx) + "\n");
            return;
        }
        else
        {
            m_btSerial->print("ok-recv:" + String(idx) + "\n");
        }

        blocksReceived++;

        size_t written = Update.write(buffer, blockSize);
        if (written > 0)
        {
            int percentCommplete = (100 * blocksReceived) / blockCount;
            String progress = "Progress " + String(percentCommplete);
            const char *str = progress.c_str();
            m_console->println(progress);
            for (int pctIdx = 0; idx < percentCommplete / 10; ++pctIdx)
            {
                progressBar[pctIdx] = '.';
            }

            progressBar[percentCommplete] = 0x00;

            m_display->drawStr("Updating Firmware", str, progressBar);
        }
        else
        {
            m_console->printError("Could not flash file");
            m_console->printError("Could not write byte array.");
            m_display->drawStr("Flashing failed");
            m_btSerial->print("fail-write:" + String(idx) + "\n");
            delay(2000);
            return;
        }

        delay(5);
    }

    m_btSerial->print("ok-recv:all\n");

    if (blocksReceived == blockCount)
    {
        if (Update.end())
        {
#ifdef MQTT_VERBOSE
            m_console->println("Success flashing, pausing and then restarting.");
#endif
            m_display->drawStr("Success flashing", "Restarting");
            delay(2000);

            m_btSerial->print("ok-update:success\n");

            m_console->println("Success flashing, restarting.");
            m_hal->restart();
        }
        else
        {
            m_console->printError("Could not flash file");
   
            m_btSerial->print("ok-update:fail\n");

            m_display->drawStr("Flasing Failed", "MD5 Error");
            delay(2000);
        }
    }
    else
    {
        m_console->printError("Could not download file.");
        m_console->printError("Downloaded: " + String(blocksReceived) + " total:" + String(blockCount));

        m_display->drawStr("Flashing Failed", "Download Error");
    }

    m_console->enableBTOut(true);
}

bool NuvIoTState::getIsConfigurationModeActive()
{
    return m_configurationMode;
}

bool NuvIoTState::getIsPaused()
{
    return m_paused;
}

void NuvIoTState::loop()
{
    while (m_btSerial->available() > 0)
    {
        int ch = m_btSerial->read();

        if (ch == '\n')
        {
            m_messageBuffer[m_messageBufferTail++] = 0x00;
            String msg = String(m_messageBuffer);
            Serial.println(msg);

            if (msg == "HELLO")
            {
                m_display->clearBuffer();
                m_display->println("Welcome");
                m_display->println("Configuration Mode");
                m_display->sendBuffer();
                m_configurationMode = true;
            }
            else if (msg == "PAUSE")
            {
                m_paused = true;
            }
            else if (msg == "CONTINUE")
            {
                m_paused = false;
            }
            else if (msg == "FIRMWARE")
            {
                readFirmware();
            }
            else if (msg == "REBOOT")
            {
                m_hal->restart();
            }
            else if (msg == "PROPERTIES")
            {
                m_btSerial->println(getRemoteProperties());
            }
            else if (msg.substring(0, 3) == "SET")
            {
                String setCommand = String(&m_messageBuffer[4]);
                int dashIdx = setCommand.indexOf('-');
                int equalsIdx = setCommand.indexOf("=");
                String type = setCommand.substring(0, dashIdx);
                String key = setCommand.substring(dashIdx + 1, equalsIdx);
                String value = setCommand.substring(equalsIdx + 1);

                Serial.println(type + "] - [" + key + "] - [" + value + "]");
                updateProperty(type, key, value);
            }
            else if (msg == "QUERY")
            {
                m_btSerial->print("deviceid=" + m_DeviceId + "\n");
                m_btSerial->print("mqtthost=" + m_HostName + "\n");
                m_btSerial->print("mqttanonymouse=");
                m_btSerial->print(m_anonymous ? "true\n" : "false\n");

                if (!m_anonymous)
                {
                    m_btSerial->print("mqttuid=" + m_HostUserName + "\n");
                    m_btSerial->print("mqttpwd=" + m_HostPassword + "\n");
                }

                if (m_DeviceAccessKey != NULL && m_DeviceAccessKey.length() > 0)
                {
                    m_btSerial->print("key=" + m_DeviceAccessKey + "\n");
                }

                if (m_WiFiSSID != NULL && m_WiFiSSID.length() > 0)
                {
                    m_btSerial->print("ssid=" + m_WiFiSSID + "\n");
                    m_btSerial->print("wifipwd=" + m_WiFiPassword + "\n");
                }

                m_btSerial->print("fwsku=" + m_firmwareSku + "\n");
                m_btSerial->print("fwversion=" + m_firmwareVersion + "\n");
                m_btSerial->print("hwversion=2\n");
                m_btSerial->print("platform=ESP32\n");
            }
            else if (msg == "QUIT")
            {
                m_configurationMode = false;
            }
            else if (msg == "COMMISSION")
            {
                EEPROM.writeUShort(IS_CONFIG_LOCATION, COMMISSIONED_ID);
                EEPROM.commit();
                m_btSerial->println("999,ACK\n");
                m_hal->restart();
            }
            else
            {
                switch (m_messageBuffer[0])
                {
                case '0': // Device ID;
                    m_DeviceId = String(&m_messageBuffer[2]);
                    writeString(DEVICEID_START, m_DeviceId);
                    m_btSerial->println("0,ACK;");
                    break;
                case '1': // Server Host Name
                    m_HostName = String(&m_messageBuffer[2]);
                    writeString(SERVER_HOST, m_HostName);
                    m_btSerial->println("1,ACK;");
                    break;
                case '2': // Anonyous
                    m_anonymous = String(&m_messageBuffer[2]) == "true";
                    EEPROM.writeBool(ANONYOUS_SERVER_CONN_START, m_anonymous);
                    if (m_anonymous)
                    {
                        writeString(SERVER_USER_NAME_START, "");
                        writeString(SERVER_PASSWORD, "");
                    }
                    m_btSerial->println("2,ACK;");
                    break;
                case '3': // Server User Name
                    m_HostUserName = String(&m_messageBuffer[2]);
                    writeString(SERVER_USER_NAME_START, m_HostUserName);
                    m_btSerial->println("3,ACK;");
                    break;
                case '4': // Server Password
                    m_HostPassword = String(&m_messageBuffer[2]);
                    writeString(SERVER_PASSWORD, m_HostPassword);
                    m_btSerial->println("4,ACK;");
                    break;
                case '5': // Device Access m_DeviceAccessKey
                    m_DeviceAccessKey = String(&m_messageBuffer[2]);
                    writeString(DEVICE_ACCESS_CODE_START, m_DeviceAccessKey);
                    m_btSerial->println("5,ACK;");
                    break;
                case '6':
                    m_WiFiSSID = String(&m_messageBuffer[2]);
                    writeString(WIFI_SSID_START, m_WiFiSSID);
                    m_btSerial->println("6,ACK;");
                    break;
                case '7':
                    m_WiFiPassword = String(&m_messageBuffer[2]);
                    writeString(WIFI_PASSWORD_START, m_WiFiPassword);
                    m_btSerial->println("7,ACK;");
                    break;
                }
            }

            m_messageBufferTail = 0;
        }
        else
        {
            m_messageBuffer[m_messageBufferTail++] = ch;
        }
    }
}

Param *NuvIoTState::findKey(Param *pHead, const char *key)
{
    Param *pNext = pHead;

    while (pNext != NULL)
    {
        if (strcmp(key, pNext->getKey()) == 0)
        {
            return pNext;
        }

        pNext = pNext->pNext;
    }

    return NULL;
}

bool NuvIoTState::getBool(String key)
{
    Param *pParam = findKey(m_pBoolParamHead, key.c_str());
    if (pParam != NULL)
    {
        return EEPROM.readBool(BOOL_BLOCK_START + pParam->getIndex() * sizeof(bool));
    }

    return 0;
}

int NuvIoTState::getInt(String key)
{
    Param *pParam = findKey(m_pIntParamHead, key.c_str());
    if (pParam != NULL)
    {
        int addr = INT_BLOCK_START + pParam->getIndex() * sizeof(int);

        int result = EEPROM.readInt(addr);
        return result;
    }

    return 0;
}

float NuvIoTState::getFlt(String key)
{
    Param *pParam = findKey(m_pFloatParamHead, key.c_str());
    if (pParam != NULL)
    {
        return EEPROM.readFloat(FLT_BLOCK_START + pParam->getIndex() * sizeof(float));
    }

    return 0;
}

void NuvIoTState::updateProperty(String fieldType, String field, String value)
{
    if (fieldType == "Integer")
    {
        Param *pParam = findKey(m_pIntParamHead, field.c_str());
        if (pParam != NULL)
        {
            int intValue = atoi(value.c_str());
            int addr = INT_BLOCK_START + pParam->getIndex() * sizeof(intValue);
            EEPROM.writeInt(addr, intValue);
            EEPROM.commit();
#ifdef STATE_VERBOSE
            m_console->printVerbose("SET int " + field + " " + String(intValue) + " at address " + String(addr));
#endif
        }
    }
    else if (fieldType == "Decimal")
    {
        Param *pParam = findKey(m_pFloatParamHead, field.c_str());
        if (pParam != NULL)
        {
            float floatValue = atof(value.c_str());
            EEPROM.writeFloat(FLT_BLOCK_START + pParam->getIndex() * sizeof(float), floatValue);
            EEPROM.commit();
#ifdef STATE_VERBOSE
            m_console->printVerbose("SET float " + field + "=" + String(floatValue));
#endif
        }
    }
    else if (fieldType == "Boolean")
    {
        Param *pParam = findKey(m_pBoolParamHead, field.c_str());
        if (pParam != NULL)
        {
            bool boolValue = value == "true";
            EEPROM.writeBool(BOOL_BLOCK_START + pParam->getIndex() * sizeof(float), boolValue);
            EEPROM.commit();
#ifdef STATE_VERBOSE
            m_console->printVerbose("SET bool " + field + "=" + String(boolValue));
#endif
        }
    }
}

void NuvIoTState::setBool(int idx, boolean value)
{
    EEPROM.writeBool(idx, value);
    EEPROM.commit();
}

void NuvIoTState::setByte(int idx, byte value)
{
    EEPROM.writeByte(idx, value);
    EEPROM.commit();
}

void NuvIoTState::setInt(int idx, int value)
{
    EEPROM.writeInt(idx, value);
}

void NuvIoTState::setFloat(int idx, float value)
{
    EEPROM.writeFloat(idx, value);
}

void NuvIoTState::writeString(int adddr, String data)
{
    Serial.println("Write " + data + " " + String(adddr));

    int _size = data.length();
    for (int i = 0; i < _size; i++)
    {
        EEPROM.write(adddr + i, data[i]);
    }
    EEPROM.write(adddr + _size, '\0'); //Add termination null character for String Data
    EEPROM.commit();
}

String NuvIoTState::readString(int addr, int maxLength)
{
    int i;
    char data[100]; //Max 100 Bytes
    int len = 0;
    unsigned char k;
    k = EEPROM.read(addr);
    while (k != '\0' && len < maxLength) //Read until null character
    {
        k = EEPROM.read(addr + len);
        data[len] = k;
        len++;
    }
    data[len] = '\0';
    return String(data);
}

Param *NuvIoTState::appendValue(Param *pHead, Param *pNode)
{
    Param *pNext = pHead;

    int idx = 1;
    while (pNext != NULL)
    {
        if (pNext->pNext == NULL)
        {
            pNode->setIndex(idx);
            pNext->pNext = pNode;
#ifdef STATE_VERBOSE
            m_console->printVerbose("Add [" + String(pNode->getKey()) + "] idx [" + String(pNode->getIndex()) + "]");
#endif

            return pNode;
        }
        else
        {
            pNext = pNext->pNext;
            ++idx;
        }
    }
}

void NuvIoTState::registerInt(const char *key, int defaultValue)
{
    Param *p = new Param(key, defaultValue);

    if (m_pIntParamHead == NULL)
    {
        p->setIndex(0);
#ifdef STATE_VERBOSE
        m_console->printVerbose("Reg int [" + String(p->getKey()) + "]=[" + String(p->getIntDefault()) + "] idx [" + String(p->getIndex()) + "]");
#endif
        m_pIntParamHead = p;
    }
    else
    {
        appendValue(m_pIntParamHead, p);
    }
}

void NuvIoTState::registerFloat(const char *key, float defaultValue)
{
    Param *p = new Param(key, defaultValue);

    if (m_pFloatParamHead == NULL)
    {
        p->setIndex(0);
#ifdef STATE_VERBOSE
        m_console->printVerbose("Reg flt [" + String(p->getKey()) + "]=[" + String(p->getFltDefault()) + "] idx [" + String(p->getIndex()) + "]");
#endif
        m_pFloatParamHead = p;
    }
    else
    {
        appendValue(m_pFloatParamHead, p);
    }
}

void NuvIoTState::registerBool(const char *key, boolean defaultValue)
{
    Param *p = new Param(key, defaultValue);

    if (m_pBoolParamHead == NULL)
    {
        p->setIndex(0);
#ifdef STATE_VERBOSE
        m_console->printVerbose("Reg bool [" + String(p->getKey()) + "]=[" + String(p->getBoolDefault()) + "] idx [" + String(p->getIndex()) + "]");
#endif

        m_pBoolParamHead = p;
    }
    else
    {
        appendValue(m_pBoolParamHead, p);
    }
}

String NuvIoTState::getFirmwareVersion() { return m_firmwareVersion; }
String NuvIoTState::getFirmwareSKU() { return m_firmwareSku; }