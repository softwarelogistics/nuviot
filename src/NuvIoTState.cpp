#include "NuvIoTState.h"
#include "esp_bt_device.h"
#include "math.h"

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

NuvIoTState::NuvIoTState(Display *display, Hal *hal, Logger *logger)
{
    m_display = display;
    m_hal = hal;
    m_logger = logger;
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
    m_btSerial.begin(btSerialName); //Name of your Bluetooth Signal

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
        m_logger->logVerbose("Not initialized, setting all EEPROM values to default.");

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
        m_logger->logVerbose("Initalized, loading values from EEPROM.");

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

    Serial.println("register ints");
    Param *pNode = m_pIntParamHead;
    while(pNode != NULL){
        Serial.println("Node " + String(pNode->getKey()) + "  " + String(pNode->getIndex()));
        pNode = pNode->pNext;
    }
    Serial.println("register ints");

    if (!m_isCommissioned)
    {
        m_display->clearBuffer();
        m_display->println("Welcome");
        if (m_DeviceId != "?")
        {
            m_display->println(m_DeviceId.c_str());            
        }
        m_display->println( "Please configure");
        m_display->println("Bluetooth Address:");
        m_display->println(m_btAddress);
        m_display->sendBuffer();
    }

    EEPROM.commit();
}

bool NuvIoTState::isValid()
{
    return m_isCommissioned;
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
    return m_HostUserName;
}

String NuvIoTState::getHostPassword()
{
    return m_HostPassword;
}

void NuvIoTState::createDefaults()
{
    Param *pNext = m_pFloatParamHead;
    while (pNext != NULL)
    {
        uint64_t fieldMask = (uint64_t)pow(2, pNext->getIndex());
        if ((m_fltSetValueMask & fieldMask) == fieldMask)
        {
            m_logger->logVerbose("Float value for [" + String(pNext->getKey()) + "] is already set at position " + String(pNext->getIndex()) + " with mask: " + String((int)fieldMask));
        }
        else
        {
            m_logger->logVerbose("Float value for [" + String(pNext->getKey()) + "] is not set, setting initial default to " + String(pNext->getFltDefault()) + " at position " + String(pNext->getIndex()) + " with mask: " + String((int)fieldMask));            
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
            m_logger->logVerbose("Int value [" + String(pNext->getKey()) + "] is already set at position " + String(pNext->getIndex()) + " with mask: " + String((int)fieldMask));
        }
        else
        {
            m_logger->logVerbose("Int value [" + String(pNext->getKey()) + "] is not set, setting initial default to " + String(pNext->getIntDefault()) + " at position " + String(pNext->getIndex()) + " with mask: " + String((int)fieldMask));
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
            m_logger->logVerbose("Bool value for [" + String(pNext->getKey()) + "] is already set at position " + String(pNext->getIndex()) + " with mask: " + String((int)fieldMask));
        }
        else
        {
            m_logger->logVerbose("Bool value for [" + String(pNext->getKey()) + "] is not set, setting initial default to " + String(pNext->getBoolDefault()) + " at position " + String(pNext->getIndex()) + " with mask: " + String((int)fieldMask));
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

void NuvIoTState::loop()
{
    while (m_btSerial.available() > 0)
    {
        int ch = m_btSerial.read();

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
            }
            else if (msg == "REBOOT")
            {
                m_hal->restart();
            }
            else if (msg == "QUERY")
            {
                m_btSerial.print("0," + m_DeviceId + ";");
                m_btSerial.print("1," + m_HostName + ";");
                m_btSerial.print("2,");
                m_btSerial.print(m_anonymous ? "true;" : "false;");
                if (!m_anonymous)
                {
                    m_btSerial.print("3," + m_HostUserName + ";");
                    m_btSerial.print("4," + m_HostPassword + ";");
                }

                m_btSerial.print("5," + m_DeviceAccessKey + ";");
                m_btSerial.print("6," + m_WiFiSSID + ";");

                m_btSerial.print("100," + m_firmwareSku + ";");
                m_btSerial.print("101," + m_firmwareVersion + ";");
                m_btSerial.println("102,ESP32;");
            }
            else if (msg == "COMMISSION")
            {
                EEPROM.writeUShort(IS_CONFIG_LOCATION, COMMISSIONED_ID);
                EEPROM.commit();
                m_btSerial.println("999,ACK\n");
                m_hal->restart();
            }
            else
            {
                switch (m_messageBuffer[0])
                {
                case '0': // Device ID;
                    m_DeviceId = String(&m_messageBuffer[2]);
                    writeString(DEVICEID_START, m_DeviceId);
                    m_btSerial.println("0,ACK;");
                    break;
                case '1': // Server Host Name
                    m_HostName = String(&m_messageBuffer[2]);
                    writeString(SERVER_HOST, m_HostName);
                    m_btSerial.println("1,ACK;");
                    break;
                case '2': // Anonyous
                    m_anonymous = String(&m_messageBuffer[2]) == "true";
                    EEPROM.writeBool(ANONYOUS_SERVER_CONN_START, m_anonymous);
                    if (m_anonymous)
                    {
                        writeString(SERVER_USER_NAME_START, "");
                        writeString(SERVER_PASSWORD, "");
                    }
                    m_btSerial.println("2,ACK;");
                    break;
                case '3': // Server User Name
                    m_HostUserName = String(&m_messageBuffer[2]);
                    writeString(SERVER_USER_NAME_START, m_HostUserName);
                    m_btSerial.println("3,ACK;");
                    break;
                case '4': // Server Password
                    m_HostPassword = String(&m_messageBuffer[2]);
                    writeString(SERVER_PASSWORD, m_HostPassword);
                    m_btSerial.println("4,ACK;");
                    break;
                case '5': // Device Access m_DeviceAccessKey
                    m_DeviceAccessKey = String(&m_messageBuffer[2]);
                    writeString(DEVICE_ACCESS_CODE_START, m_DeviceAccessKey);
                    m_btSerial.println("5,ACK;");
                    break;
                case '6':
                    m_WiFiSSID = String(&m_messageBuffer[2]);
                    writeString(WIFI_SSID_START, m_WiFiSSID);
                    m_btSerial.println("6,ACK;");
                    break;
                case '7':
                    m_WiFiPassword = String(&m_messageBuffer[2]);
                    writeString(WIFI_PASSWORD_START, m_WiFiPassword);
                    m_btSerial.println("7,ACK;");
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
        return EEPROM.readInt(INT_BLOCK_START + pParam->getIndex() * sizeof(int));
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
            EEPROM.writeShort(INT_BLOCK_START + pParam->getIndex() * sizeof(intValue), intValue);
            EEPROM.commit();
            Serial.println("Write int value " + field + " " + String(intValue));
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
            Serial.println("Write float value " + field + " " + String(floatValue));
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

            m_logger->logVerbose("Appending node for [" + String(pNode->getKey()) + "] with index [" + String(pNode->getIndex()) + "]");

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
        m_logger->logVerbose("Register bool to for [" + String(p->getKey()) + "] with value [" + String(p->getIntDefault()) + "] with index [" + String(p->getIndex()) + "]");
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
        m_logger->logVerbose("Register bool to for [" + String(p->getKey()) + "] with value [" + String(p->getFltDefault()) + "] with index [" + String(p->getIndex()) + "]");
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
        m_logger->logVerbose("Register bool to for [" + String(p->getKey()) + "] with value [" + String(p->getBoolDefault()) + "] with index [" + String(p->getIndex()) + "]");
        m_pBoolParamHead = p;
    }
    else
    {
        appendValue(m_pBoolParamHead, p);
    }
}