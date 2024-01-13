#include "PulseCounter.h"

#include <Arduino.h>
#include "driver/pcnt.h" // Biblioteca ESP32 PCNT
#include "soc/pcnt_struct.h"

#include <nvs.h>

PulseCounter *__pulseCounter;

#define PCNT_COUNT_CHANNEL PCNT_CHANNEL_0 // Canal 0 do Contador de pulso PCNT do ESP32
#define PCNT_H_LIM_VAL overflow         // Limite superior de contagem

bool flag = true;          // Indicador de fim de contagem - libera impressão
uint32_t overflow = 20000; // Maximum value for PCNT counter overflow
uint32_t count_window        = 1000000;                                  // Tempo de amostragem  de 1 segundo para a contagem de pulsos 999990

uint32_t multPulses = 0;   // Number of PCNT counter overflows
int16_t pulses[8];        // Number of pulses counted
uint64_t pulse_accumulator[8];
// char            buf[32];                                                  // Buffer para guardar a pontuacao



portMUX_TYPE pulseCounterTimerMux = portMUX_INITIALIZER_UNLOCKED; // variavel tipo portMUX_TYPE para sincronismo

void readCounts(void *p) // Fim de tempo de leitura de pulsos
{   
    pcnt_get_counter_value(PCNT_UNIT_0, &pulses[0]);
    pcnt_get_counter_value(PCNT_UNIT_1, &pulses[1]);
    pcnt_get_counter_value(PCNT_UNIT_2, &pulses[2]);

    pulse_accumulator[0] += pulses[0] >> 1; // current pulse count
    pulse_accumulator[1] += pulses[1] >> 1; // current pulse count
    pulse_accumulator[2] += pulses[2] >> 1; // current pulse count

    flag = true;                      // mark available to load
}

static void IRAM_ATTR pcnt_intr_handler(void *arg) 
{
    portENTER_CRITICAL_ISR(&pulseCounterTimerMux); 
/*    pcnt_config_t *prt = (pcnt_config_t*)arg;

    if(prt->unit == PCNT_UNIT_0) 
                                         */

    multPulses++; 

    PCNT.int_clr.val = BIT(PCNT_UNIT_0);       
    portEXIT_CRITICAL_ISR(&pulseCounterTimerMux);  
}

void initialize_counter(pcnt_unit_t unit, int pin)
{
    pcnt_config_t pcnt_config = {};

    pcnt_config.pulse_gpio_num = pin;
    pcnt_config.unit = unit;
    pcnt_config.channel = PCNT_COUNT_CHANNEL;
    pcnt_config.counter_h_lim = PCNT_H_LIM_VAL;
    pcnt_config.pos_mode = PCNT_COUNT_INC;
    pcnt_config.neg_mode = PCNT_COUNT_INC;
    pcnt_config.lctrl_mode = PCNT_MODE_DISABLE;
    pcnt_config.hctrl_mode = PCNT_MODE_KEEP;
    pcnt_unit_config(&pcnt_config);

    pcnt_counter_pause(unit);
    pcnt_counter_clear(unit);

    pcnt_event_enable(unit, PCNT_EVT_H_LIM);
    pcnt_isr_register(pcnt_intr_handler, NULL, 0, NULL);
    pcnt_intr_enable(unit);

    pcnt_counter_resume(unit);
}

PulseCounter::PulseCounter(Console *console, ConfigPins *configPins, MessagePayload *payload)
{
    __pulseCounter = this;
    m_console = console;
    m_configPins = configPins;
    m_payload = payload;
    for (uint8_t idx = 0; idx < 8; ++idx)
        m_portEnabled[idx] = 0;
}

void PulseCounter::registerPin(uint8_t idx, String name, uint8_t pin)
{
    m_names[idx] = name;    
    m_portEnabled[idx] = 1;
}

