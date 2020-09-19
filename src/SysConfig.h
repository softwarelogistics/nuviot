#ifndef SYSCONFIG_H
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
    bool CellEnabled;
    bool WiFiEnabled;
    bool VerboseLogging;

    uint32_t PingRate;
    uint32_t SendUpdateRate;
    uint32_t GPSUpdateRate;
    uint32_t GPRSModemBaudRate;

    String toJSON()
    {
        const size_t capacity = JSON_OBJECT_SIZE(40);
        DynamicJsonDocument doc(capacity);
        doc["baud"] = GPRSModemBaudRate;
        doc["commissioned"] = Commissioned;
        doc["deviceId"] = DeviceId;
        doc["wifissid"] = WiFiSSID;
        doc["wifipwd"] = WiFiPWD;
        doc["srvrHostName"] = SrvrHostName;
        doc["anonymous"] = Anonymous;
        doc["srvrUid"] = SrvrUID;
        doc["srvrPwd"] = SrvrPWD;
        doc["pingRate"] = PingRate;
        doc["verboseLogging"] = VerboseLogging;
        doc["sendUpdateRate"] = SendUpdateRate;
        doc["gpsUpdateRate"] = GPSUpdateRate;
        doc["gpsEnabled"] = GPSEnabled;
        doc["wifiEnabled"] = WiFiEnabled;
        doc["cellEnabled"] = CellEnabled;
        String output;
        serializeJson(doc, output);

        return output;
    }

    void load()
    {
        File file = SPIFFS.open(SYSCONFIG_FN, FILE_READ);
        if (file)
        {
            String json = file.readString();
            file.close();
            
            if (json.length() == 0)
            {
                file.close();
                setDefaults();
          
                write();
            }
            else
            {
                if(!parseJSON(json)) {
                    setDefaults();
                    write();
                }
            }
        }
        else
        {
            setDefaults();
            write();
        }
    }

    void write()
    {
        File file = SPIFFS.open(SYSCONFIG_FN, FILE_WRITE);

        file.print(toJSON());
        file.flush();
        file.close();
    }

    void setDefaults()
    {
        GPRSModemBaudRate = 115200;
        WiFiEnabled = false;
        CellEnabled = false;
        Commissioned = false;
        VerboseLogging = false;
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
        GPSEnabled = true;
    }

    bool parseJSON(String json)
    {
        return parseJSON(json.c_str());
    }

    bool parseJSON(const char *str)
    {
        const size_t capacity = JSON_OBJECT_SIZE(40);
        DynamicJsonDocument doc(capacity);

        doc.clear();

        ArduinoJson::DeserializationError err = deserializeJson(doc, str);

        if(err == ArduinoJson::DeserializationError::Ok) {
            Commissioned = doc["commissioned"].as<bool>();
            CellEnabled = doc["cellEnabled"].as<bool>();
            WiFiEnabled = doc["wifiEnabled"].as<bool>();
            VerboseLogging = doc["verboseLogging"].as<bool>();
            DeviceId = doc["deviceId"].as<String>();
            WiFiSSID = doc["wifissid"].as<String>();
            WiFiPWD = doc["wifipwd"].as<String>();
            SrvrHostName = doc["srvrHostName"].as<String>();
            Anonymous = doc["anonymous"].as<bool>();
            SrvrUID = doc["srvrUid"].as<String>();
            SrvrPWD = doc["srvrPwd"].as<String>();
            PingRate = doc["pingRate"].as<uint32_t>();
            GPSEnabled = doc["gpsEnabled"].as<bool>();
            SendUpdateRate = doc["sendUpdateRate"].as<uint32_t>();
            GPSUpdateRate = doc["gpsUpdateRate"].as<uint32_t>();
            GPRSModemBaudRate = doc["baud"].as<uint32_t>();       
            return true;
        }
        else {
            // TODO: Add back in error handling.
            //Serial.println("ERROR SYSCONFIG JSON");
            //Serial.println(str);
            return false;
        }
    }
};

#endif