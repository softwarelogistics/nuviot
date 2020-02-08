#include "PageHandler.h"

#define LOGIN_PAGE "\
<html> \
    <header>\
     <link href=\"https://stackpath.bootstrapcdn.com/bootstrap/4.4.1/css/bootstrap.min.css\" rel=\"stylesheet\" integrity=\"sha384-Vkoo8x4CGsO3+Hhxv8T/Q5PaXtkKtu6ug5TOeNV6gBiFeWPGFN9MuhOf23Q9Ifjh\" crossorigin=\"anonymous\">\
    <header>\
    <body>\
        <img src=\"https://www.nuviot.com/images/applogo.png\" /> \
        <h1> Temperate Sensor Module </h1> \
        <p>This is sent out </p> \
    </body>\
    <script src='https://ajax.googleapis.com/ajax/libs/jquery/3.2.1/jquery.min.js'></script>\
    <script src='https://stackpath.bootstrapcdn.com/bootstrap/4.4.1/js/bootstrap.bundle.min.js'></script>\
</html>"

PageHandler::PageHandler(WebServer *webServer)
{
    m_webServer = webServer;
}

void PageHandler::setup()
{
    m_webServer->on("/", HTTP_GET, [this]() {
        Serial.println("got first.");
        m_webServer->sendHeader("Connection", "close");
        m_webServer->send(200, "text/html", LOGIN_PAGE);
    });

    m_webServer->on("/sensor", HTTP_GET, [this]() {
        Serial.println("got second.");
        m_webServer->sendHeader("Connection", "close");
        m_webServer->send(200, "text/html", LOGIN_PAGE);
    });

    m_webServer->begin();
}

void PageHandler::loop()
{
    m_webServer->handleClient();
}