void PulseCounter::applyValues()
{
    for (int idx = 0; idx < NUMBER_PULSE_COUNTER_CHANNELS; ++idx)
    {
        if (m_portEnabled[idx])
        {
            m_rawValues[idx] = ((m_frequencies[idx] * m_calibration[idx]) - m_zero[idx]) * m_scalers[idx];
            m_payload->ioValues->setValue(idx, m_rawValues[idx]);
        }
        else
        {
            m_rawValues[idx] = -1;
        }
    }
}

esp_timer_create_args_t create_args;   

void PulseCounter::setup(IOConfig *ioConfig)
{
    setScaler(0, ioConfig->GPIO1Scaler);
    setScaler(1, ioConfig->GPIO2Scaler);
    setScaler(2, ioConfig->GPIO3Scaler);
    setScaler(3, ioConfig->GPIO4Scaler);
    setScaler(4, ioConfig->GPIO5Scaler);
    setScaler(5, ioConfig->GPIO6Scaler);
    setScaler(6, ioConfig->GPIO7Scaler);
    setScaler(7, ioConfig->GPIO8Scaler);

    setZero(0, ioConfig->GPIO1Zero);
    setZero(1, ioConfig->GPIO2Zero);
    setZero(2, ioConfig->GPIO3Zero);
    setZero(3, ioConfig->GPIO4Zero);
    setZero(4, ioConfig->GPIO5Zero);
    setZero(5, ioConfig->GPIO6Zero);
    setZero(6, ioConfig->GPIO7Zero);
    setZero(7, ioConfig->GPIO8Zero);

    setCalibration(0, ioConfig->GPIO1Calibration);
    setCalibration(1, ioConfig->GPIO2Calibration);
    setCalibration(2, ioConfig->GPIO3Calibration);
    setCalibration(3, ioConfig->GPIO4Calibration);
    setCalibration(4, ioConfig->GPIO5Calibration);
    setCalibration(5, ioConfig->GPIO6Calibration);
    setCalibration(6, ioConfig->GPIO7Calibration);
    setCalibration(7, ioConfig->GPIO8Calibration);

    ESP_ERROR_CHECK(nvs_open_from_partition("nvs", "pulse_counter", NVS_READWRITE, &m_nvsHandle));
    nvs_get_u64(m_nvsHandle, "channel0", &m_channelCounts[0]);
    nvs_get_u64(m_nvsHandle, "channel1", &m_channelCounts[1]);
    nvs_get_u64(m_nvsHandle, "channel2", &m_channelCounts[2]);
    nvs_get_u64(m_nvsHandle, "channel3", &m_channelCounts[3]);
    nvs_get_u64(m_nvsHandle, "channel4", &m_channelCounts[4]);
    nvs_get_u64(m_nvsHandle, "channel5", &m_channelCounts[5]);
    nvs_get_u64(m_nvsHandle, "channel6", &m_channelCounts[6]);
    nvs_get_u64(m_nvsHandle, "channel7", &m_channelCounts[7]);

    pulse_accumulator[0] = m_channelCounts[0];
    pulse_accumulator[1] = m_channelCounts[1];
    pulse_accumulator[2] = m_channelCounts[2];

    m_console->println("channel0=" + String(m_channelCounts[0]));
    
    initialize_counter(PCNT_UNIT_0, m_configPins->Gpio1);
    initialize_counter(PCNT_UNIT_1, m_configPins->Gpio2);
    initialize_counter(PCNT_UNIT_2, m_configPins->Gpio3);

    create_args.callback = readCounts;    
    esp_timer_create(&create_args, &m_clearCountsTimerHandle);

    configure(ioConfig);
}

