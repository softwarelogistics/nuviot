#include "NuvIoTState.h"
#include "esp_bt_device.h"
#include "math.h"
#include <Update.h>

#include <nvs.h>

#define FLOAT_DECIMAL_SCALER 1000.0f

NuvIoTState::NuvIoTState(Display *display, IOConfig *config, SysConfig *sysConfig, LedManager *ledManager, FS *fs, Hal *hal, Console *console)
{
    m_display = display;
    m_hal = hal;
    m_console = console;
    m_ioConfig = config;
    m_sysConfig = sysConfig;
    m_ledManager = ledManager;
}

void NuvIoTState::init(String firmwareSku, String firmwareVersion, String hardwareRevision, String deviceModelKey, uint16_t structureVersion)
{
    m_firmwareSku = firmwareSku;
    m_firmwareVersion = firmwareVersion;
    m_hardwareRevision = hardwareRevision;    
    m_deviceModelKey = deviceModelKey;
    
    esp_err_t openStat = nvs_open_from_partition("nvs", "kvp", NVS_READWRITE, &m_nvsHandle);
    if (openStat != ESP_OK)
    {
        String err = "UNKNOWN: " + String(openStat);

        switch (openStat)
        {
        case ESP_ERR_NVS_NOT_INITIALIZED:
            err = "NVS Not Initialized.";
            break;
        case ESP_ERR_NVS_PART_NOT_FOUND:
            err = "Partition Not Found.";
            break;
        case ESP_ERR_NVS_NOT_FOUND:
            err = "NS Not Exist, mode readonly.";
            break;
        case ESP_ERR_NVS_INVALID_NAME:
            err = "Invalid namespace name.";
            break;
        }

        m_display->drawStr("Could not initialize", "NVS KVP Storage", err.c_str());
        m_console->repeatFatalError("nvskvp=notinitialized; // " + err);
    }
    else
    {
        m_console->printVerbose("nvskvp=initialized;");
    }

    int countDown = 5000;
    m_console->println("pauseforbt=start;  // pause on startup.");
    while(countDown-- > 0) {
        loop();
        delay(1);
    }
    m_console->println("pauseforbt=complete;  // pause on startup.");
}

bool NuvIoTState::isValid()
{
    return m_sysConfig->Commissioned;
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
    return m_sysConfig->WiFiSSID;
}

String NuvIoTState::getWiFiPassword()
{
    return m_sysConfig->WiFiPWD;
}

String NuvIoTState::getDeviceAccessKey()
{
    return m_sysConfig->DeviceAccessKey;
}

bool NuvIoTState::getIsAnonymous()
{
    return m_sysConfig->Anonymous;
}

bool NuvIoTState::getSecureTransport()
{
    return m_sysConfig->TLS;
}

String NuvIoTState::queryFirmwareVersion()
{
    String state =
        "firmwareSku=" + m_firmwareSku + ", " +
        "firmwareVersion=" + m_firmwareVersion + ";";

    return state;
}

#define DFU_TIMEOUT 10000

