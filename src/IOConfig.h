#ifndef IOCONFIG_H
#define IOCONFIG_H

#define ADC_CONFIG_NONE 0
#define ADC_CONFIG_ADC 1
#define ADC_CONFIG_CT 2

#define GPIO_CONFIG_NONE 0
#define GPIO_CONFIG_INPUT 1
#define GPIO_CONFIG_OUTPUT 2
#define GPIO_CONFIG_PULSE_COUNTER 3
#define GPIO_CONFIG_DBS18 4
#define GPIO_CONFIG_DHT11 5
#define GPIO_CONFIG_DHT22 6

#include <Arduino.h>
#include <ArduinoJson.h>

#include <SPIFFS.h>
#include <Console.h>

#define SETTINGS_FN "/ioconfig.json"

class IOConfig
{
private:
    Console *m_pConsole;

public:
    IOConfig(Console *pConsole)
    {
        m_pConsole = pConsole;
    }

    uint8_t ADC1Config = ADC_CONFIG_NONE;
    String ADC1Name;
    float ADC1Calibration;
    float ADC1Scaler;
    float ADC1Zero;

    uint8_t ADC2Config = ADC_CONFIG_NONE;
    String ADC2Name;
    float ADC2Calibration;
    float ADC2Scaler;
    float ADC2Zero;

    uint8_t ADC3Config = ADC_CONFIG_NONE;
    String ADC3Name;
    float ADC3Calibration;
    float ADC3Scaler;
    float ADC3Zero;

    uint8_t ADC4Config = ADC_CONFIG_NONE;
    String ADC4Name;
    float ADC4Calibration;
    float ADC4Scaler;
    float ADC4Zero;

    uint8_t ADC5Config = ADC_CONFIG_NONE;
    String ADC5Name;
    float ADC5Calibration;
    float ADC5Scaler;
    float ADC5Zero;

    uint8_t ADC6Config = ADC_CONFIG_NONE;
    String ADC6Name;
    float ADC6Calibration;
    float ADC6Scaler;
    float ADC6Zero;

    uint8_t ADC7Config = ADC_CONFIG_NONE;
    String ADC7Name;
    float ADC7Calibration;
    float ADC7Scaler;
    float ADC7Zero;

    uint8_t ADC8Config = ADC_CONFIG_NONE;
    String ADC8Name;
    float ADC8Calibration;
    float ADC8Scaler;
    float ADC8Zero;

    uint8_t GPIO1Config = GPIO_CONFIG_NONE;
    String GPIO1Name;
    float GPIO1Calibration;
    float GPIO1Scaler;
    float GPIO1Zero;

    uint8_t GPIO2Config = GPIO_CONFIG_NONE;
    String GPIO2Name;
    float GPIO2Calibration;
    float GPIO2Scaler;
    float GPIO2Zero;

    uint8_t GPIO3Config = GPIO_CONFIG_NONE;
    String GPIO3Name;
    float GPIO3Calibration;
    float GPIO3Scaler;
    float GPIO3Zero;

    uint8_t GPIO4Config = GPIO_CONFIG_NONE;
    String GPIO4Name;
    float GPIO4Calibration;
    float GPIO4Scaler;
    float GPIO4Zero;

    uint8_t GPIO5Config = GPIO_CONFIG_NONE;
    String GPIO5Name;
    float GPIO5Calibration;
    float GPIO5Scaler;
    float GPIO5Zero;

    uint8_t GPIO6Config = GPIO_CONFIG_NONE;
    String GPIO6Name;
    float GPIO6Calibration;
    float GPIO6Scaler;
    float GPIO6Zero;

    uint8_t GPIO7Config = GPIO_CONFIG_NONE;
    String GPIO7Name;
    float GPIO7Calibration;
    float GPIO7Scaler;
    float GPIO7Zero;

    uint8_t GPIO8Config = GPIO_CONFIG_NONE;
    String GPIO8Name;
    float GPIO8Calibration;
    float GPIO8Scaler;
    float GPIO8Zero;

