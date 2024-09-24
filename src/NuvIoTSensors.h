

#include "TemperatureProbes.h"
#ifndef NUVIOT_SENSORS_H
#define NUVIOT_SENSORS_H




void determineSensorConfiguration(IOConfig *ioConfig, Console *console, byte io1Pin, byte io2Pin)
{ 
  NuvIoT_DHT *dht;
  DallasTemperature *probe1;
  DallasTemperature *probe2;
  
  bool hasDHT22 = false;
  bool hasProbe1 = false;
  bool hasProbe2 = false;

  dht = new NuvIoT_DHT(io1Pin, DHT22, 6, console);
  dht->begin();
  uint8_t retryCount = 0;
  while (retryCount++ < 5 && !hasDHT22)
  {
    float temp = dht->readTemperature(true,true);
    if (!isnan(temp)){
      hasDHT22 = true;
      console->println("Found DHT22 on " + String(io1Pin) + ", current temperature: " + String(temp));
    }
    else {
      console->println("Attempt " + String(retryCount));
      delay(1000);
    }
  }

  if (!hasDHT22){
    console->println("Did Not Find DHT22");  
    delete dht;
    dht = NULL;
  }
  else{
    ioConfig->GPIO1Config = GPIO_CONFIG_DHT22;
    ioConfig->GPIO1Name = "Digital Temperature";
    ioConfig->GPIO1Scaler = 1;
    ioConfig->GPIO1Zero = 0;
    ioConfig->GPIO1Calibration = 1;

    ioConfig->GPIO2Config = GPIO_CONFIG_DHT22_HUMIDITY;
    ioConfig->GPIO2Name = "Digital Humidity";
    ioConfig->GPIO2Scaler = 1;
    ioConfig->GPIO2Zero = 0;
    ioConfig->GPIO2Calibration = 1;
  }

  if (dht == NULL){
    probe1 = new DallasTemperature(new OneWire(io1Pin));
    retryCount = 0;
    while (retryCount++ < 5 && !hasProbe1)
    {
      float temp = probe1->getTempFByIndex(0);
      if (!isnan(temp) && temp > -50.60f)
      {
        console->println("actual probe 2 response " + String(temp));
        hasProbe1 = true;
      }
    }

    if (!hasProbe1){
      probe1 = NULL;
      console->println("Does not have DS18B Probe 1");
    }
    else{
      console->println("Has DS18B Probe 1");
      ioConfig->GPIO1Config = GPIO_CONFIG_DBS18;
      if (ioConfig->GPIO1Name == "")
        ioConfig->GPIO1Name = "Digital Temperature - Port 1";

      ioConfig->GPIO1Scaler = 1;
      ioConfig->GPIO1Zero = 0;
      ioConfig->GPIO1Calibration = 1;
    }

    probe2 = new DallasTemperature(new OneWire(io2Pin));
    retryCount = 0;
    while (retryCount++ < 5 && !hasProbe2)
    {
      float temp = probe2->getTempFByIndex(0);
      if (!isnan(temp) && temp > -50.60f)
      {
        console->println("actual probe 2 response " + String(temp));
        hasProbe2 = true;
      }
    }

    if (!hasProbe2)
    {
      probe2 = NULL;
      console->println("Does not have DS18B Probe 2");
    }
    else
    {
      console->println("Has DS18B Probe 2");
      ioConfig->GPIO2Config = GPIO_CONFIG_DBS18;
      if (ioConfig->GPIO2Name == "")
        ioConfig->GPIO2Name = "Digital Temperature - Port 2";

      ioConfig->GPIO2Scaler = 1;
      ioConfig->GPIO2Zero = 0;
      ioConfig->GPIO2Calibration = 1;
    }

    ioConfig->ADC1Config = ADC_CONFIG_ADC;
    ioConfig->ADC1Name = "Analog Temperature";
    ioConfig->ADC1Scaler = 100;
    ioConfig->ADC1Zero = 32;
    ioConfig->ADC1Calibration = 1;    
  }
  else {
    delete dht;
    dht = NULL;
  }
}

#endif