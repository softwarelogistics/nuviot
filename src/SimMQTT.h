#ifndef SIMMQTT_H
#define SIMMQTT_H

#include <arduino.h>
#include <Display.h>

#define QOS0 0
#define QOS1 1
#define QOS2 2

#define TX_BUFFER_SIZE 1024
#define RX_BUFFER_SIZE 2048

class SimMQTT
{
private:
    HardwareSerial *m_serial;
    Display *m_display;
    String m_apn;
    String m_apnUid;
    String m_apnPwd;
    String m_lastError;
    int m_cmdIdx;
    int m_subscriptionId = 1;
    int m_packetId = 1;
    bool m_verbose = false;

    void (*callback)(String status);
    
    byte m_tempBuffer[2048];
   
    byte m_txBuffer[TX_BUFFER_SIZE];
    byte m_rxBuffer[RX_BUFFER_SIZE];

    int m_txHead = 0;
    int m_txTail = 0;

    int m_rxHead = 0;
    int m_rxTail = 0;

    bool write_it_all = false;

private:
    String readStringUntil(char c, int timeout);
    size_t readBytes(uint8_t *buffer, size_t length);
    void WriteString(String str, boolean trace);
    void WriteLengthPrefixedString(String str, boolean trace);

    void DebugPrint(boolean trace, String msg);
    void PrintByteArray(boolean trace, String prefix, byte buffer[], int len);
    void PrintByteArray(boolean trace, byte array[], int len);
    void PrintByteArray(boolean trace, String prefix, byte buffer[]);
    void PrintByteArray(boolean trace, byte array[]);    
    void PrintByte(byte ch);
    boolean ReadByteArray(boolean trace, byte array[], int len, long timeOutMS);
    boolean ReadByteArray(boolean trace, byte array[], int len);
   
    void WriteByteArray(byte array[], int len, boolean trace);
    String SendCommand(String cmd, String expectedReply, unsigned long delayMS, long timeout, boolean returnAny, boolean trace);
    String SendCommand(String cmd, boolean trace = false);
    boolean WaitForReply(String expectedReply, int iterations, boolean trace);
    boolean HandleByteArray(unsigned char *buffer, unsigned int bufferSize, boolean trace);
    void EnableErrorMessages(boolean trace);
    boolean GetCREG(boolean trace);
    boolean GetCGREG(boolean trace);
    boolean SetLTE();
    boolean DisconnectIP();
    boolean ConnectGPRS(boolean trace);

    boolean ReceivePublishedMessage(unsigned char *buffer, unsigned int bufferSize, boolean trace);

    void EnqueueByte(uint8_t byte);
    void EneuqueByteArray(uint8_t buffer[], int len);
    void Flush();

    void ResetModemBuffer();
    bool SetNBIoTMode(boolean trace);
    bool SetLTE(boolean trace);
    bool SetAPN(boolean trace);
    bool SetBand(boolean trace);
    bool DisconnectIP(boolean trace);
    bool IsString(byte buffer[], int len);
    boolean SendBuffer(boolean truce);
    void WriteRemainingLength(int realLeangth, boolean trace);
    void WriteControlField(byte packetId, boolean trace);
    boolean ReadMQTTResponse(uint8_t expected, bool trace);
 
public:
    SimMQTT(HardwareSerial *serial, Display *display);
   

    boolean Init(bool trace);

    boolean IsSIM800Online(boolean trace);
    String GetSIMId(boolean trace);
    String GetNetwork(boolean trace);
    String GetIPAddress(boolean trace);
    int GetSignalQuality(boolean trace);
    void SetStatusUpdateCallback(void (*callback)(String status));
    void SetVerboseLogging(boolean enabled);
  
    bool ConnectMQTT(String site, String uid, String pwd, String clientId, boolean trace);
    bool ConnectMQTT(String site, String uid, String pwd, boolean trace);
    bool ConnectMQTT(String site, String clientId, boolean trace);
    bool ConnectMQTT(String site, boolean trace);
    boolean IsServiceStarted(boolean trace);
    boolean Publish(String topic, String payload, byte qos, boolean trace);
    boolean Publish(String topic, byte qos, boolean trace);
    boolean Ping(boolean trace);
    int Subscribe(String topic, boolean trace);
    boolean Connect(String apn, String apnId, String apnPwd, boolean trace);   
    boolean Loop(boolean trace);
    boolean Reset(boolean trace);

    String GetLastError();
};

#endif
