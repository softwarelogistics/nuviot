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
    int Port;
    
    bool TLS;
    bool Anonymous;
    String SrvrUID;
    String SrvrType;
    String SrvrPWD;
    
    String OrgId;
    String Id;
    String RepoId;

    String DeviceAccessKey;

    bool GPSEnabled;
    bool CellEnabled;
    bool WiFiEnabled;
    bool VerboseLogging;

    uint32_t PingRateSecond;
    uint32_t SendUpdateRateMS;
    uint32_t LoopUpdateRateMS;
    uint32_t GPSUpdateRateMS;
    unsigned long GPRSModemBaudRate;

    String toJSON(){

        const size_t capacity = JSON_OBJECT_SIZE(64);
        DynamicJsonDocument doc(capacity);
        doc["baud"] = GPRSModemBaudRate;        
        doc["tls"] = TLS;
        doc["commissioned"] = Commissioned;
        doc["deviceId"] = DeviceId;
        doc["wifissid"] = WiFiSSID;
        doc["wifipwd"] = WiFiPWD;
        doc["srvrHostName"] = SrvrHostName;
        doc["port"] = Port;
        doc["srvrType"] = SrvrType;
        doc["anonymous"] = Anonymous;
        doc["srvrUid"] = SrvrUID;
        doc["srvrPwd"] = SrvrPWD;
        doc["pingRate"] = PingRateSecond;
        doc["verboseLogging"] = VerboseLogging;
        doc["sendUpdateRate"] = SendUpdateRateMS;
        doc["loopUpdateRate"] = LoopUpdateRateMS;
        doc["gpsUpdateRate"] = GPSUpdateRateMS;
        doc["gpsEnabled"] = GPSEnabled;
        doc["wifiEnabled"] = WiFiEnabled;
        doc["cellEnabled"] = CellEnabled;
        doc["orgId"] = OrgId;
        doc["id"] = Id;
        doc["repoId"] = RepoId;

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

    bool writeFlag = false;
    void setWriteFlag() {
        writeFlag = true;
    }

    bool getWriteFlag() {
        return writeFlag;
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
                m_pConsole->println("sysconfig=writefile; // wrote " + String(written) + " bytes to " + SYSCONFIG_FN);
            }
        }

        writeFlag = false;
    }

    void setDefaults(){
        GPRSModemBaudRate = 115200;
        WiFiEnabled = false;
        CellEnabled = false;
        Commissioned = false;
        VerboseLogging = false;
        DeviceId = "";
        TLS = false;
        WiFiSSID = "";
        WiFiPWD = "";
        SrvrHostName = "";
        Port = 80;
        SrvrType = "";
        Anonymous = false;
        SrvrUID = "";
        SrvrPWD = "";
        DeviceAccessKey = "";

        PingRateSecond = 30;
        SendUpdateRateMS = 250;
        LoopUpdateRateMS = 250;

        GPSUpdateRateMS = 1000;        
        GPSEnabled = false;

        OrgId = "";
        Id = "";
        RepoId = "";
    }

    bool parseJSON(String json)
    {
        return parseJSON(json.c_str());
    }

    bool parseJSON(const char *str)
    {
        const size_t capacity = JSON_OBJECT_SIZE(64);
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
            Port = doc["port"].as<uint16_t>();
            SrvrType = doc["srvrType"].as<String>();
            Anonymous = doc["anonymous"].as<bool>();
            SrvrUID = doc["srvrUid"].as<String>();
            SrvrPWD = doc["srvrPwd"].as<String>();
            PingRateSecond = doc["pingRate"].as<uint32_t>();
            GPSEnabled = doc["gpsEnabled"].as<bool>();
            SendUpdateRateMS = doc["sendUpdateRate"].as<uint32_t>();
            LoopUpdateRateMS = doc["loopUpdateRate"].as<uint32_t>();
            GPSUpdateRateMS = doc["gpsUpdateRate"].as<uint32_t>();
            GPRSModemBaudRate = doc["baud"].as<unsigned long>();

            if(LoopUpdateRateMS == 0) LoopUpdateRateMS = 250;            
            if(SendUpdateRateMS < 250) SendUpdateRateMS = 250;

            OrgId = doc["orgId"].as<String>();
            RepoId = doc["repoId"].as<String>();
            Id = doc["id"].as<String>();

            if(OrgId == "null") OrgId = "";
            if(RepoId == "null") RepoId = "";
            if(Id == "null") Id = "";

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
            m_pConsole->printError("Error reading JSON");
            return false;
        }
    }

    void print() {
        m_pConsole->println("DEID: " + DeviceId);
        m_pConsole->println("COMM: " + String(Commissioned));
        m_pConsole->println("CELL: " + String(CellEnabled));
        m_pConsole->println("WIFI: " + String(WiFiEnabled));
        m_pConsole->println("SRTY: " + String(SrvrType));
        m_pConsole->println("HOST: " + SrvrHostName);
        m_pConsole->println("ORG : " + OrgId);
        m_pConsole->println("REP : " + RepoId);
        m_pConsole->println("ID  : " + Id);
        m_pConsole->println("PNGR: " + String(PingRateSecond));
        m_pConsole->println("SNDR: " + String(SendUpdateRateMS));
        m_pConsole->println("LOPR: " + String(LoopUpdateRateMS));
        m_pConsole->println("GPSR: " + String(GPSUpdateRateMS));
    }

};

#endif