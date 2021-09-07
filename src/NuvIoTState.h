#ifndef NuvIoTState_h
#define NuvIoTState_h

#include <Arduino.h>
#include <EEPROM.h>
#include "BluetoothServer.h"
#include "Display.h"
#include "LedManager.h"
#include "Hal.h"
#include "Console.h"
#include "IOConfig.h"
#include "SysConfig.h"
#include <FS.h>
#include <nvs.h>

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
    Param(const char *key, int value)
    {
        m_key = key;
        m_intDefault = value;
    }
    Param(const char *key, float value)
    {
        m_key = key;
        m_fltDefault = value;
    }
    Param(const char *key, bool value)
    {
        m_key = key;
        m_boolDefault = value;
    }

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
    Display *m_display;
    LedManager *m_ledManager;
    Hal *m_hal;
    Console *m_console;
    bool m_configurationMode = false;
    IOConfig *m_ioConfig;
    SysConfig *m_sysConfig;
    nvs_handle m_nvsHandle;

    uint8_t m_wifiRSSI = 0;
    uint8_t m_cellRSSI = 0;

    bool m_isCellConnected = false;
    bool m_isWiFiConnected = false;

    bool m_isCloudConnected = false;

    long m_pauseTimeout = 0;

    bool m_isInitialized = false;
    bool m_debugMode = false;
    bool m_verboseLogging = false;
    FS *m_fs;

    char m_jsonBuffer[1024];

#ifdef BT_SERIAL

#endif
    uint16_t m_jsonBufferTail = 0;

    String readString(int add, int maxLength);
    String m_deviceAddress;

    int byteCount = 0;

    String m_firmwareSku;
    String m_firmwareVersion;
    String m_hardwareRevision;
    String m_ipAddress;

    bool m_paused;

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
    String resolveError(esp_err_t err);

public:
    NuvIoTState(Display *display, IOConfig *ioConfig, SysConfig *sysConfig, LedManager *ledManager, FS *fs, Hal *hal, Console *console);
    void init(String firmwareSku, String firmwareVersion, String hardwareRevision, String deviceConfigKey, uint16_t deviceConfigVersion);
    bool isValid();
    void loop();

    uint16_t OTAState;
    uint16_t OTAParam;

    String getDeviceId();
    String getDeviceAccessKey();
    String getWiFiSSID();
    String getWiFiPassword();
    String getHardwareRevision();
    String getFirmwareVersion();
    String getFirmwareSKU();

    bool getIsWiFiConnected() { return m_isWiFiConnected; }
    void setIsWiFiConnected(bool connected) { m_isWiFiConnected = connected; }

    bool getIsCellConnected() { return m_isCellConnected; }
    void setIsCellConnected(bool connected) { m_isCellConnected = connected; };

    String getIPAddress() { return m_ipAddress; }
    void setIPAddress(String value) { m_ipAddress = value; };

    uint8_t getCellRSSI() { return m_cellRSSI; }
    void setCellRSSI(uint8_t value) { m_cellRSSI = value; };

    uint8_t getWiFiRSSI() { return m_wifiRSSI; }
    void setWiFiRSSI(uint8_t value) { m_wifiRSSI = value; };

    bool getIsCloudConnected() {return m_isCloudConnected;}
    void setIsCloudConnected(bool connected) {m_isCloudConnected = connected;}

    bool getVerboseLogging();
    bool getDebugMode();
    void setDebugMode(bool mode);

    bool getIsConfigurationModeActive();
    bool getIsPaused();

    String queryFirmwareVersion();
    String getRemoteProperties();
    String getIOConfigSettings();

    void handleConsoleCommand(String cmd);

    bool getSecureTransport();
    bool getIsAnonymous();

    void setADCConfig(int idx, uint8_t config, float scaler);
    void setIOCConfig(int idx, uint8_t config, float scaler);
    void persistConfig();

    void updateProperty(String fieldType, String field, String value);

    void registerInt(const char *key, int32_t defaultValue);
    void registerFloat(const char *key, float defaultValue);
    void registerBool(const char *key, bool defaultValue);

    bool getBool(String key);
    int32_t getInt(String key);
    float getFlt(String key);

private:
    void readFirmware();
};

#endif