#ifndef ABSTRACT_SENSOR_H
#define ABSTRACT_SENSOR_H

#include "IOConfig.h"

class AbstractSensor {

    public:
        virtual void setup(IOConfig *ioConfig);
        virtual void loop();
        virtual void debugPrint();

    public:
        bool getIsErrorState() {return _isErrorState;}
        bool getDiagnosticMode() {return _isDiagnostics; }
        void clearLastError() {_isErrorState = false;}


    protected:
        void setIsErrorState(bool err) { _isErrorState = err; }
        void setDiagnosticsMode(bool mode) { _isDiagnostics = mode; }
    
    protected:
        bool _isErrorState;
        bool _isDiagnostics;
};
#endif 
