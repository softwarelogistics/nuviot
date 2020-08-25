#ifndef HAL_C
#define HAL_C

#include <Arduino.h>

#ifdef __cplusplus
extern "C" {
#endif
uint8_t temprature_sens_read();
#ifdef __cplusplus
}
#endif
uint8_t temprature_sens_read();

class Hal {
    public:
        void restart();        
};

#endif
