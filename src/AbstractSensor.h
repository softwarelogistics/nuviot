#ifndef ABSTRACT_SENSOR_H
#define ABSTRACT_SENSOR_H

#include "IOConfig.h"

class AbstractSensor {
    public:
        virtual void setup(IOConfig *ioConfig);
        virtual void loop();
        virtual void debugPrint();
};
#endif 
