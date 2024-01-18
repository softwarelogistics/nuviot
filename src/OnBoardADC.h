#ifndef ONBOARD_ADC_H
#define ONBOARD_ADC_H

#include "Console.h"
#include "ConfigPins.h"
#include "IOConfig.h"

class OnBoardADC {
    private:
        Console *m_pConsole;
        ConfigPins *m_pConfigPins;

    public:
        OnBoardADC(Console *console, ConfigPins *configPins) {
            m_pConsole = console;
            m_pConfigPins = configPins;
        }

        void configure(IoConfig *ioConfig);
        void setup();
        void loop();
        void debugPrint();
};

#endif