String NuvIoTState::getRemoteProperties()
{
    String state = "firmwareSku=" + m_firmwareSku + "," + "firmwareVersion=" + m_firmwareVersion;

    Param *pNext = m_pBoolParamHead;
    while (pNext != NULL)
    {
        state += ",Boolean-" + String(pNext->getKey()) + "=" + getBool(pNext->getKey()) ? "true" : "false";
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

    return state;
}

String NuvIoTState::getIOConfigSettings()
{
    return String(m_ioConfig->ADC1Config) + "," + String(m_ioConfig->ADC1Scaler) + "," +
           String(m_ioConfig->ADC2Config) + "," + String(m_ioConfig->ADC2Scaler) + "," +
           String(m_ioConfig->ADC3Config) + "," + String(m_ioConfig->ADC3Scaler) + "," +
           String(m_ioConfig->ADC4Config) + "," + String(m_ioConfig->ADC4Scaler) + "," +
           String(m_ioConfig->ADC5Config) + "," + String(m_ioConfig->ADC5Scaler) + "," +
           String(m_ioConfig->ADC6Config) + "," + String(m_ioConfig->ADC6Scaler) + "," +
           String(m_ioConfig->ADC7Config) + "," + String(m_ioConfig->ADC7Scaler) + "," +
           String(m_ioConfig->ADC8Config) + "," + String(m_ioConfig->ADC8Scaler) + "," +
           String(m_ioConfig->GPIO1Config) + "," + String(m_ioConfig->GPIO1Scaler) + "," +
           String(m_ioConfig->GPIO2Config) + "," + String(m_ioConfig->GPIO2Scaler) + "," +
           String(m_ioConfig->GPIO3Config) + "," + String(m_ioConfig->GPIO3Scaler) + "," +
           String(m_ioConfig->GPIO4Config) + "," + String(m_ioConfig->GPIO4Scaler) + "," +
           String(m_ioConfig->GPIO5Config) + "," + String(m_ioConfig->GPIO5Scaler) + "," +
           String(m_ioConfig->GPIO6Config) + "," + String(m_ioConfig->GPIO6Scaler) + "," +
           String(m_ioConfig->GPIO7Config) + "," + String(m_ioConfig->GPIO7Scaler) + "," +
           String(m_ioConfig->GPIO8Config) + "," + String(m_ioConfig->GPIO8Scaler) + "\n";
}

int _lstSend = 0;


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
    m_console->loop();
    m_hal->feedHWWatchdog();

    // if we don't receive any inputs for the pause timeout period assume we should restart.
    if (m_paused && m_pauseTimeout < millis())
    {
        m_paused = false;
        m_console->enableBTOut(true);
        m_pauseTimeout = 0;
    }
    else if (m_paused)
    {
        //   m_console->println("PAUSE: " + String(m_pauseTimeout) + " " + String(millis()));
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

    m_console->printError("kvp=notfound; // could not find key " + String(key) + " in linked list.");

    return NULL;
}

bool NuvIoTState::getBool(String key)
{
    uint8_t tmpBool;

    esp_err_t err = nvs_get_u8(m_nvsHandle, key.c_str(), &tmpBool);

    if (err == ESP_OK)
    {
        return tmpBool != 0;
    }
    else
    {
        String errMsg = resolveError(err);
        Param *pParam = findKey(m_pIntParamHead, key.c_str());
        if (pParam == NULL)
        {
            m_console->printWarning("getbool=failed; // err: not found and not registered, key: " + String(key) + ";");
            return false;
        }

        m_console->printWarning("getbool=failed; // returning default, err: " + errMsg + ", key: " + String(key) + ";");
        return pParam->getBoolDefault();
    }
}

int32_t NuvIoTState::getInt(String key)
{
    int32_t tmpInt;
    esp_err_t err = nvs_get_i32(m_nvsHandle, key.c_str(), &tmpInt);
    if (err == ESP_OK)
    {
        return tmpInt;
    }
    else
    {
        String errMsg = resolveError(err);
        Param *pParam = findKey(m_pIntParamHead, key.c_str());
        if (pParam == NULL)
        {
            m_console->printWarning("getint=failed; // err: not found and not registered, key: " + String(key) + ";");
            return false;
        }

        m_console->printWarning("getint=failed; // returning default, err: " + errMsg + " key: " + String(key));
        return pParam->getIntDefault();
    }
}

float NuvIoTState::getFlt(String key)
{
    int32_t tmpFlt;

    esp_err_t err = nvs_get_i32(m_nvsHandle, key.c_str(), &tmpFlt);

    if (err == ESP_OK)
    {
        return tmpFlt / FLOAT_DECIMAL_SCALER;
    }
    else
    {
        String errMsg = resolveError(err);
        Param *pParam = findKey(m_pIntParamHead, key.c_str());
        if (pParam == NULL)
        {
            m_console->printWarning("getflt=failed," + key + "; // err: not found and not registered, key: " + String(key) + ";");
            return 0;
        }

        m_console->printWarning("getflt=failed," + key + "; // returning default, err: " + errMsg + ", key: " + String(key) + ";");
        return pParam->getFltDefault();
    }
}

void NuvIoTState::setADCConfig(int idx, uint8_t config,  float zero, float scaler, float calibration)
{
    m_console->println("setadc=true; // index=" + String(idx) + ", " + String(config) + ", " + String(zero) + ", " + String(scaler) + ", " + String(calibration) + ";");

    switch (idx)
    {
    case 0:
        m_ioConfig->ADC1Config = config;
        m_ioConfig->ADC1Zero = zero;
        m_ioConfig->ADC1Scaler = scaler;
        m_ioConfig->ADC1Calibration = calibration;
        break;
    case 1:
        m_ioConfig->ADC2Config = config;
        m_ioConfig->ADC2Zero = zero;
        m_ioConfig->ADC2Scaler = scaler;
        m_ioConfig->ADC2Calibration = calibration;
        break;
    case 2:
        m_ioConfig->ADC3Config = config;
        m_ioConfig->ADC3Zero = zero;
        m_ioConfig->ADC3Scaler = scaler;
        m_ioConfig->ADC3Calibration = calibration;
        break;
    case 3:
        m_ioConfig->ADC4Config = config;
        m_ioConfig->ADC4Zero = zero;
        m_ioConfig->ADC4Scaler = scaler;
        m_ioConfig->ADC4Calibration = calibration;
        break;
    case 4:
        m_ioConfig->ADC5Config = config;
        m_ioConfig->ADC5Zero = zero;
        m_ioConfig->ADC5Scaler = scaler;
        m_ioConfig->ADC5Calibration = calibration;
        break;
    case 5:
        m_ioConfig->ADC6Config = config;
        m_ioConfig->ADC6Zero = zero;
        m_ioConfig->ADC6Scaler = scaler;
        m_ioConfig->ADC6Calibration = calibration;
        break;
    case 6:
        m_ioConfig->ADC7Config = config;
        m_ioConfig->ADC7Zero = zero;
        m_ioConfig->ADC7Scaler = scaler;
        m_ioConfig->ADC7Calibration = calibration;
        break;
    case 7:
        m_ioConfig->ADC8Config = config;
        m_ioConfig->ADC8Zero = zero;
        m_ioConfig->ADC8Scaler = scaler;
        m_ioConfig->ADC8Calibration = calibration;
        break;
    }

    m_ioConfig->write();
}

void NuvIoTState::setIOCConfig(int idx, uint8_t config,  float zero, float scaler, float calibration)
{
    m_console->println("setioconfig=true; // index=" + String(idx) + ", " + String(config) + ", " + String(zero) + ", " + String(scaler) + ", " + String(calibration) + ";");

    switch (idx)
    {
    case 0:
        m_ioConfig->GPIO1Config = config;
        m_ioConfig->GPIO1Zero = zero;
        m_ioConfig->GPIO1Scaler = scaler;
        m_ioConfig->GPIO1Calibration = calibration;
        break;
    case 1:
        m_ioConfig->GPIO2Config = config;
        m_ioConfig->GPIO2Zero = zero;
        m_ioConfig->GPIO2Scaler = scaler;
        m_ioConfig->GPIO2Calibration = calibration;
        break;
    case 2:
        m_ioConfig->GPIO3Config = config;
        m_ioConfig->GPIO3Zero = zero;
        m_ioConfig->GPIO3Scaler = scaler;
        m_ioConfig->GPIO3Calibration = calibration;
        break;
    case 3:
        m_ioConfig->GPIO4Config = config;
        m_ioConfig->GPIO4Zero = zero;
        m_ioConfig->GPIO4Scaler = scaler;
        m_ioConfig->GPIO4Calibration = calibration;
        break;
    case 4:
        m_ioConfig->GPIO5Config = config;
        m_ioConfig->GPIO5Zero = zero;
        m_ioConfig->GPIO5Scaler = scaler;
        m_ioConfig->GPIO5Calibration = calibration;
        break;
    case 5:
        m_ioConfig->GPIO6Config = config;
        m_ioConfig->GPIO6Zero = zero;
        m_ioConfig->GPIO6Scaler = scaler;
        m_ioConfig->GPIO6Calibration = calibration;
        break;
    case 6:
        m_ioConfig->GPIO7Config = config;
        m_ioConfig->GPIO7Zero = zero;
        m_ioConfig->GPIO7Scaler = scaler;
        m_ioConfig->GPIO7Calibration = calibration;
        break;
    case 7:
        m_ioConfig->GPIO8Config = config;
        m_ioConfig->GPIO8Zero = zero;
        m_ioConfig->GPIO8Scaler = scaler;
        m_ioConfig->GPIO8Calibration = calibration;
        break;
    }

    m_ioConfig->write();
}

void NuvIoTState::persistConfig()
{
    m_ioConfig->write();
}

void NuvIoTState::handleConsoleCommand(String msg)
{
    m_console->println("HANDLING MESSAGE [" + msg + "]");

    if (msg == "HELLO")
    {
        m_display->clearBuffer();
        m_display->println("Welcome");
        m_display->println("Configuration Mode");
        m_display->sendBuffer();
        m_configurationMode = true;
        m_paused = true;
    }
    else if (msg == "PAUSE")
    {
        m_ledManager->setOnlineFlashRate(8);
        m_pauseTimeout = millis() + (60 * 1000);
        m_paused = true;
    }
    else if (msg == "CONTINUE")
    {
        m_paused = false;
        m_configurationMode = false;

        m_display->clearBuffer();
        m_display->println("Completed");
        m_display->println("Leaving Config Mode");
        m_display->sendBuffer();
        m_console->enableBTOut(true);
    }
    else if (msg == "REBOOT")
    {
        m_hal->restart();
    }
    else if (msg == "PROPERTIES")
    {
        m_console->println(getRemoteProperties());
    }
    else if (msg == "VERSION")
    {
        m_console->println(queryFirmwareVersion());
    }
    else if (msg.substring(0, 3) == "SET")
    {
        String setCommand = msg.substring(4);
        int dashIdx = setCommand.indexOf('-');
        int equalsIdx = setCommand.indexOf("=");
        String type = setCommand.substring(0, dashIdx);
        String key = setCommand.substring(dashIdx + 1, equalsIdx);
        String value = setCommand.substring(equalsIdx + 1);
        updateProperty(type, key, value);
        m_console->println("set-ack:" + key);
    }
    else if (msg.substring(0, 5) == "BAUD=")
    {
        String strBaudRate = msg.substring(5);
        unsigned long baud = atol(strBaudRate.c_str());
        if (baud > 0)
        {
            m_sysConfig->GPRSModemBaudRate = baud;
            m_sysConfig->write();
            m_console->println("new baud rate:" + String(m_sysConfig->GPRSModemBaudRate));
            m_console->println("rebooting in 2 seconds.");
            delay(2000);
            m_hal->restart();
        }
    }
    else if (msg.substring(0, 3) == "ID=")
    {
        String deviceId = msg.substring(3);
        if (deviceId.length() > 0)
        {
            m_sysConfig->DeviceId = deviceId;
            m_sysConfig->write();

            m_console->println("new device id:" + m_sysConfig->DeviceId);
            m_console->println("rebooting in 2 seconds.");
            delay(2000);
            m_hal->restart();
        }
    }
    else if (msg == "IOCONFIG-SEND")
    {
        String json = m_ioConfig->toJSON();
        uint16_t remaining = json.length();
        uint16_t chunkSize = 100;
        uint16_t chunkIndex = 0;
        while (remaining > 0)
        {
            int start = chunkIndex * 100;
            int end = min((uint16_t)(start + chunkSize), (uint16_t)json.length());
            remaining = json.length() - end;
            m_console->print(json.substring(chunkIndex * 100, end));
            delay(100);
            chunkIndex++;
        }

        m_console->println("");
    }
    else if (msg == "SYSCONFIG-SEND")
    {
        String json = m_sysConfig->toJSON();
        uint16_t remaining = json.length();
        uint16_t chunkSize = 100;
        uint16_t chunkIndex = 0;
        while (remaining > 0)
        {
            int start = chunkIndex * 100;
            int end = min((uint16_t)(start + chunkSize), (uint16_t)json.length());
            remaining = json.length() - end;
            m_console->print(json.substring(chunkIndex * 100, end));
            delay(100);
            chunkIndex++;
        }

        m_console->println("");
    }
    else
    {
        m_console->println("UNKNOWN COMMAND: " + msg);
    }
}

void NuvIoTState::updateProperty(String fieldType, String field, String value)
{
    m_console->println("-- " + fieldType + " - " + field + " - " + value);

    if (fieldType == "Integer" || fieldType == "State")
    {
        int32_t intValue = atol(value.c_str());
        if (field == "updaterate")
        {
            m_sysConfig->SendUpdateRateMS = intValue;
            m_sysConfig->write();
            m_console->println("setint=success," + field + ";");
        }
        else if (field == "looprate")
        {
            m_sysConfig->LoopUpdateRateMS = intValue;
            m_sysConfig->write();
            m_console->println("setint=success," + field + ";");
        }
        else if (field == "gps")
        {
            m_sysConfig->GPSUpdateRateMS = intValue;
            m_sysConfig->write();
            m_console->println("setint=success," + field + ";");
        }
        else if (field == "pingrate")
        {
            m_sysConfig->PingRateSecond = intValue;
            m_sysConfig->write();
            m_console->println("setint=success," + field + ";");
        }
        else
        {
            Param *pParam = findKey(m_pIntParamHead, field.c_str());
            if (pParam != NULL)
            {

                esp_err_t err = nvs_set_i32(m_nvsHandle, field.c_str(), intValue);
                if (err == ESP_OK)
                {
                    err = nvs_commit(m_nvsHandle);
                    if (err == ESP_OK)
                    {
                        m_console->println("setint=success," + field + ";");
                    }
                    else
                    {
                        String errMsg = resolveError(err);
                        m_console->printError("setint=failed,commit" + field + "; // error: " + errMsg);
                    }
                }
                else
                {
                    String errMsg = resolveError(err);
                    m_console->printError("setint=failed,write," + field + "; // error: " + errMsg);
                }
            }
            else
            {
                m_console->printError("setint=failed,find," + field + "; // error: could not find field.");
            }
        }
    }
    else if (fieldType == "Decimal" || fieldType == "UnitSet")
    {
        Param *pParam = findKey(m_pFloatParamHead, field.c_str());
        if (pParam != NULL)
        {
            float floatValue = atof(value.c_str()) * FLOAT_DECIMAL_SCALER;
            esp_err_t err = nvs_set_i32(m_nvsHandle, field.c_str(), floatValue);
            if (err == ESP_OK)
            {
                err = nvs_commit(m_nvsHandle);
                if (err == ESP_OK)
                {
                    m_console->println("setdecimal=success," + field + ";");
                }
                else
                {
                    String errMsg = resolveError(err);
                    m_console->printError("setdecimal=failed,commit," + field + "; // error: " + errMsg);
                }
            }
            else
            {
                String errMsg = resolveError(err);
                m_console->printError("setdecimal=failed,write," + field + "; // error: " + errMsg);
            }
        }
        else
        {
            m_console->printError("setdecimal=failed,find" + field + "; // error: could not find field.");
        }
    }
    else if (fieldType == "TrueFalse")
    {
        if(field == "gps") {
            m_sysConfig->GPSEnabled = value == "true" || value == "True";
            m_sysConfig->write();
            m_console->println("setbool=success," + field + ";");
        }
        else {
            Param *pParam = findKey(m_pBoolParamHead, field.c_str());
            if (pParam != NULL)
            {
                uint8_t boolValue = value == "true" || value == "True" ? 255 : 0;
                esp_err_t err = nvs_set_u8(m_nvsHandle, field.c_str(), boolValue);
                if (err == ESP_OK)
                {
                    err = nvs_commit(m_nvsHandle);
                    if (err == ESP_OK)
                    {
                        m_console->println("setbool=success," + field + ";");
                    }
                    else
                    {
                        String errMsg = resolveError(err);
                        m_console->printError("setbool=failed,commit," + field + "; // error: " + errMsg);
                    }
                }
                else
                {
                    String errMsg = resolveError(err);
                    m_console->printError("setbool=failed,write," + field + "; // error: " + errMsg);
                }
            }
            else
            {
                m_console->printError("setbool=failed,find" + field + "; // error: could not find field.");
            }
        }
    }
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

    return NULL;
}

String NuvIoTState::resolveError(esp_err_t err)
{
    switch (err)
    {
    case ESP_ERR_NVS_NOT_FOUND:
        return "keynotfound";
    case ESP_ERR_NVS_INVALID_HANDLE:
        return "invalidhandle";
    case ESP_ERR_NVS_INVALID_NAME:
        return "invalidname";
    case ESP_ERR_NVS_INVALID_LENGTH:
        return "invalidlength";
    default:
        return "unknown-" + String(err);
    }
}

void NuvIoTState::registerInt(const char *keyName, int32_t defaultValue)
{
    Param *p = new Param(keyName, defaultValue);

    if (m_pIntParamHead == NULL)
    {
        p->setIndex(0);
        m_pIntParamHead = p;
    }
    else
    {
        appendValue(m_pIntParamHead, p);
    }

    int32_t tmpValue;
    esp_err_t err = nvs_get_i32(m_nvsHandle, keyName, &tmpValue);
    if (err == ESP_OK)
    {
        m_console->println("keyreg=existing,int," + String(keyName) + "; // existing value: " + String(tmpValue));
    }
    else if (err == ESP_ERR_NVS_NOT_FOUND)
    {
        err = nvs_set_i32(m_nvsHandle, keyName, (int32_t)(defaultValue));
        if (err == ESP_OK)
        {
            err = nvs_commit(m_nvsHandle);
            if (err == ESP_OK)
            {
                m_console->println("keyreg=added,int," + String(keyName) + "; // default value: " + String(defaultValue));
            }
            else
            {
                m_console->repeatFatalError("keyreg=failed,commit,int," + String(keyName) + "; // " + resolveError(err));
            }
        }
        else
        {
            m_console->repeatFatalError("keyreg=failed,write,int," + String(keyName) + "; // " + resolveError(err));
        }
    }
    else
    {
        m_console->repeatFatalError("addkey=>" + String(keyName) + "; // " + resolveError(err));
    }
}

void NuvIoTState::registerFloat(const char *keyName, float defaultValue)
{
    Param *p = new Param(keyName, defaultValue);

    if (m_pFloatParamHead == NULL)
    {
        p->setIndex(0);
        m_pFloatParamHead = p;
    }
    else
    {
        appendValue(m_pFloatParamHead, p);
    }

    int32_t tmpValue;
    esp_err_t err = nvs_get_i32(m_nvsHandle, keyName, &tmpValue);
    if (err == ESP_OK)
    {
        m_console->println("keyreg=existing,decimal," + String(keyName) + "; // existing value: " + String(tmpValue / FLOAT_DECIMAL_SCALER));
    }
    else if (err == ESP_ERR_NVS_NOT_FOUND)
    {
        err = nvs_set_i32(m_nvsHandle, keyName, (int32_t)(defaultValue * FLOAT_DECIMAL_SCALER));
        if (err == ESP_OK)
        {
            err = nvs_commit(m_nvsHandle);
            if (err == ESP_OK)
            {
                m_console->println("keyreg=added,decimal," + String(keyName) + "; // default value: " + String(defaultValue));
            }
            else
            {
                m_console->repeatFatalError("keyreg=failed,commit,decimal," + String(keyName) + "; // " + resolveError(err));
            }
        }
        else
        {
            m_console->repeatFatalError("keyreg=failed,write,decimal," + String(keyName) + "; // " + resolveError(err));
        }
    }
    else
    {
        m_console->repeatFatalError("keyreg=failed,read,decimal," + String(keyName) + "; // " + resolveError(err));
    }
}

void NuvIoTState::registerBool(const char *keyName, boolean defaultValue)
{
    Param *p = new Param(keyName, defaultValue);

    if (m_pBoolParamHead == NULL)
    {
        p->setIndex(0);
        m_pBoolParamHead = p;
    }
    else
    {
        appendValue(m_pBoolParamHead, p);
    }

    uint8_t tmpValue;
    esp_err_t err = nvs_get_u8(m_nvsHandle, keyName, &tmpValue);
    if (err == ESP_OK)
    {
        // key exists and we read it.
        m_console->println("keyreg=existing,bool," + String(keyName) + "; // existing value: " + String(tmpValue) + ", 255 = true 0 = false");
    }
    else if (err == ESP_ERR_NVS_NOT_FOUND)
    {
        err = nvs_set_u8(m_nvsHandle, keyName, defaultValue);
        if (err == ESP_OK)
        {
            err = nvs_commit(m_nvsHandle);
            if (err == ESP_OK)
            {
                m_console->println("keyreg=added,bool," + String(keyName) + "; // default value: " + String(tmpValue) + ", 255 = true 0 = false");
            }
            else
            {
                m_console->repeatFatalError("keyreg=failed,commit,bool," + String(keyName) + "; // " + resolveError(err));
            }
        }
        else
        {
            m_console->repeatFatalError("keyreg=failed,write,bool," + String(keyName) + "; // " + resolveError(err));
        }
    }
    else
    {
        m_console->repeatFatalError("keyreg=failed,read,bool," + String(keyName) + "; // " + resolveError(err));
    }
}

String NuvIoTState::getHardwareRevision() { return m_hardwareRevision; }
String NuvIoTState::getFirmwareVersion() { return m_firmwareVersion; }
String NuvIoTState::getFirmwareSKU() { return m_firmwareSku; }
String NuvIoTState::getDeviceModelKey() {return m_deviceModelKey; }