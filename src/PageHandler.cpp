#include "PageHandler.h"

#define LOGIN_PAGE "\
<html> \
    <header>\
        <script src = \"https://code.jquery.com/jquery-3.1.1.min.js\" integrity = \"sha256-hVVnYaiADRTO2PzUGmuLJr8BLUSjGIZsDYGmIJLv2b8=\" crossorigin = \"anonymous\"></script> \
        <link href=\"https://maxcdn.bootstrapcdn.com/bootstrap/3.3.7/css/bootstrap.min.css\" rel=\"stylesheet\" integrity=\"sha384-BVYiiSIFeK1dGmJRAkycuHAHRg32OmUcww7on3RYdg4Va+PmSTsz/K68vbdEjh4u\" crossorigin=\"anonymous\"> \
        <script src = \"https://maxcdn.bootstrapcdn.com/bootstrap/3.3.7/js/bootstrap.min.js\" integrity = \"sha384-Tc5IQib027qvyjSMfHjOMaLkfuWVxZxUPnCJA7l2mCWNIpG9mGCD8wGNIcPD7Txa\" crossorigin = \"anonymous\"></script> \
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
        m_webServer->sendHeader("Connection", "close");
        m_webServer->send(200, "text/html", LOGIN_PAGE);
    });

    m_webServer->on("/sensor", HTTP_GET, [this]() {
        m_webServer->sendHeader("Connection", "close");
        m_webServer->send(200, "text/html", LOGIN_PAGE);
    });

    m_webServer->begin();
}

void PageHandler::loop()
{
    m_webServer->handleClient();
}