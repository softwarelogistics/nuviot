#include "TemperatureProbes.h"

TemperatureProbes::TemperatureProbes(int oneWirePin, MessagePayload *payload, NuvIoTState *state)
{
    m_payload = payload;
    m_pin = oneWirePin;
    m_state = state;
}

void TemperatureProbes::setup()
{
    m_oneWire = new OneWire(m_pin);
    m_probes = new DallasTemperature(m_oneWire);
    m_probes->begin();
    findAddresses();
    /*int deviceCount = m_probes->getDeviceCount();
   for(int idx = 0; idx < deviceCount; ++idx){
       m_probes->getAddress(m_highThermometer, idx);
   }*/
}

void TemperatureProbes::findAddresses()
{
    byte i;
    byte addr[8];

    Serial.println("Getting the address...");
    /* initiate a search for the OneWire object we created and read its value into
    addr array we declared above*/
    m_addressCount = 0;
    while (m_oneWire->search(addr))
    {
        Serial.print("The address is:\t");
        //read each byte in the address array
        for (i = 0; i < 8; i++)
        {
            m_addressBank[m_addressCount][i] = addr[i];
            Serial.print("0x");
            if (addr[i] < 16)
            {
                Serial.print('0');
            }
            // print each byte in the address array in hex format
            Serial.print(addr[i], HEX);
            if (i < 7)
            {
                Serial.print(", ");
            }
        }
        m_addressCount++;
        // a check to make sure that what we read is correct.
        if (OneWire::crc8(addr, 7) != addr[7])
        {
            Serial.print("CRC is not valid!\n");
            return;
        }

        Serial.println(";");
    }

    Serial.println("Found temperature probe count " + String(m_addressCount));
    m_oneWire->reset_search();
}

void TemperatureProbes::loop()
{
    if (m_addressCount == 2)
    {
        m_probes->requestTemperatures();

        // update the device information
        m_payload->lowTemperature = m_probes->getTempF(m_addressBank[0]);
        m_payload->highTemperature = m_probes->getTempF(m_addressBank[1]);
        m_payload->areProbesOnline = true;
    }
    else
    {
        m_payload->areProbesOnline = false;
        m_payload->lowTemperature = 0;
        m_payload->highTemperature = 0;
    }
}