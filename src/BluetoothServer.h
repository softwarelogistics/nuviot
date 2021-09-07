#ifndef BELUTOOTHSERVER_H
#define BELUTOOTHSERVER_H

#include <Arduino.h>


class BluetoothServer {
    public:
        void Setup();
        void Loop();
        void begin(String name);
        void print(String str);
        void println(String str);
        void println();
        int available();
        void flush();
        unsigned char read();
};

#endif