void PulseCounter::loop()
{
    uint32_t millisDelta = millis() - m_lastMillis;
    if (millisDelta > SAMPLE_PERIOD)
    {   
        m_lastMillis = millis();

        if (flag == true) 
        {
            flag = false; // Impede nova impressao
            for(uint8_t idx = 0; idx < 3; idx++) {
                m_frequencies[idx] = (pulses[idx] + (multPulses * overflow)) / 2; 
                m_channelCounts[idx] = pulse_accumulator[idx];
            }
                
            ESP_ERROR_CHECK_WITHOUT_ABORT(nvs_set_u64(m_nvsHandle, "channel0", m_channelCounts[0]));
            ESP_ERROR_CHECK_WITHOUT_ABORT(nvs_set_u64(m_nvsHandle, "channel1", m_channelCounts[1]));
            ESP_ERROR_CHECK_WITHOUT_ABORT(nvs_set_u64(m_nvsHandle, "channel2", m_channelCounts[2]));
            ESP_ERROR_CHECK_WITHOUT_ABORT(nvs_set_u64(m_nvsHandle, "channel3", m_channelCounts[3]));
            ESP_ERROR_CHECK_WITHOUT_ABORT(nvs_set_u64(m_nvsHandle, "channel4", m_channelCounts[4]));
            ESP_ERROR_CHECK_WITHOUT_ABORT(nvs_set_u64(m_nvsHandle, "channel5", m_channelCounts[5]));
            ESP_ERROR_CHECK_WITHOUT_ABORT(nvs_set_u64(m_nvsHandle, "channel6", m_channelCounts[6]));
            ESP_ERROR_CHECK_WITHOUT_ABORT(nvs_set_u64(m_nvsHandle, "channel7", m_channelCounts[7]));

            multPulses = 0;

            pcnt_counter_clear(PCNT_UNIT_0);                             
            pcnt_counter_clear(PCNT_UNIT_1);                             
            pcnt_counter_clear(PCNT_UNIT_2);                             

            ESP_ERROR_CHECK_WITHOUT_ABORT(nvs_commit(m_nvsHandle));
            esp_timer_start_once(m_clearCountsTimerHandle, count_window); 
            applyValues();
        }
    }
}

void PulseCounter::configure(IOConfig *config)
{
    if (config->GPIO1Config == GPIO_CONFIG_PULSE_COUNTER || config->GPIO1Config == GPIO_CONFIG_RPM)
        registerPin(0, config->GPIO1Name, m_configPins->Gpio1);
    if (config->GPIO2Config == GPIO_CONFIG_PULSE_COUNTER || config->GPIO2Config == GPIO_CONFIG_RPM)
        registerPin(1, config->GPIO2Name, m_configPins->Gpio2);
    if (config->GPIO3Config == GPIO_CONFIG_PULSE_COUNTER || config->GPIO3Config == GPIO_CONFIG_RPM)
        registerPin(2, config->GPIO3Name, m_configPins->Gpio3);
    if (config->GPIO4Config == GPIO_CONFIG_PULSE_COUNTER || config->GPIO4Config == GPIO_CONFIG_RPM)
        registerPin(3, config->GPIO4Name, m_configPins->Gpio4);
    if (config->GPIO5Config == GPIO_CONFIG_PULSE_COUNTER || config->GPIO5Config == GPIO_CONFIG_RPM)
        registerPin(4, config->GPIO5Name, m_configPins->Gpio5);
    if (config->GPIO6Config == GPIO_CONFIG_PULSE_COUNTER || config->GPIO6Config == GPIO_CONFIG_RPM)
        registerPin(5, config->GPIO6Name, m_configPins->Gpio6);
    if (config->GPIO7Config == GPIO_CONFIG_PULSE_COUNTER || config->GPIO7Config == GPIO_CONFIG_RPM)
        registerPin(6, config->GPIO7Name, m_configPins->Gpio7);
    if (config->GPIO8Config == GPIO_CONFIG_PULSE_COUNTER || config->GPIO8Config == GPIO_CONFIG_RPM)
        registerPin(7, config->GPIO8Name, m_configPins->Gpio8);
}

int PulseCounter::countsPerSecond(uint8_t channel)
{
    return m_frequencies[channel];
}

void PulseCounter::debugPrint()
{    
    for (int idx = 0; idx < NUMBER_PULSE_COUNTER_CHANNELS; ++idx)
    {
        if (m_portEnabled[idx]) {
            m_console->printVerbose(m_names[idx] + ": Freq=" + String(m_frequencies[idx]) + ", Total Counts=" + String(m_channelCounts[idx]) + ", Value=" + String(m_rawValues[idx]));
        }
    }
}
