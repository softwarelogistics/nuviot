#include "MessagePayload.h"

#include <Arduino.h>

String MessagePayload::getJSON()
{
    const size_t capacity = JSON_OBJECT_SIZE(14);
    DynamicJsonDocument doc(capacity);

    String output;
    serializeJson(doc, output);

    return output;
}