#ifndef ABSTRACT_SENSOR_H
#define ABSTRACT_SENSOR_H
class AbstractSensor {
    public:
        virtual void setup();
        virtual void loop();
        virtual void debugPrint();
};
#endif 
