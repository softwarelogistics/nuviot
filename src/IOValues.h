#ifndef IOVALUES_H
#define IOVALUES_H

#include <Arduino.h>
#include "Console.h"

#define PORT_COUNT 16

class IOValues 
{


private:
    Console *m_pConsole;
    String Values[PORT_COUNT];

public:
    IOValues(Console *console) {
        m_pConsole = console;
        for(int idx = 0; idx < PORT_COUNT; ++idx){
            Values[idx] = "";
        }
    }
    
    String toString() {
        String status;
        for(int idx = 0; idx < PORT_COUNT; ++idx)
        {
            status += idx > 0 ? "," + Values[idx] : Values[idx];
        }        

        return status;
    }

    void debugPrint() {
        m_pConsole->printVerbose(toString());
    }

    void setValue(byte idx, String value){
        if(idx < PORT_COUNT)
            Values[idx] = "\"" + value + "\"";
    }

    void setValue(byte idx, double value){
        if(idx < PORT_COUNT)
            Values[idx] = String(value);
    }

    void setValue(byte idx, int value){
        if(idx < PORT_COUNT)
            Values[idx] = String(value);
    }

    void setIOValue(byte idx, String value){
        if(idx < PORT_COUNT)
            Values[idx] = "\"" + value + "\"";
    }

    void setIOValue(byte idx, double value){
        if(idx < PORT_COUNT)
            Values[idx] = String(value);
    }

    void setIOValue(byte idx, int value){
        if(idx < PORT_COUNT)
            Values[idx] = String(value);
    }

    void setADCValue(byte idx, String value){
        idx += 8;
        
        if(idx < PORT_COUNT)
            Values[idx] = "\"" + value + "\"";
    }

    void setADCValue(byte idx, double value){
        idx += 8;

        if(idx < PORT_COUNT)
            Values[idx] = String(value);
    }

    void setADCValue(byte idx, int value){
        idx += 8;
        if(idx < PORT_COUNT)
            Values[idx] = String(value);
    }

    String getValue(byte idx){
        if(idx < PORT_COUNT)
            return Values[idx];

        return "-1";
    }

    String getIOValue(byte idx) {
        return getValue(idx);
    }

    String getADCValue(byte idx) {
        return getValue(idx + 8);
    }

    double getIODoubleValue(byte idx) {
        return getDoubleValue(idx);
    }

    double getADCDoubleValue(byte idx) {
        return getDoubleValue(idx + 8);
    }

    double getDoubleValue(byte idx) {
        if(idx < PORT_COUNT){
            return atof(Values[idx].c_str());
        }

        return -1;
    }

    void clearValue(byte idx){
        if(idx < PORT_COUNT)
            Values[idx] = "";
    }
};

#endif