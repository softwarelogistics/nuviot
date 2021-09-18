#ifndef SYSCONFIG_H
#define SYSCONFIG_H

#include <Arduino.h>
#include <ArduinoJson.h>

#include <SPIFFS.h>
#include <Console.h>

#define SYSCONFIG_FN "/sysconfig.json"
#define FILE_READ "r"
#define FILE_WRITE "w"

class SysConfig
{
private:
    Console *m_pConsole;

public:
    SysConfig(Console *pConsole)
    {
        m_pConsole = pConsole;
    }

    bool Commissioned;

    String DeviceId;

    String WiFiSSID;
    String WiFiPWD;

    String SrvrHostName;
    bool TLS;
    bool Anonymous;
    String SrvrUID;
    String SrvrType;
    String SrvrPWD;

    String DeviceAccessKey;

    bool GPSEnabled;
    bool CellEnabled;
    bool WiFiEnabled;
    bool VerboseLogging;

    uint32_t PingRate;
    uint32_t SendUpdateRate;
    uint32_t GPSUpdateRate;
    unsigned long GPRSModemBaudRate;

    String toJSON()
    {
        const size_t capacity = JSON_OBJECT_SIZE(40);
        DynamicJsonDocument doc(capacity);
        doc["baud"] = GPRSModemBaudRate;
        doc["tls"] = TLS;
        doc["commissioned"] = Commissioned;
        doc["deviceId"] = DeviceId;
        doc["wifissid"] = WiFiSSID;
        doc["wifipwd"] = WiFiPWD;
        doc["srvrHostName"] = SrvrHostName;
        doc["srvrType"] = SrvrType;
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
            m_pConsole->printVerbose("sysconfig=fileexits;");
            String json = file.readString();
            file.close();

            if (json.length() == 0)
            {
                m_pConsole->printVerbose("sysconfig=errorread; // file length = 0");
                file.close();
                setDefaults();

                write();
            }
            else
            {
                if (!parseJSON(json))
                {
                    m_pConsole->printVerbose("sysconfig=errorread; //could not parse json");

                    setDefaults();
                    write();
                }
                else
                {
                    m_pConsole->printVerbose("sysconfig=fileread;");
                }
            }
        }
        else
        {
            m_pConsole->printVerbose("sysconfig=filedoesnotexists;");
            setDefaults();
            write();
        }
    }

    void write()
    {
        File file = SPIFFS.open(SYSCONFIG_FN, FILE_WRITE);
        if (!file)
        {
            m_pConsole->printError("sysconfig=failwrite; // could not open file or write");
        }
        else
        {
            String json = toJSON();
            size_t written = file.print(json);
            file.flush();
            file.close();

            if (written != json.length())
            {
                m_pConsole->printError("sysconfig=failwrite; // mismatch write, written: " + String(written) + " size: " + String(json.length()));
            }
            else
            {
                m_pConsole->printVerbose("sysconfig=writefile; // wrote " + String(written) + " bytes to " + String(file.name()));
            }
        }
    }

    void setDefaults()
    {
        GPRSModemBaudRate = 115200;
        WiFiEnabled = false;
        CellEnabled = false;
        Commissioned = false;
        VerboseLogging = false;
        DeviceId = "?";
        TLS = false;
        WiFiSSID = "";
        WiFiPWD = "";
        SrvrHostName = "?";
        SrvrType = "?";
        Anonymous = false;
        SrvrUID = "";
        SrvrPWD = "";
        DeviceAccessKey = "";

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

        if (err == ArduinoJson::DeserializationError::Ok)
        {
            Commissioned = doc["commissioned"].as<bool>();
            CellEnabled = doc["cellEnabled"].as<bool>();
            WiFiEnabled = doc["wifiEnabled"].as<bool>();
            TLS = doc["tls"].as<bool>();
            VerboseLogging = doc["verboseLogging"].as<bool>();
            DeviceId = doc["deviceId"].as<String>();
            WiFiSSID = doc["wifissid"].as<String>();
            WiFiPWD = doc["wifipwd"].as<String>();
            SrvrHostName = doc["srvrHostName"].as<String>();
            SrvrType = doc["srvrType"].as<String>();
            Anonymous = doc["anonymous"].as<bool>();
            SrvrUID = doc["srvrUid"].as<String>();
            SrvrPWD = doc["srvrPwd"].as<String>();
            PingRate = doc["pingRate"].as<uint32_t>();
            GPSEnabled = doc["gpsEnabled"].as<bool>();
            SendUpdateRate = doc["sendUpdateRate"].as<uint32_t>();
            GPSUpdateRate = doc["gpsUpdateRate"].as<uint32_t>();
            GPRSModemBaudRate = doc["baud"].as<unsigned long>();
            if (doc.containsKey("deviceAccessKey"))
            {
                String tmpKey = doc["deviceAccessKey"].as<String>();
                if (tmpKey.length() > 0)
                {
                    DeviceAccessKey = tmpKey;
                }
            }
            return true;
        }
        else
        {
            // TODO: Add back in error handling.
            //Serial.println("ERROR SYSCONFIG JSON");
            //Serial.println(str);
            return false;
        }
    }
};

#endif