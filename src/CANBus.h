#ifndef CANBUS_H
#define CANBUS_H

#include <Arduino.h>
#include "Console.h"
#include "ConfigPins.h"
#include "BLE.h"

class CANMessage {
private:
    uint32_t m_messageId;

public:    
    CANMessage(uint32_t messageId) {
        pNext = NULL;
        m_messageId = messageId;
    }
    
    uint32_t getMessageId() { return m_messageId; }

    String getHEXMessageId() {
        char hexChar[16];
        sprintf(hexChar, "%08X", m_messageId);
        return String(hexChar);
    }

    uint8_t msg[16];
    uint8_t msgLen;
    uint8_t transmitted = 0;

    CANMessage *pNext = NULL;
};

class CANBus {
private:
    CANMessage *pTail = NULL;
    CANMessage *pHead = NULL;
    Console *m_pConsole;
    ConfigPins *m_pConfigPins;
    BLE *m_pBle;

public:    
    CANMessage* getMessageHead() {return pHead;}

    CANBus(Console *pConsole, ConfigPins *pConfigPins, BLE *m_pBle);

    bool readStatus();
    void setup();
    void send(uint32_t messageId, uint8_t *data, uint8_t len);
    void loop();
};

#endif