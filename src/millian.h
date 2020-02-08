#ifndef MILLIAN_H
#define MILLIAN_H

typedef struct 
{
    std::string timeStamp;
    float ambientTemperature;
    float compressorCurrent;
    float blowerCurrent;
    int16_t highPressureRaw;
    double highPressure;
    int16_t lowPressureRaw;
    double lowPressure;
    float ambientHumidity;
    float highTemperature;
    float lowTemperature;
    char panWater[3];
} ma_status;

#endif