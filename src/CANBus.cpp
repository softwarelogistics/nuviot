#include "CANBus.h"
#include "driver/twai.h"

void CANBus::send(uint32_t messageId, uint8_t data[8]) {
    twai_message_t message;
    message.identifier = messageId;
    message.data_length_code = 8;
    message.extd = 1;

    for (uint8_t j = 1; j < 8; ++j)
      message.data[j] = data[j];

    ESP_ERROR_CHECK_WITHOUT_ABORT(twai_transmit(&message, pdMS_TO_TICKS(1000)));
}