    String toJSON()
    {
        const size_t capacity = JSON_OBJECT_SIZE(4096);
        DynamicJsonDocument doc(capacity);
        doc["adc1c"] = ADC1Config;
        doc["adc2c"] = ADC2Config;
        doc["adc3c"] = ADC3Config;
        doc["adc4c"] = ADC4Config;
        doc["adc5c"] = ADC5Config;
        doc["adc6c"] = ADC6Config;
        doc["adc7c"] = ADC7Config;
        doc["adc8c"] = ADC8Config;

        doc["adc1n"] = ADC1Name;
        doc["adc2n"] = ADC2Name;
        doc["adc3n"] = ADC3Name;
        doc["adc4n"] = ADC4Name;
        doc["adc5n"] = ADC5Name;
        doc["adc6n"] = ADC6Name;
        doc["adc7n"] = ADC7Name;
        doc["adc8n"] = ADC8Name;

        doc["adc1l"] = ADC1Calibration;
        doc["adc2l"] = ADC2Calibration;
        doc["adc3l"] = ADC3Calibration;
        doc["adc4l"] = ADC4Calibration;
        doc["adc5l"] = ADC5Calibration;
        doc["adc6l"] = ADC6Calibration;
        doc["adc7l"] = ADC7Calibration;
        doc["adc8l"] = ADC8Calibration;

        doc["adc1s"] = ADC1Scaler;
        doc["adc2s"] = ADC2Scaler;
        doc["adc3s"] = ADC3Scaler;
        doc["adc4s"] = ADC4Scaler;
        doc["adc5s"] = ADC5Scaler;
        doc["adc6s"] = ADC6Scaler;
        doc["adc7s"] = ADC7Scaler;
        doc["adc8s"] = ADC8Scaler;

        doc["adc1z"] = ADC1Zero;
        doc["adc2z"] = ADC2Zero;
        doc["adc3z"] = ADC3Zero;
        doc["adc4z"] = ADC4Zero;
        doc["adc5z"] = ADC5Zero;
        doc["adc6z"] = ADC6Zero;
        doc["adc7z"] = ADC7Zero;
        doc["adc8z"] = ADC8Zero;

        doc["io1c"] = GPIO1Config;
        doc["io2c"] = GPIO2Config;
        doc["io3c"] = GPIO3Config;
        doc["io4c"] = GPIO4Config;
        doc["io5c"] = GPIO5Config;
        doc["io6c"] = GPIO6Config;
        doc["io7c"] = GPIO7Config;
        doc["io8c"] = GPIO8Config;

        doc["io1n"] = GPIO1Name;
        doc["io2n"] = GPIO2Name;
        doc["io3n"] = GPIO3Name;
        doc["io4n"] = GPIO4Name;
        doc["io5n"] = GPIO5Name;
        doc["io6n"] = GPIO6Name;
        doc["io7n"] = GPIO7Name;
        doc["io8n"] = GPIO8Name;

        doc["io1l"] = GPIO1Calibration;
        doc["io2l"] = GPIO3Calibration;
        doc["io3l"] = GPIO3Calibration;
        doc["io4l"] = GPIO4Calibration;
        doc["io5l"] = GPIO5Calibration;
        doc["io6l"] = GPIO6Calibration;
        doc["io7l"] = GPIO7Calibration;
        doc["io8l"] = GPIO8Calibration;

        doc["io1s"] = GPIO1Scaler;
        doc["io2s"] = GPIO2Scaler;
        doc["io3s"] = GPIO3Scaler;
        doc["io4s"] = GPIO4Scaler;
        doc["io5s"] = GPIO5Scaler;
        doc["io6s"] = GPIO6Scaler;
        doc["io7s"] = GPIO7Scaler;
        doc["io8s"] = GPIO8Scaler;

        doc["io1z"] = GPIO1Zero;
        doc["io2z"] = GPIO2Zero;
        doc["io3z"] = GPIO3Zero;
        doc["io4z"] = GPIO4Zero;
        doc["io5z"] = GPIO5Zero;
        doc["io6z"] = GPIO6Zero;
        doc["io7z"] = GPIO7Zero;
        doc["io8z"] = GPIO8Zero;

        String output;
        serializeJson(doc, output);
        return output;
    }

