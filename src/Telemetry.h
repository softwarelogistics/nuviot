#ifndef TELEMETRY_H
#define TELEMETRY_H

#include <Arduino.h>
#include <Stream.h>

class Telemetry
{
private:
    Stream *m_stream;

public:
    Telemetry(Stream *stream)
    {
        m_stream = stream;
    }

    void SendParameter(String name, double value)
    {
        m_stream->println(name + "=" + String(value));

    }

    void SendParameter(String name, int value)
    {
        m_stream->println(name + "=" + String(value));
    }

    void SendParameter(String name, String value)
    {
        m_stream->println(name + "=" + value);
    }
};

#endif