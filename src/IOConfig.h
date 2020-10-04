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

#define SETTINGS_FN "/ioconfig.json"

class IOConfig {
    public:
    uint8_t ADC1Config = ADC_CONFIG_NONE;
    String ADC1Name;
    String ADC1Label;
    float ADC1Scaler;

    uint8_t ADC2Config = ADC_CONFIG_NONE;
    String ADC2Name;
    String ADC2Label;
    float ADC2Scaler;
    uint8_t ADC3Config;
    String ADC3Name;
    String ADC3Label;
    float ADC3Scaler;

    uint8_t ADC4Config = ADC_CONFIG_NONE;
    String ADC4Name;
    String ADC4Label;
    float ADC4Scaler;

    uint8_t ADC5Config = ADC_CONFIG_NONE;
    String ADC5Name;
    String ADC5Label;
    float ADC5Scaler;

    uint8_t ADC6Config = ADC_CONFIG_NONE;
    String ADC6Name;
    String ADC6Label;
    float ADC6Scaler;

    uint8_t ADC7Config = ADC_CONFIG_NONE;
    String ADC7Name;
    String ADC7Label;
    float ADC7Scaler;

    uint8_t ADC8Config = ADC_CONFIG_NONE;
    String ADC8Name;
    String ADC8Label;
    float ADC8Scaler;

    uint8_t GPIO1Config = GPIO_CONFIG_NONE;
    String GPIO1Name;
    String GPIO1Label;
    float GPIO1Scaler;

    uint8_t GPIO2Config = GPIO_CONFIG_NONE;
    String GPIO2Name;
    String GPIO2Label;
    float GPIO2Scaler;

    uint8_t GPIO3Config = GPIO_CONFIG_NONE;
    String GPIO3Name;
    String GPIO3Label;
    float GPIO3Scaler;

    uint8_t GPIO4Config = GPIO_CONFIG_NONE;
    String GPIO4Name;
    String GPIO4Label;
    float GPIO4Scaler;
    
    uint8_t GPIO5Config = GPIO_CONFIG_NONE;
    String GPIO5Name;
    String GPIO5Label;
    float GPIO5Scaler;

    uint8_t GPIO6Config = GPIO_CONFIG_NONE;
    String GPIO6Name;
    String GPIO6Label;
    float GPIO6Scaler;

    uint8_t GPIO7Config = GPIO_CONFIG_NONE;
    String GPIO7Name;
    String GPIO7Label;
    float GPIO7Scaler;
    
    uint8_t GPIO8Config = GPIO_CONFIG_NONE;
    String GPIO8Name;
    String GPIO8Label;
    float GPIO8Scaler;

    String toJSON() {
        const size_t capacity = JSON_OBJECT_SIZE(120);
        DynamicJsonDocument doc(capacity);
        doc["adc1c"] = ADC1Config;
        doc["adc2c"] = ADC2Config;
        doc["adc3c"] = ADC3Config;
        doc["adc4c"] = ADC4Config;
        doc["adc5c"] = ADC5Config;
        doc["adc6c"] = ADC6Config;
        doc["adc7c"] = ADC7Config;
        doc["adc8c"] = ADC8Config;

        doc["adc1l"] = ADC1Label;
        doc["adc2l"] = ADC2Label;
        doc["adc3l"] = ADC3Label;
        doc["adc4l"] = ADC4Label;
        doc["adc5l"] = ADC5Label;
        doc["adc6l"] = ADC6Label;
        doc["adc7l"] = ADC7Label;
        doc["adc8l"] = ADC8Label;

        doc["adc1n"] = ADC1Name;
        doc["adc2n"] = ADC2Name;
        doc["adc3n"] = ADC3Name;
        doc["adc4n"] = ADC4Name;
        doc["adc5n"] = ADC5Name;
        doc["adc6n"] = ADC6Name;
        doc["adc7n"] = ADC7Name;
        doc["adc8n"] = ADC8Name;
    
        doc["adc1s"] = ADC1Scaler;
        doc["adc2s"] = ADC2Scaler;
        doc["adc3s"] = ADC3Scaler;
        doc["adc4s"] = ADC4Scaler;
        doc["adc5s"] = ADC5Scaler;
        doc["adc6s"] = ADC6Scaler;
        doc["adc7s"] = ADC7Scaler;
        doc["adc8s"] = ADC8Scaler;


        doc["io1c"] = GPIO1Config;
        doc["io2c"] = GPIO2Config;
        doc["io3c"] = GPIO3Config;
        doc["io4c"] = GPIO4Config;
        doc["io5c"] = GPIO5Config;
        doc["io6c"] = GPIO6Config;
        doc["io7c"] = GPIO7Config;
        doc["io8c"] = GPIO8Config;

        doc["io1l"] = GPIO1Label;
        doc["io2l"] = GPIO2Label;
        doc["io3l"] = GPIO3Label;
        doc["io4l"] = GPIO4Label;
        doc["io5l"] = GPIO5Label;
        doc["io6l"] = GPIO6Label;
        doc["io7l"] = GPIO7Label;
        doc["io8l"] = GPIO8Label;

        doc["io1n"] = GPIO1Name;
        doc["io2n"] = GPIO2Name;
        doc["io3n"] = GPIO3Name;
        doc["io4n"] = GPIO4Name;
        doc["io5n"] = GPIO5Name;
        doc["io6n"] = GPIO6Name;
        doc["io7n"] = GPIO7Name;
        doc["io8n"] = GPIO8Name;

        doc["io1s"] = GPIO1Scaler;
        doc["io2s"] = GPIO2Scaler;
        doc["io3s"] = GPIO3Scaler;
        doc["io4s"] = GPIO4Scaler;
        doc["io5s"] = GPIO5Scaler;
        doc["io6s"] = GPIO6Scaler;
        doc["io7s"] = GPIO7Scaler;
        doc["io8s"] = GPIO8Scaler;
        
        String output;
        serializeJson(doc, output);

        return output;    
    }