    void load()
    {
        File file = SPIFFS.open(SETTINGS_FN, FILE_READ);
        if (file)
        {
            m_pConsole->printVerbose("ioconfig=fileexits;");

            String json = file.readString();
            file.close();
            if (json.length() == 0)
            {
                m_pConsole->printWarning("ioconfig=errorread; // file length = 0");
                setDefaults();
                write();
            }
            else
            {
                if (!parseJSON(json.c_str()))
                {
                    m_pConsole->printError("sysconfig=errorread; //could not parse json");
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
            m_pConsole->printWarning("ioconfig=filedoesnotexists;");
            setDefaults();
            write();
        }
    }

    void write()
    {
        File file = SPIFFS.open(SETTINGS_FN, FILE_WRITE);
        if (!file)
        {
            m_pConsole->printError("ioconfig=failwrite; // could not open file or write");
        }
        else
        {
            String json = toJSON();
            size_t written = file.print(json);
            file.flush();
            file.close();

            if (written != json.length())
            {
                m_pConsole->printError("ioconfig=failwrite; // mismatch write, written: " + String(written) + " size: " + String(json.length()));
            }
            else
            {
                m_pConsole->printVerbose("ioconfig=writefile; // wrote " + String(written) + " bytes to " + String(file.name()));
            }
        }
    }

    bool parseJSON(const char *str)
    {
        const size_t capacity = JSON_OBJECT_SIZE(4096);
        DynamicJsonDocument doc(capacity);

        DeserializationError result = deserializeJson(doc, str);
        if (result.code() == DeserializationError::Code::Ok)
        {
            ADC1Config = doc["adc1c"].as<uint8_t>();
            ADC2Config = doc["adc2c"].as<uint8_t>();
            ADC3Config = doc["adc3c"].as<uint8_t>();
            ADC4Config = doc["adc4c"].as<uint8_t>();
            ADC5Config = doc["adc5c"].as<uint8_t>();
            ADC6Config = doc["adc6c"].as<uint8_t>();
            ADC7Config = doc["adc7c"].as<uint8_t>();
            ADC8Config = doc["adc8c"].as<uint8_t>();

            ADC1Name = doc["adc1n"].as<String>();
            ADC2Name = doc["adc2n"].as<String>();
            ADC3Name = doc["adc3n"].as<String>();
            ADC4Name = doc["adc4n"].as<String>();
            ADC5Name = doc["adc5n"].as<String>();
            ADC6Name = doc["adc6n"].as<String>();
            ADC7Name = doc["adc7n"].as<String>();
            ADC8Name = doc["adc8n"].as<String>();

            ADC1Calibration = doc["adc1l"].as<float>();
            ADC2Calibration = doc["adc2l"].as<float>();
            ADC3Calibration = doc["adc3l"].as<float>();
            ADC4Calibration = doc["adc4l"].as<float>();
            ADC5Calibration = doc["adc5l"].as<float>();
            ADC6Calibration = doc["adc6l"].as<float>();
            ADC7Calibration = doc["adc7l"].as<float>();
            ADC8Calibration = doc["adc8l"].as<float>();

            ADC1Scaler = doc["adc1s"].as<float>();
            ADC2Scaler = doc["adc2s"].as<float>();
            ADC3Scaler = doc["adc3s"].as<float>();
            ADC4Scaler = doc["adc4s"].as<float>();
            ADC5Scaler = doc["adc5s"].as<float>();
            ADC6Scaler = doc["adc6s"].as<float>();
            ADC7Scaler = doc["adc7s"].as<float>();
            ADC8Scaler = doc["adc8s"].as<float>();

            ADC1Zero = doc["adc1z"].as<float>();
            ADC2Zero = doc["adc2z"].as<float>();
            ADC3Zero = doc["adc3z"].as<float>();
            ADC4Zero = doc["adc4z"].as<float>();
            ADC5Zero = doc["adc5z"].as<float>();
            ADC6Zero = doc["adc6z"].as<float>();
            ADC7Zero = doc["adc7z"].as<float>();
            ADC8Zero = doc["adc8z"].as<float>();

            GPIO1Config = doc["io1c"].as<uint8_t>();
            GPIO2Config = doc["io2c"].as<uint8_t>();
            GPIO3Config = doc["io3c"].as<uint8_t>();
            GPIO4Config = doc["io4c"].as<uint8_t>();
            GPIO5Config = doc["io5c"].as<uint8_t>();
            GPIO6Config = doc["io6c"].as<uint8_t>();
            GPIO7Config = doc["io7c"].as<uint8_t>();
            GPIO8Config = doc["io8c"].as<uint8_t>();

            GPIO1Name = doc["io1n"].as<String>();
            GPIO2Name = doc["io2n"].as<String>();
            GPIO3Name = doc["io3n"].as<String>();
            GPIO4Name = doc["io4n"].as<String>();
            GPIO5Name = doc["io5n"].as<String>();
            GPIO6Name = doc["io6n"].as<String>();
            GPIO7Name = doc["io7n"].as<String>();
            GPIO8Name = doc["io8n"].as<String>();

            GPIO1Calibration = doc["io1l"].as<float>();
            GPIO2Calibration = doc["io2l"].as<float>();
            GPIO3Calibration = doc["io3l"].as<float>();
            GPIO4Calibration = doc["io4l"].as<float>();
            GPIO5Calibration = doc["io5l"].as<float>();
            GPIO6Calibration = doc["io6l"].as<float>();
            GPIO7Calibration = doc["io7l"].as<float>();
            GPIO8Calibration = doc["io8l"].as<float>();

            GPIO1Scaler = doc["io1s"].as<float>();
            GPIO2Scaler = doc["io2s"].as<float>();
            GPIO3Scaler = doc["io3s"].as<float>();
            GPIO4Scaler = doc["io4s"].as<float>();
            GPIO5Scaler = doc["io5s"].as<float>();
            GPIO6Scaler = doc["io6s"].as<float>();
            GPIO7Scaler = doc["io7s"].as<float>();
            GPIO8Scaler = doc["io8s"].as<float>();

            GPIO1Zero = doc["io1z"].as<float>();
            GPIO2Zero = doc["io2z"].as<float>();
            GPIO3Zero = doc["io3z"].as<float>();
            GPIO4Zero = doc["io4z"].as<float>();
            GPIO5Zero = doc["io5z"].as<float>();
            GPIO6Zero = doc["io6z"].as<float>();
            GPIO7Zero = doc["io7z"].as<float>();
            GPIO8Zero = doc["io8z"].as<float>();
            return true;
        }

        return false;
    }

    void setDefaults()
    {
        ADC1Config = ADC_CONFIG_NONE;
        ADC2Config = ADC_CONFIG_NONE;
        ADC3Config = ADC_CONFIG_NONE;
        ADC4Config = ADC_CONFIG_NONE;
        ADC5Config = ADC_CONFIG_NONE;
        ADC6Config = ADC_CONFIG_NONE;
        ADC7Config = ADC_CONFIG_NONE;
        ADC8Config = ADC_CONFIG_NONE;

        ADC1Name = "adc1";
        ADC2Name = "adc2";
        ADC3Name = "adc3";
        ADC4Name = "adc4";
        ADC5Name = "adc5";
        ADC6Name = "adc6";
        ADC7Name = "adc7";
        ADC8Name = "adc8";

        ADC1Calibration = 1.0;
        ADC2Calibration = 1.0;
        ADC3Calibration = 1.0;
        ADC4Calibration = 1.0;
        ADC5Calibration = 1.0;
        ADC6Calibration = 1.0;
        ADC7Calibration = 1.0;
        ADC8Calibration = 1.0;

        ADC1Scaler = 1.0;
        ADC2Scaler = 1.0;
        ADC3Scaler = 1.0;
        ADC4Scaler = 1.0;
        ADC5Scaler = 1.0;
        ADC6Scaler = 1.0;
        ADC7Scaler = 1.0;
        ADC8Scaler = 1.0;

        ADC1Zero = 0.0;
        ADC2Zero = 0.0;
        ADC3Zero = 0.0;
        ADC4Zero = 0.0;
        ADC5Zero = 0.0;
        ADC6Zero = 0.0;
        ADC7Zero = 0.0;
        ADC8Zero = 0.0;

        GPIO1Config = GPIO_CONFIG_NONE;
        GPIO2Config = GPIO_CONFIG_NONE;
        GPIO3Config = GPIO_CONFIG_NONE;
        GPIO4Config = GPIO_CONFIG_NONE;
        GPIO5Config = GPIO_CONFIG_NONE;
        GPIO6Config = GPIO_CONFIG_NONE;
        GPIO7Config = GPIO_CONFIG_NONE;
        GPIO8Config = GPIO_CONFIG_NONE;

        GPIO1Name = "io1";
        GPIO2Name = "io2";
        GPIO3Name = "io3";
        GPIO4Name = "io4";
        GPIO5Name = "io5";
        GPIO6Name = "io6";
        GPIO7Name = "io7";
        GPIO8Name = "io8";

        GPIO1Calibration = 1.0;
        GPIO2Calibration = 1.0;
        GPIO3Calibration = 1.0;
        GPIO4Calibration = 1.0;
        GPIO5Calibration = 1.0;
        GPIO6Calibration = 1.0;
        GPIO7Calibration = 1.0;
        GPIO8Calibration = 1.0;

        GPIO1Scaler = 1.0;
        GPIO2Scaler = 1.0;
        GPIO3Scaler = 1.0;
        GPIO4Scaler = 1.0;
        GPIO5Scaler = 1.0;
        GPIO6Scaler = 1.0;
        GPIO7Scaler = 1.0;
        GPIO8Scaler = 1.0;

        GPIO1Zero = 0.0;
        GPIO2Zero = 0.0;
        GPIO3Zero = 0.0;
        GPIO4Zero = 0.0;
        GPIO5Zero = 0.0;
        GPIO6Zero = 0.0;
        GPIO7Zero = 0.0;
        GPIO8Zero = 0.0;
    }

    void getADCConfigString(char *str)
    {
        sprintf(str, "%d,%d,%d,%d,%d,%d,%d,%d",
                ADC1Config, ADC2Config, ADC3Config, ADC4Config, ADC5Config, ADC6Config, ADC7Config, ADC8Config);
    }

    void getADCNamesString(char *str)
    {
        sprintf(str, "%s,%s,%s,%s,%s,%s,%s,%s",
                ADC1Name.c_str(), ADC2Name.c_str(), ADC3Name.c_str(), ADC4Name.c_str(), ADC5Name.c_str(), ADC6Name.c_str(), ADC7Name.c_str(), ADC8Name.c_str());
    }

    void getADCScalerString(char *str)
    {
        sprintf(str, "%f,%f,%f,%f,%f,%f,%f,%f",
                ADC1Scaler, ADC2Scaler, ADC3Scaler, ADC4Scaler, ADC5Scaler, ADC6Scaler, ADC7Scaler, ADC8Scaler);
    }

    void getADCZeroString(char *str)
    {
        sprintf(str, "%f,%f,%f,%f,%f,%f,%f,%f",
                ADC1Zero, ADC2Zero, ADC3Zero, ADC4Zero, ADC5Zero, ADC6Zero, ADC7Zero, ADC8Zero);
    }

    void getGPIOConfigString(char *str)
    {
        sprintf(str, "%d,%d,%d,%d,%d,%d,%d,%d",
                GPIO1Config, GPIO2Config, GPIO3Config, GPIO4Config, GPIO5Config, GPIO6Config, GPIO7Config, GPIO8Config);
    }

    void getGPIONamesString(char *str)
    {
        sprintf(str, "%s,%s,%s,%s,%s,%s,%s,%s",
                GPIO1Name.c_str(), GPIO2Name.c_str(), GPIO3Name.c_str(), GPIO4Name.c_str(), GPIO5Name.c_str(), GPIO6Name.c_str(), GPIO7Name.c_str(), GPIO8Name.c_str());
    }

    void getGPIOScalerString(char *str)
    {
        sprintf(str, "%f,%f,%f,%f,%f,%f,%f,%f",
                GPIO1Scaler, GPIO2Scaler, GPIO3Scaler, GPIO4Scaler, GPIO5Scaler, GPIO6Scaler, GPIO7Scaler, GPIO8Scaler);
    }

    void getGPIOZeroString(char *str)
    {
        sprintf(str, "%f,%f,%f,%f,%f,%f,%f,%f",
                GPIO1Zero, GPIO2Zero, GPIO3Zero, GPIO4Zero, GPIO5Zero, GPIO6Zero, GPIO7Zero, GPIO8Zero);
    }

    void getADC(char *str, int port)
    {
        sprintf(str, "err - invalid adc port %d", port);

        switch (port)
        {
        case 1:
            sprintf(str, "a,1,%d,%f,%f,%f", ADC1Config, ADC1Calibration, ADC1Zero, ADC1Scaler);
            break;
        case 2:
            sprintf(str, "a,2,%d,%f,%f,%f", ADC2Config, ADC2Calibration, ADC2Zero, ADC2Scaler);
            break;
        case 3:
            sprintf(str, "a,3,%d,%f,%f,%f", ADC3Config, ADC3Calibration, ADC3Zero, ADC3Scaler);
            break;
        case 4:
            sprintf(str, "a,4,%d,%f,%f,%f", ADC4Config, ADC4Calibration, ADC4Zero, ADC4Scaler);
            break;
        case 5:
            sprintf(str, "a,5,%d,%f,%f,%f", ADC5Config, ADC5Calibration, ADC5Zero, ADC5Scaler);
            break;
        case 6:
            sprintf(str, "a,6,%d,%f,%f,%f", ADC6Config, ADC6Calibration, ADC6Zero, ADC6Scaler);
            break;
        case 7:
            sprintf(str, "a,7,%d,%f,%f,%f", ADC7Config, ADC7Calibration, ADC7Zero, ADC7Scaler);
            break;
        case 8:
            sprintf(str, "a,8,%d,%f,%f,%f", ADC8Config, ADC8Calibration, ADC8Zero, ADC8Scaler);
            break;
        }
    }

    int setADC(char *str)
    {
        /* Skip first */
        char *tmp = strtok(str, ",");
        int port = atoi(tmp);
        tmp = strtok(NULL, ",");
        if (tmp == NULL)
            return -1;

        int sensorType = atoi(tmp);
        tmp = strtok(NULL, ",");
        if (tmp == NULL)
            return -1;

        float calibration = atof(tmp);
        tmp = strtok(NULL, ",");
        if (tmp == NULL)
            return -1;

        float zero = atof(tmp);
        tmp = strtok(NULL, ",");
        if (tmp == NULL)
            return -1;
        float scaler = atof(tmp);

        switch (port)
        {
        case 1:
            ADC1Config = sensorType;
            ADC1Calibration = calibration;
            ADC1Zero = zero;
            ADC1Scaler = scaler;
            break;
        case 2:
            ADC2Config = sensorType;
            ADC2Calibration = calibration;
            ADC2Zero = zero;
            ADC2Scaler = scaler;
            break;
        case 3:
            ADC3Config = sensorType;
            ADC3Calibration = calibration;
            ADC3Zero = zero;
            ADC3Scaler = scaler;
            break;
        case 4:
            ADC4Config = sensorType;
            ADC4Calibration = calibration;
            ADC4Zero = zero;
            ADC4Scaler = scaler;
            break;
        case 5:
            ADC5Config = sensorType;
            ADC5Calibration = calibration;
            ADC5Zero = zero;
            ADC5Scaler = scaler;
            break;
        case 6:
            ADC6Config = sensorType;
            ADC6Calibration = calibration;
            ADC6Zero = zero;
            ADC6Scaler = scaler;
            break;
        case 7:
            ADC7Config = sensorType;
            ADC7Calibration = calibration;
            ADC7Zero = zero;
            ADC7Scaler = scaler;
            break;
        case 8:
            ADC8Config = sensorType;
            ADC8Calibration = calibration;
            ADC8Zero = zero;
            ADC8Scaler = scaler;
            break;
        }

        return 0;
    }

    void getGPIO(char *str, int port)
    {
        sprintf(str, "err - invalid gpio port %d", port);

        switch (port)
        {
        case 1:
            sprintf(str, "a,1,%d,%f,%f,%f", GPIO1Config, GPIO1Calibration, GPIO1Zero, GPIO1Scaler);
            break;
        case 2:
            sprintf(str, "a,2,%d,%f,%f,%f", GPIO2Config, GPIO2Calibration, GPIO2Zero, GPIO2Scaler);
            break;
        case 3:
            sprintf(str, "a,3,%d,%f,%f,%f", GPIO3Config, GPIO3Calibration, GPIO3Zero, GPIO3Scaler);
            break;
        case 4:
            sprintf(str, "a,4,%d,%f,%f,%f", GPIO4Config, GPIO4Calibration, GPIO4Zero, GPIO4Scaler);
            break;
        case 5:
            sprintf(str, "a,5,%d,%f,%f,%f", GPIO5Config, GPIO5Calibration, GPIO5Zero, GPIO5Scaler);
            break;
        case 6:
            sprintf(str, "a,6,%d,%f,%f,%f", GPIO6Config, GPIO6Calibration, GPIO6Zero, GPIO6Scaler);
            break;
        case 7:
            sprintf(str, "a,7,%d,%f,%f,%f", GPIO7Config, GPIO7Calibration, GPIO7Zero, GPIO7Scaler);
            break;
        case 8:
            sprintf(str, "a,8,%d,%f,%f,%f", GPIO8Config, GPIO8Calibration, GPIO8Zero, GPIO8Scaler);
            break;
        }
    }

    int setGPIO(char *str)
    {
        /* Skip first */
        strtok(str, ",");
        char *tmp = strtok(NULL, ",");
        int port = atoi(tmp);
        tmp = strtok(NULL, ",");
        if (tmp == NULL)
            return -1;

        int sensorType = atoi(tmp);
        tmp = strtok(NULL, ",");
        if (tmp == NULL)
            return -1;

        float calibration = atof(tmp);
        tmp = strtok(NULL, ",");
        if (tmp == NULL)
            return -1;

        float zero = atof(tmp);
        tmp = strtok(NULL, ",");
        if (tmp == NULL)
            return -1;
        float scaler = atof(tmp);

        switch (port)
        {
        case 1:
            GPIO1Config = sensorType;
            GPIO1Calibration = calibration;
            GPIO1Zero = zero;
            GPIO1Scaler = scaler;
            break;
        case 2:
            GPIO2Config = sensorType;
            GPIO2Calibration = calibration;
            GPIO2Zero = zero;
            GPIO2Scaler = scaler;
            break;
        case 3:
            GPIO3Config = sensorType;
            GPIO3Calibration = calibration;
            GPIO3Zero = zero;
            GPIO3Scaler = scaler;
            break;
        case 4:
            GPIO4Config = sensorType;
            GPIO4Calibration = calibration;
            GPIO4Zero = zero;
            GPIO4Scaler = scaler;
            break;
        case 5:
            GPIO5Config = sensorType;
            GPIO5Calibration = calibration;
            GPIO5Zero = zero;
            GPIO5Scaler = scaler;
            break;
        case 6:
            GPIO6Config = sensorType;
            GPIO6Calibration = calibration;
            GPIO6Zero = zero;
            GPIO6Scaler = scaler;
            break;
        case 7:
            GPIO7Config = sensorType;
            GPIO7Calibration = calibration;
            GPIO7Zero = zero;
            GPIO7Scaler = scaler;
            break;
        case 8:
            GPIO8Config = sensorType;
            GPIO8Calibration = calibration;
            GPIO8Zero = zero;
            GPIO8Scaler = scaler;
            break;
        }

        return 0;
    }

    void setADCConfig(char *str)
    {

        char *foundIdx = strtok(str, ",");
        char *foundConfig = strtok(NULL, ",");

        int idx = atoi(foundIdx);
        int cfg = atoi(foundConfig);

        switch (idx)
        {
        case 1:
            ADC1Config = cfg;
            break;
        case 2:
            ADC2Config = cfg;
            break;
        case 3:
            ADC3Config = cfg;
            break;
        case 4:
            ADC4Config = cfg;
            break;
        case 5:
            ADC5Config = cfg;
            break;
        case 6:
            ADC6Config = cfg;
            break;
        case 7:
            ADC7Config = cfg;
            break;
        case 8:
            ADC8Config = cfg;
            break;
        }
    }

    void setADCCalibration(char *str)
    {
        char *foundIdx = strtok(str, ",");
        char *foundConfig = strtok(NULL, ",");

        int idx = atoi(foundIdx);
        float cfg = atof(foundConfig);

        switch (idx)
        {
        case 1:
            ADC1Calibration = cfg;
            break;
        case 2:
            ADC1Calibration = cfg;
            break;
        case 3:
            ADC1Calibration = cfg;
            break;
        case 4:
            ADC1Calibration = cfg;
            break;
        case 5:
            ADC1Calibration = cfg;
            break;
        case 6:
            ADC1Calibration = cfg;
            break;
        case 7:
            ADC1Calibration = cfg;
            break;
        case 8:
            ADC1Calibration = cfg;
            break;
        }
    }

    void setADCScaler(char *str)
    {
        char *foundIdx = strtok(str, ",");
        char *foundConfig = strtok(NULL, ",");

        int idx = atoi(foundIdx);
        float cfg = atof(foundConfig);

        switch (idx)
        {
        case 1:
            ADC1Scaler = cfg;
            break;
        case 2:
            ADC2Scaler = cfg;
            break;
        case 3:
            ADC3Scaler = cfg;
            break;
        case 4:
            ADC4Scaler = cfg;
            break;
        case 5:
            ADC5Scaler = cfg;
            break;
        case 6:
            ADC6Scaler = cfg;
            break;
        case 7:
            ADC7Scaler = cfg;
            break;
        case 8:
            ADC8Scaler = cfg;
            break;
        }
    }

    void setADCZero(char *str)
    {
        char *foundIdx = strtok(str, ",");
        char *foundConfig = strtok(NULL, ",");

        int idx = atoi(foundIdx);
        float cfg = atof(foundConfig);

        switch (idx)
        {
        case 1:
            ADC1Zero = cfg;
            break;
        case 2:
            ADC2Zero = cfg;
            break;
        case 3:
            ADC3Zero = cfg;
            break;
        case 4:
            ADC4Zero = cfg;
            break;
        case 5:
            ADC5Zero = cfg;
            break;
        case 6:
            ADC6Zero = cfg;
            break;
        case 7:
            ADC7Zero = cfg;
            break;
        case 8:
            ADC8Zero = cfg;
            break;
        }
    }

    void setGPIOConfig(char *str)
    {
        char *foundIdx = strtok(str, ",");
        char *foundConfig = strtok(NULL, ",");

        int idx = atoi(foundIdx);
        int cfg = atoi(foundConfig);

        switch (idx)
        {
        case 1:
            GPIO1Config = cfg;
            break;
        case 2:
            GPIO2Config = cfg;
            break;
        case 3:
            GPIO3Config = cfg;
            break;
        case 4:
            GPIO4Config = cfg;
            break;
        case 5:
            GPIO5Config = cfg;
            break;
        case 6:
            GPIO6Config = cfg;
            break;
        case 7:
            GPIO7Config = cfg;
            break;
        case 8:
            GPIO8Config = cfg;
            break;
        }
    }

    void setGPIOCalibration(char *str)
    {
        char *foundIdx = strtok(str, ",");
        char *foundConfig = strtok(NULL, ",");

        int idx = atoi(foundIdx);
        float cfg = atof(foundConfig);

        switch (idx)
        {
        case 1:
            GPIO1Calibration = cfg;
            break;
        case 2:
            GPIO2Calibration = cfg;
            break;
        case 3:
            GPIO3Calibration = cfg;
            break;
        case 4:
            GPIO4Calibration = cfg;
            break;
        case 5:
            GPIO5Calibration = cfg;
            break;
        case 6:
            GPIO6Calibration = cfg;
            break;
        case 7:
            GPIO7Calibration = cfg;
            break;
        case 8:
            GPIO8Calibration = cfg;
            break;
        }
    }

    void setGPIOScaler(char *str)
    {
        char *foundIdx = strtok(str, ",");
        char *foundConfig = strtok(NULL, ",");

        int idx = atoi(foundIdx);
        float cfg = atof(foundConfig);

        switch (idx)
        {
        case 1:
            GPIO1Scaler = cfg;
            break;
        case 2:
            GPIO2Scaler = cfg;
            break;
        case 3:
            GPIO3Scaler = cfg;
            break;
        case 4:
            GPIO4Scaler = cfg;
            break;
        case 5:
            GPIO5Scaler = cfg;
            break;
        case 6:
            GPIO6Scaler = cfg;
            break;
        case 7:
            GPIO7Scaler = cfg;
            break;
        case 8:
            GPIO8Scaler = cfg;
            break;
        }
    }

    void setGPIOZero(char *str)
    {
        char *foundIdx = strtok(str, ",");
        char *foundConfig = strtok(NULL, ",");

        int idx = atoi(foundIdx);
        float cfg = atof(foundConfig);

        switch (idx)
        {
        case 1:
            GPIO1Zero = cfg;
            break;
        case 2:
            GPIO2Zero = cfg;
            break;
        case 3:
            GPIO3Zero = cfg;
            break;
        case 4:
            GPIO4Zero = cfg;
            break;
        case 5:
            GPIO5Zero = cfg;
            break;
        case 6:
            GPIO6Zero = cfg;
            break;
        case 7:
            GPIO7Zero = cfg;
            break;
        case 8:
            GPIO8Zero = cfg;
            break;
        }
    }
};

#endif