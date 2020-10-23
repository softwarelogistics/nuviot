#include "NuvIoTState.h"
#include "esp_bt_device.h"
#include "math.h"
#include <Update.h>

#include <nvs.h>

#define FLOAT_DECIMAL_SCALER 1000.0f

NuvIoTState::NuvIoTState(Display *display, IOConfig *config, SysConfig *sysConfig, LedManager *ledManager, BluetoothSerial *btSerial, FS *fs, Hal *hal, Console *console)
{
    m_display = display;
    m_hal = hal;
    m_console = console;
    m_btSerial = btSerial;
    m_ioConfig = config;
    m_sysConfig = sysConfig;
    m_ledManager = ledManager;
}

void NuvIoTState::init(String firmwareSku, String firmwareVersion, String deviceConfigKey, uint16_t structureVersion)
{
    m_firmwareSku = firmwareSku;
    m_firmwareVersion = firmwareVersion;

    String btSerialName = "NuvIoT - " + (m_sysConfig->DeviceId == "?" ? firmwareSku : m_sysConfig->DeviceId);
    m_btSerial->begin(btSerialName); //Name of your Bluetooth Signal

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

    if (!EEPROM.begin(2048))
    {
        m_display->drawStr("Could not initialize", "EEPROM");
        m_console->repeatFatalError("Could not init EEPROM storage;");
    }
    else
    {
        m_console->printVerbose("eeprom=initialized;");
    }
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
    String state =
        "readonly-firmwareSku=" + m_firmwareSku + "," +
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

void NuvIoTState::readFirmware()
{
    long timeout;

    m_console->enableBTOut(false);
    m_ledManager->setOnlineFlashRate(2);
    m_console->println("Start DFU");
    m_btSerial->print("fwupdate=ok-start;\n");

    timeout = millis() + 10000;
    while (m_btSerial->available() < 4)
    {
        if (millis() > timeout)
        {
            m_ledManager->setErrFlashRate(4);
            m_btSerial->println("fwupdate=failed; // tiemout waiting for update size.\n");
            m_console->printError("fwupdate=failed; // tiemout waiting for update size.");
            m_display->drawStr("Flashing failed", "total size timeout.");
            delay(2000);
            m_hal->restart();
        }
        delay(1);
    }

    char progressBar[110];

    int32_t total = m_btSerial->read() << 24 | m_btSerial->read() << 16 | m_btSerial->read() << 8 | m_btSerial->read();

    m_console->println("Expecting [" + String(total) + "] bytes");
    m_btSerial->print("fwupdate=ok-size=" + String(total) + ";\n");

    timeout = millis() + 10000;

    while (m_btSerial->available() < 2)
    {
        if (millis() > timeout)
        {
            m_ledManager->setErrFlashRate(4);
            m_btSerial->println("fwupdate=failed; // timeout waiting for block count\n.");
            m_console->printError("fwupdate=failed; // timeout waiting for block count.");
            m_display->drawStr("Flashing failed", "total block count timeout.");
            delay(5000);
            m_hal->restart();
        }
        delay(1);
    }

    short blockCount = m_btSerial->read() << 8 | m_btSerial->read();
    byte buffer[512];

    m_console->println("Started reading [" + String(blockCount) + "] blocks");
    m_btSerial->print("fwupdate=ok-blocks=" + String(blockCount) + ";\n");

    int blocksReceived = 0;

    if (!Update.begin(total, U_FLASH))
    {
        m_ledManager->setErrFlashRate(4);
        m_btSerial->print("fwupdate=failed; // could not start DFU, " + String(Update.errorString()) + ".\n");
        m_console->printError("fwupdate=failed; // could not start DFU, " + String(Update.errorString()) + ".");
        m_display->drawStr("Flashing failed", "Update.begin() failed.");
        delay(5000);
        m_hal->restart();
    }

    m_console->println("Begining Download for DFU");

    for (int idx = 0; idx < blockCount; ++idx)
    {
        m_ledManager->setOnlineFlashRate(1);

        timeout = millis() + 10000;
        while (m_btSerial->available() < 2)
        {
            if (millis() > timeout)
            {
                m_btSerial->println("fwupdate=failed; // timeout waiting for 2 bytes for block size.\n");
                m_console->printError("fwupdate=failed; // timeout waiting for 2 bytes for block size.");
                m_display->drawStr("Flashing failed", "Block size read timeout.");
                m_ledManager->setErrFlashRate(4);
                delay(5000);
                m_hal->restart();
            }

            delay(1);
        }

        short blockSize = m_btSerial->read() << 8 | m_btSerial->read();
        m_console->println("Expecting block size of " + String(blockSize));

        timeout = millis() + 10000;

        while (m_btSerial->available() < blockSize + 1)
        {
            if (millis() - _lstSend > 1000)
            {
                _lstSend = millis();
                m_console->println("Bytes in hopper [" + String(m_btSerial->available()) + "].");
            }

            if (millis() > timeout)
            {
                m_ledManager->setErrFlashRate(4);
                m_btSerial->println("fwupdate=failed;// timeout on read " + String(m_btSerial->available()) + " of " + String(blockSize) + "\n");
                m_console->printError("fwupdate=failed; // timeout on read " + String(m_btSerial->available()) + " of " + String(blockSize));
                m_display->drawStr("Flashing failed", "Block read timeout.");
                delay(5000);
                m_hal->restart();
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
        m_console->println("fwupdate=read; // block (" + String(idx) + "/" + String(blockCount) + ") size: " + String(blockSize) + " calc cs:" + calcCheckSum + " act cs" + actualCheckSum + ".");

        if (actualCheckSum != calcCheckSum)
        {
            m_ledManager->setErrFlashRate(4);
            m_btSerial->print("fwupdate=failed; // checksum error, block= " + String(idx) + "," + String(actualCheckSum) + "/" + String(calcCheckSum) + ".\n");
            m_console->printError("fwupdate=failed; // checksum error, block=" + String(idx) + "," + String(actualCheckSum) + "/" + String(calcCheckSum) + ".");
            m_display->drawStr("Flashing failed", ("Block " + String(idx) + " check sum error.").c_str());
            delay(5000);
            m_hal->restart();
        }
        else
        {
            m_btSerial->print("fwupdate=ok-read=" + String(idx) + ";\n");
        }

        blocksReceived++;

        size_t written = Update.write(buffer, blockSize);
        if (written == blockSize)
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
            m_ledManager->setErrFlashRate(4);
            m_btSerial->print("fwupdate=failed; // dfu-write:blk-" + String(idx) + "," + String(Update.errorString()) + "\n");
            m_console->printError("fwupdate=failed; // dfu-write:blk-" + String(idx) + "," + String(Update.errorString()) + ".");
            m_display->drawStr("Flashing failed", "block write error", Update.errorString());
            delay(5000);
            m_hal->restart();
        }

        delay(5);
    }

    m_btSerial->print("ok-recv:all\n");

    if (blocksReceived != blockCount)
    {
        m_ledManager->setErrFlashRate(4);
        m_btSerial->print("fwupdate=failed; // block count mis-match: " + String(blocksReceived) + " expected:" + String(blockCount) + "\n");
        m_console->printError("fwupdate=failed; // block count mis-match: " + String(blocksReceived) + " expected:" + String(blockCount) + ".");
        m_display->drawStr("Flashing Failed", "final block count mismatch");
        delay(5000);
        m_hal->restart();
    }

    if (!Update.isFinished())
    {
        m_ledManager->setErrFlashRate(4);
        m_btSerial->print("fwupdate=failed; // Update.isFinished() failed: " + String(Update.errorString()) + "\n");
        m_console->println("fwupdate=failed; // Update.isFinished() failed: " + String(Update.errorString()) + ".");
        m_display->drawStr("Flashing Failed", "Update IsFinished Failed.");
        delay(5000);
        m_hal->restart();
    }

    if (!Update.end())
    {

        m_btSerial->print("fwupdate=failed; Update.end() failed: " + String(Update.errorString()) + "\n");
        m_console->printError("fwupdate=failed; Update.end() failed: " + String(Update.errorString()) + ".");
        m_display->drawStr("Flashing Failed", Update.errorString());
        delay(5000);
        m_hal->restart();
    }

    m_display->drawStr("Success flashing", "Restarting");
    m_btSerial->print("fw-update:completed; rebooting.\n");
    m_console->println("fw-update:completed; rebooting.");
    delay(2000);
    m_hal->restart();
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
    // if we don't receive any inputs for the pause timeout period assume we should restart.
    if (m_paused && m_pauseTimeout < millis())
    {
        m_paused = false;
        m_btSerial->println("DISCONNECTED");
        m_pauseTimeout = 0;
    }
    else if (m_paused)
    {
        m_console->println("PAUSE: " + String(m_pauseTimeout) + " " + String(millis()));
    }

    while (m_btSerial->available() > 0)
    {
        if (m_paused)
        {
            m_pauseTimeout = millis() + (30 * 1000);
        }

        int ch = m_btSerial->read();

        if (ch == '\n')
        {
            m_messageBuffer[m_messageBufferTail++] = 0x00;
            String msg = String(m_messageBuffer);
            Serial.println("RECEIVE LINE: " + msg);

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
                m_ledManager->setOnlineFlashRate(8);
                m_pauseTimeout = millis() + (60 * 1000);
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
            else if (msg == "VERSION")
            {
                m_btSerial->println(queryFirmwareVersion());
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
                    m_btSerial->print(json.substring(chunkIndex * 100, end));
                    m_btSerial->flush();
                    delay(100);
                    chunkIndex++;
                }

                m_btSerial->println();
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
                    m_btSerial->print(json.substring(chunkIndex * 100, end));
                    m_btSerial->flush();
                    delay(100);
                    chunkIndex++;
                }

                m_btSerial->println();
            }
            else if (msg.substring(0) == "SYSCONFIG-RECV-START")
            {
                m_jsonBufferTail = 0x00;
            }
            else if (msg.substring(0) == "SYSCONFIG-RECV-END")
            {
                m_jsonBuffer[m_jsonBufferTail] = 0x00;
                Serial.println(m_jsonBuffer);
                m_jsonBufferTail = 0;

                if (m_sysConfig->parseJSON(m_jsonBuffer))
                {
                    m_sysConfig->write();
                    m_btSerial->println("SYSCONFIG-RECV-END:OK");
                }
                else
                {
                    m_btSerial->println("SYSCONFIG-RECV-END:FAIL");
                }
            }
            else if (msg.substring(0, 14) == "SYSCONFIG-RECV")
            {
                // format is
                // ICONFIG-RECV:XX,CRC,[CONTENTS]
                Serial.println("----------------------");
                char hexBuffer[3];
                msg.toCharArray(hexBuffer, 3, 15);
                Serial.println(hexBuffer);
                uint8_t rowIndex = strtol(hexBuffer, NULL, 16);

                msg.toCharArray(hexBuffer, 3, 18);
                uint8_t crc = strtol(hexBuffer, NULL, 16);
                Serial.println(hexBuffer);
                uint8_t calcCRC = 0x00;

                for (int idx = 21; idx < msg.length(); ++idx)
                {
                    calcCRC += (uint8_t)msg[idx];
                }

                Serial.println("RESULT: " + String(rowIndex) + " " + String(crc) + " " + String(calcCRC));
                Serial.println("----------------------");

                if (crc == calcCRC)
                {
                    m_btSerial->println("SYSCONFIG-RECV-OK:" + String(rowIndex));

                    if (rowIndex == 0)
                    {
                        m_jsonBufferTail = 0;
                    }

                    for (int idx = 21; idx < msg.length(); ++idx)
                    {
                        m_jsonBuffer[m_jsonBufferTail++] = msg[idx];
                    }
                }
                else
                {
                    m_btSerial->println("SYSCONFIG-RECV-CRC-ERR:" + String(rowIndex));
                }
            }
            else if (msg.substring(0) == "IOCONFIG-RECV-START")
            {
                m_jsonBufferTail = 0;
            }
            else if (msg.substring(0) == "IOCONFIG-RECV-END")
            {
                m_jsonBuffer[m_jsonBufferTail] = 0x00;
                Serial.println(m_jsonBuffer);
                m_jsonBufferTail = 0;

                if (m_ioConfig->parseJSON(m_jsonBuffer))
                {
                    m_ioConfig->write();
                    m_btSerial->println("IOCONFIG-RECV-END:OK");
                }
                else
                {
                    m_btSerial->println("IOCONFIG-RECV-END:FAIL");
                }
            }
            else if (msg.substring(0, 13) == "IOCONFIG-RECV")
            {
                // format is
                // ICONFIG-RECV:XX,CRC,[CONTENTS]
                Serial.println("----------------------");
                char hexBuffer[3];
                msg.toCharArray(hexBuffer, 3, 14);
                Serial.println(hexBuffer);
                uint8_t rowIndex = strtol(hexBuffer, NULL, 16);

                msg.toCharArray(hexBuffer, 3, 17);
                uint8_t crc = strtol(hexBuffer, NULL, 16);
                Serial.println(hexBuffer);
                uint8_t calcCRC = 0x00;

                for (int idx = 20; idx < msg.length(); ++idx)
                {
                    calcCRC += (uint8_t)msg[idx];
                }

                Serial.println("RESULT: " + String(rowIndex) + " " + String(crc) + " " + String(calcCRC));
                Serial.println("----------------------");

                if (crc == calcCRC)
                {
                    m_btSerial->println("IOCONFIG-RECV-OK:" + String(rowIndex));

                    if (rowIndex == 0)
                    {
                        m_jsonBufferTail = 0;
                    }

                    for (int idx = 20; idx < msg.length(); ++idx)
                    {
                        m_jsonBuffer[m_jsonBufferTail++] = msg[idx];
                    }
                }
                else
                {
                    m_btSerial->println("IOCONFIG-RECV-CRC-ERR:" + String(rowIndex));
                }
            }
            else if (msg == "RESET-STATE")
            {
                m_ioConfig->setDefaults();
                m_ioConfig->write();
                m_hal->restart();
            }
            else if (msg.substring(0, 3) == "SET")
            {
                String setCommand = String(&m_messageBuffer[4]);
                int dashIdx = setCommand.indexOf('-');
                int equalsIdx = setCommand.indexOf("=");
                String type = setCommand.substring(0, dashIdx);
                String key = setCommand.substring(dashIdx + 1, equalsIdx);
                String value = setCommand.substring(equalsIdx + 1);
                updateProperty(type, key, value);
                m_btSerial->println("set-ack:" + key);
            }
            else if (msg == "QUIT")
            {
                m_configurationMode = false;
            }
            else if (msg == "COMMISSION")
            {
                m_sysConfig->Commissioned = true;
                m_sysConfig->write();
                m_btSerial->println("999,ACK\n");
                m_hal->restart();
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
            m_console->printWarning("getbool=failed; // err: not found and not registerd, key: " + String(key) + ";");
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
            m_console->printWarning("getint=failed; // err: not found and not registerd, key: " + String(key) + ";");
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
            m_console->printWarning("getflt=failed," + key + "; // err: not found and not registerd, key: " + String(key) + ";");
            return 0;
        }

        m_console->printWarning("getflt=failed," + key + "; // returning default, err: " + errMsg + ", key: " + String(key) + ";");
        return pParam->getFltDefault();
    }
}

void NuvIoTState::setADCConfig(int idx, uint8_t config, float scaler)
{
    m_console->println("setadc=true; // index=" + String(idx) + ", " + String(config) + ", " + String(scaler)) ;

    switch(idx)
    {
        case 0: m_ioConfig->ADC1Config = config; m_ioConfig->ADC1Scaler = scaler; break;
        case 1: m_ioConfig->ADC2Config = config; m_ioConfig->ADC2Scaler = scaler; break;
        case 2: m_ioConfig->ADC3Config = config; m_ioConfig->ADC3Scaler = scaler; break;
        case 3: m_ioConfig->ADC4Config = config; m_ioConfig->ADC4Scaler = scaler; break;
        case 4: m_ioConfig->ADC5Config = config; m_ioConfig->ADC5Scaler = scaler; break;
        case 5: m_ioConfig->ADC6Config = config; m_ioConfig->ADC6Scaler = scaler; break;
        case 6: m_ioConfig->ADC7Config = config; m_ioConfig->ADC7Scaler = scaler; break;
        case 7: m_ioConfig->ADC8Config = config; m_ioConfig->ADC8Scaler = scaler; break;
    }
}

void NuvIoTState::setIOCConfig(int idx, uint8_t config, float scaler)
{
    m_console->println("setioconfig=true; // index=" + String(idx) + ", " + String(config) + ", " + String(scaler)) ;

    switch(idx)
    {
        case 0: m_ioConfig->GPIO1Config = config; m_ioConfig->GPIO1Scaler = scaler; break;
        case 1: m_ioConfig->GPIO2Config = config; m_ioConfig->GPIO2Scaler = scaler; break;        
        case 2: m_ioConfig->GPIO3Config = config; m_ioConfig->GPIO3Scaler = scaler; break;
        case 3: m_ioConfig->GPIO4Config = config; m_ioConfig->GPIO4Scaler = scaler; break;        
        case 4: m_ioConfig->GPIO5Config = config; m_ioConfig->GPIO5Scaler = scaler; break;
        case 5: m_ioConfig->GPIO6Config = config; m_ioConfig->GPIO6Scaler = scaler; break;        
        case 6: m_ioConfig->GPIO7Config = config; m_ioConfig->GPIO7Scaler = scaler; break;
        case 7: m_ioConfig->GPIO8Config = config; m_ioConfig->GPIO8Scaler = scaler; break;        
    }
}

void NuvIoTState::persistConfig()
{
    m_ioConfig->write();
}

void NuvIoTState::updateProperty(String fieldType, String field, String value)
{
    if (fieldType == "Integer")
    {
        Param *pParam = findKey(m_pIntParamHead, field.c_str());
        if (pParam != NULL)
        {
            int32_t intValue = atol(value.c_str());
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
    else if (fieldType == "Decimal")
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

String NuvIoTState::getFirmwareVersion() { return m_firmwareVersion; }
String NuvIoTState::getFirmwareSKU() { return m_firmwareSku; }