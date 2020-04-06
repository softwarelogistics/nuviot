#ifndef NuvIoTState_h
#define NuvIoTState_h

#include <Arduino.h>
#include <EEPROM.h>
#include <BluetoothSerial.h>
#include "Display.h"
#include "Hal.h"
#include "Logger.h"

enum Storage_DataTypes
{
    BooleanStorage,
    ShortStorage,
    LongStorage,
    DoubleStorage,
};

class Param
{
private:
    int m_iIndex;
    int m_intDefault;
    float m_fltDefault;
    bool m_boolDefault;
    const char *m_key;

public:
    Param(const char *key, int value) { m_key = key; m_intDefault = value; }
    Param(const char *key, float value) { m_key = key; m_fltDefault = value; }
    Param(const char *key, bool value) { m_key = key; m_boolDefault = value; }

    const char *getKey() { return m_key; }
    void setIndex(int idx) { m_iIndex = idx; }
    int getIndex() { return m_iIndex; }
    int getIntDefault() { return m_intDefault; }
    bool getBoolDefault() { return m_boolDefault; }
    float getFltDefault() { return m_fltDefault; }

    Param *pNext = NULL;
};

class NuvIoTState
{
private:
    BluetoothSerial m_btSerial;
    Display *m_display;
    Hal *m_hal;
    Logger *m_logger;

    bool m_isInitialized = false;
    bool m_isCommissioned = false;
    bool m_debugMode = false;
    bool m_verboseLogging = false;

    String readString(int add, int maxLength);
    String m_deviceAddress;
    void writeString(int add, String data);
    char m_messageBuffer[128];
    char m_btAddress[20];
    int m_messageBufferTail = 0;
    int byteCount = 0;

    String m_firmwareSku;
    String m_firmwareVersion;
    String m_DeviceId;
    String m_DeviceAccessKey;
    String m_HostName;
    String m_HostUserName;
    String m_HostPassword;

    String m_WiFiSSID;
    String m_WiFiPassword;

    bool m_anonymous;
    bool m_secureTransport;

    /* These three values use bit masking to signify that the value has
           been initialized in EEPROM, this allows multiple values to be set */
    uint64_t m_intSetValueMask;
    uint64_t m_fltSetValueMask;
    uint64_t m_boolSetValueMask;

    Param *m_pBoolParamHead = NULL;
    Param *m_pIntParamHead = NULL;
    Param *m_pFloatParamHead = NULL;
    Param *appendValue(Param *pHead, Param *pNode);

    Param *findKey(Param *pHead, const char *key);
    void createDefaults();

public:
    NuvIoTState(Display *display, Hal *hal, Logger *logger);
    void init(String firmwareSku, String firmwareVersion, String deviceConfigKey, uint16_t deviceConfigVersion);
    bool isValid();
    void loop();

    String getDeviceId();
    String getDeviceAccessKey();
    String getHostName();
    String getHostUserName();
    String getHostPassword();
    String getWiFiSSID();
    String getWiFiPassword();

    bool getVerboseLogging();
    bool getDebugMode();
    void setDebugMode(bool mode);

    String queryState();

    bool getSecureTransport();
    bool getIsAnonymous();

    bool getBool(String key);
    byte getByte(String key);
    int getInt(String key);
    long getLong(String key);
    float getFlt(String key);

    void updateProperty(String fieldType, String field, String value);

    void setBool(int idx, boolean value);
    void setByte(int idx, byte value);
    void setInt(int idx, int value);
    void setFloat(int idx, float value);

    void registerInt(const char *key, int defaultValue);
    void registerFloat(const char *key, float defaultValue);
    void registerBool(const char *key, bool defaultValue);
};

#endif