    void load() {
        File file = SPIFFS.open(SETTINGS_FN, FILE_READ);
        if(file) {
            
            String json = file.readString();

            if(json.length() == 0)
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
        else {
            setDefaults();
            write();
        }
    }

    void write() {
        File file = SPIFFS.open(SETTINGS_FN, FILE_WRITE);
        file.print(toJSON());
        file.flush();
        file.close();
    }

    bool parseJSON(const char *str)
    {
        const size_t capacity = JSON_OBJECT_SIZE(120);
        DynamicJsonDocument doc(capacity);
        
        DeserializationError result = deserializeJson(doc, str);
        if(result.code() == DeserializationError::Code::Ok) {
            ADC1Config = doc["adc1c"].as<uint8_t>();
            ADC2Config = doc["adc2c"].as<uint8_t>();
            ADC3Config = doc["adc3c"].as<uint8_t>();
            ADC4Config = doc["adc4c"].as<uint8_t>();
            ADC5Config = doc["adc5c"].as<uint8_t>();
            ADC6Config = doc["adc6c"].as<uint8_t>();
            ADC7Config = doc["adc7c"].as<uint8_t>();
            ADC8Config = doc["adc8c"].as<uint8_t>();

            ADC1Label = doc["adc1l"].as<String>();
            ADC2Label = doc["adc2l"].as<String>();
            ADC3Label = doc["adc3l"].as<String>();
            ADC4Label = doc["adc4l"].as<String>();
            ADC5Label = doc["adc5l"].as<String>();
            ADC6Label = doc["adc6l"].as<String>();
            ADC7Label = doc["adc7l"].as<String>();
            ADC8Label = doc["adc8l"].as<String>();

            ADC1Name = doc["adc1n"].as<String>();
            ADC2Name = doc["adc2n"].as<String>();
            ADC3Name = doc["adc3n"].as<String>();
            ADC4Name = doc["adc4n"].as<String>();
            ADC5Name = doc["adc5n"].as<String>();
            ADC6Name = doc["adc6n"].as<String>();
            ADC7Name = doc["adc7n"].as<String>();
            ADC8Name = doc["adc8n"].as<String>();
        
            ADC1Scaler = doc["adc1s"].as<float>();
            ADC2Scaler = doc["adc2s"].as<float>();
            ADC3Scaler = doc["adc3s"].as<float>();
            ADC4Scaler = doc["adc4s"].as<float>();
            ADC5Scaler = doc["adc5s"].as<float>();
            ADC6Scaler = doc["adc6s"].as<float>();
            ADC7Scaler = doc["adc7s"].as<float>();
            ADC8Scaler = doc["adc8s"].as<float>();

            GPIO1Config = doc["io1c"].as<uint8_t>();
            GPIO2Config = doc["io2c"].as<uint8_t>();
            GPIO3Config = doc["io3c"].as<uint8_t>();
            GPIO4Config = doc["io4c"].as<uint8_t>();
            GPIO5Config = doc["io5c"].as<uint8_t>();
            GPIO6Config = doc["io6c"].as<uint8_t>();
            GPIO7Config = doc["io7c"].as<uint8_t>();
            GPIO8Config = doc["io8c"].as<uint8_t>();

            GPIO1Label = doc["io1l"].as<String>();
            GPIO2Label = doc["io2l"].as<String>();
            GPIO3Label = doc["io3l"].as<String>();
            GPIO4Label = doc["io4l"].as<String>();
            GPIO5Label = doc["io5l"].as<String>();
            GPIO6Label = doc["io6l"].as<String>();
            GPIO7Label = doc["io7l"].as<String>();
            GPIO8Label = doc["io8l"].as<String>();

            GPIO1Name = doc["io1n"].as<String>();
            GPIO2Name = doc["io2n"].as<String>();
            GPIO3Name = doc["io3n"].as<String>();
            GPIO4Name = doc["io4n"].as<String>();
            GPIO5Name = doc["io5n"].as<String>();
            GPIO6Name = doc["io6n"].as<String>();
            GPIO7Name = doc["io7n"].as<String>();
            GPIO8Name = doc["io8n"].as<String>();

            GPIO1Scaler = doc["io1s"].as<float>();
            GPIO2Scaler = doc["io2s"].as<float>();
            GPIO3Scaler = doc["io3s"].as<float>();
            GPIO4Scaler = doc["io4s"].as<float>();
            GPIO5Scaler = doc["io5s"].as<float>();
            GPIO6Scaler = doc["io6s"].as<float>();
            GPIO7Scaler = doc["io7s"].as<float>();
            GPIO8Scaler = doc["io8s"].as<float>();
            return true;
        }

        return false;
    }

    void parseJSON(String json) {
        parseJSON(json.c_str());
   }

    void setDefaults(){
        ADC1Config = ADC_CONFIG_NONE;
        ADC2Config = ADC_CONFIG_NONE;
        ADC3Config = ADC_CONFIG_NONE;
        ADC4Config = ADC_CONFIG_NONE;
        ADC5Config = ADC_CONFIG_NONE;
        ADC6Config = ADC_CONFIG_NONE;
        ADC7Config = ADC_CONFIG_NONE;
        ADC8Config = ADC_CONFIG_NONE;

        ADC1Label = "V1";
        ADC2Label = "V2";
        ADC3Label = "V3";
        ADC4Label = "V4";
        ADC5Label = "V5";
        ADC6Label = "V6";
        ADC7Label = "V7";
        ADC8Label = "V8";

        ADC1Name = "v1";
        ADC2Name = "v2";
        ADC3Name = "v3";
        ADC4Name = "v4";
        ADC5Name = "v5";
        ADC6Name = "v6";
        ADC7Name = "v7";
        ADC8Name = "v8";   

        ADC1Scaler = 1.0;
        ADC2Scaler = 1.0;
        ADC3Scaler = 1.0;
        ADC4Scaler = 1.0;
        ADC5Scaler = 1.0;
        ADC6Scaler = 1.0;
        ADC7Scaler = 1.0;
        ADC8Scaler = 1.0;

        GPIO1Config = GPIO_CONFIG_NONE;
        GPIO2Config = GPIO_CONFIG_NONE;
        GPIO3Config = GPIO_CONFIG_NONE;
        GPIO4Config = GPIO_CONFIG_NONE;
        GPIO5Config = GPIO_CONFIG_NONE;
        GPIO6Config = GPIO_CONFIG_NONE;
        GPIO7Config = GPIO_CONFIG_NONE;
        GPIO8Config = GPIO_CONFIG_NONE;

        GPIO1Label = "IO1";
        GPIO2Label = "IO2";
        GPIO3Label = "IO3";
        GPIO4Label = "IO4";
        GPIO5Label = "IO5";
        GPIO6Label = "IO6";
        GPIO7Label = "IO7";
        GPIO8Label = "IO8";

        GPIO1Name = "io1";
        GPIO2Name = "io2";
        GPIO3Name = "io3";
        GPIO4Name = "io4";
        GPIO5Name = "io5";
        GPIO6Name = "io6";
        GPIO7Name = "io7";
        GPIO8Name = "io8";
 
        GPIO1Scaler = 1.0;
        GPIO2Scaler = 1.0;
        GPIO3Scaler = 1.0;
        GPIO4Scaler = 1.0;
        GPIO5Scaler = 1.0;
        GPIO6Scaler = 1.0;
        GPIO7Scaler = 1.0;
        GPIO8Scaler = 1.0;
    }
};

#endif