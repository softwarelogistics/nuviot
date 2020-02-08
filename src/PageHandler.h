#ifndef PageHandler_h
#define PageHandler_h

#include <WebServer.h>

class PageHandler {
    private:
        WebServer *m_webServer;

    public:
        PageHandler(WebServer *webServer);
        void setup();
        void loop();
};

#endif