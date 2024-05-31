#include "CANBus.h"
#include "driver/twai.h"
#include "driver/gpio.h"

CANBus::CANBus(Console *pConsole, ConfigPins *pConfigPins, BLE *pBle)
{
  m_pConsole = pConsole;
  m_pConfigPins = pConfigPins;
  m_pBle = pBle;
}

bool CANBus::readStatus()
{
  twai_status_info_t status;
  ESP_ERROR_CHECK(twai_get_status_info(&status));

  if (status.state == TWAI_STATE_BUS_OFF)
  {
    m_pConsole->println("TWAI_STATE_BUS_OFF: Tx err:" + String(status.tx_error_counter) + " Rx err:" + String(status.rx_error_counter));
    ESP_ERROR_CHECK_WITHOUT_ABORT(twai_initiate_recovery());
    return false;
  }

  if (status.state == TWAI_STATE_STOPPED)
  {
    m_pConsole->println("TWAI_STATE_STOPPED");
    ESP_ERROR_CHECK_WITHOUT_ABORT(twai_start());
    return false;
  }

  if (status.state == TWAI_STATE_RECOVERING)
  {
    m_pConsole->println("TWAI_STATE_RECOVERING");
    return false;
  }

  if(status.rx_error_counter > 50 || status.tx_error_counter > 50)
    m_pConsole->printError("STATE_OK Tx err:" + String(status.tx_error_counter) + " Rx err:" + String(status.rx_error_counter));

  return true;
}

void CANBus::setup()
{
  twai_general_config_t g_config = TWAI_GENERAL_CONFIG_DEFAULT((gpio_num_t)(m_pConfigPins->CANTx), (gpio_num_t)m_pConfigPins->CANRx, TWAI_MODE_NORMAL);
  twai_timing_config_t t_config = TWAI_TIMING_CONFIG_250KBITS();
  twai_filter_config_t f_config = TWAI_FILTER_CONFIG_ACCEPT_ALL();

  ESP_ERROR_CHECK(twai_driver_install(&g_config, &t_config, &f_config));
  ESP_ERROR_CHECK(twai_start());

  uint32_t alerts_to_enable = TWAI_ALERT_ERR_PASS | TWAI_ALERT_BUS_OFF | TWAI_ALERT_TX_FAILED;
  ESP_ERROR_CHECK(twai_reconfigure_alerts(alerts_to_enable, NULL));
  m_pConsole->println("canbus=initialized;");
}

void CANBus::send(uint32_t messageId, uint8_t *data, uint8_t len)
{
  if (readStatus())
  {
    twai_message_t message;
    message.identifier = messageId; 
    message.data_length_code = len;
    message.extd = 1;

    for (uint8_t j = 1; j < len; ++j)
      message.data[j] = data[j];

    ESP_ERROR_CHECK_WITHOUT_ABORT(twai_transmit(&message, pdMS_TO_TICKS(1000)));
  }
}

void CANBus::loop()
{
  twai_message_t message;
  esp_err_t err = twai_receive(&message, pdMS_TO_TICKS(1000));
  if (err == ESP_OK)
  {
    if (!(message.rtr))
    {
      CANMessage *pMessage;

      if(pHead == NULL) {
        pMessage = new CANMessage(message.identifier);
        pMessage->msgLen = message.data_length_code;        
        pHead = pMessage;    
        pTail = pMessage;
        pTail->transmitted = false;
        memcpy(pTail->msg, message.data, message.data_length_code);
      } else {
        pMessage = pHead;
        while(pMessage != NULL) {
          if(pMessage->getMessageId() == message.identifier) {            
            break;
          }          

          pMessage = pMessage->pNext;
        }
        
        if(pMessage == NULL) {
          CANMessage *msg = new CANMessage(message.identifier);
          pTail->pNext = msg;
          pTail = msg;
          pTail->transmitted = false;
          pTail->pNext = NULL;
          memcpy(pTail->msg, message.data, message.data_length_code);
          pTail->msgLen = message.data_length_code;          
          m_pConsole->println("Added new message: " + pTail->getHEXMessageId());
        }
        else {
          memcpy(pTail->msg,message.data, message.data_length_code);
          pMessage->msgLen = message.data_length_code;
          pTail->transmitted = false;
        }
      }
      
      m_pBle->writeCANMessage(message.identifier, message.data, message.data_length_code);
    }
  }
  else if(err != ESP_ERR_TIMEOUT)
  {
    m_pConsole->print("[CANBus__loop] - twai_receive() failed");
    ESP_ERROR_CHECK_WITHOUT_ABORT(err);    
  }
}