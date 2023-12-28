#ifndef CANBUS_H
#define CANBUS_H

#include <Arduino.h>

class CANBus {
    void send(uint32_t messageId, uint8_t data[8]);
};

#endif