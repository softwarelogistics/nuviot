#ifndef SYCCONFIG_H
#define SYSCONFIG_H

#include <Arduino.h>
#include <ArduinoJson.h>

#include <SPIFFS.h>

#define SYSCONFIG_FN "/sysconfig.json"
#define FILE_READ "r"
#define FILE_WRITE "w"

class SysConfig
{
public:
    bool Commissioned;

    String DeviceId;

    String WiFiSSID;
    String WiFiPWD;

    String SrvrHostName;
    bool Anonymous;
    String SrvrUID;
    String SrvrPWD;

    bool GPSEnabled;

    int PingRate;
    int SendUpdateRate;
    int GPSUpdateRate;
    long GPRSModemBaudRate;

    String toJSON()
    {
        const size_t capacity = JSON_OBJECT_SIZE(1024);
        DynamicJsonDocument doc(capacity);
    }

    void load()
    {
        File file = SPIFFS.open(SYSCONFIG_FN, FILE_READ);
        if (file)
        {
            String json = file.readString();

            Serial.println("File " + String(SYSCONFIG_FN) + " exists, file size: " + String(json.length()));

            if (json.length() == 0)
            {
                file.close();
                setDefaults();
                write();
            }
            {
                parseJSON(json);
                file.close();
            }
        }
        else
        {
            setDefaults();
            write();
            Serial.println("FILE DIDN'T EXIST WRITE DEFAULTS.");
        }
    }

    void write()
    {
        File file = SPIFFS.open(SYSCONFIG_FN, FILE_WRITE);
        if (!file)
        {
            Serial.println("Could not open " + String(SYSCONFIG_FN) + " file to write.");
        }

        size_t bytesWritten = file.print(toJSON());
        file.flush();
        file.close();

        Serial.println("Write " + String((int)bytesWritten) + " to " + String(SYSCONFIG_FN));
    }

    void setDefaults()
    {
        GPRSModemBaudRate = 115200;
        Commissioned = false;
        DeviceId = "?";
        WiFiSSID = "";
        WiFiPWD = "";
        SrvrHostName = "?";
        Anonymous = false;
        SrvrUID = "";
        SrvrPWD = "";

        PingRate = 120;
        SendUpdateRate = 120;
        GPSUpdateRate = 5;
    }

    void parseJSON(String json)
    {

    }
};

#endif