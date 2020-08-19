#ifndef GPSDATA_H
#define GPSDATA_H

#include <Arduino.h>
#include "Console.h"
#include <ArduinoJson.h>


/* strtok_fixed - fixed variation of strtok_single */
static char *strtok_single(char *str, char const *delims)
{
    static char *src = NULL;
    char *p, *ret = 0;

    if (str != NULL)
        src = str;

    if (src == NULL || *src == '\0') // Fix 1
        return NULL;

    ret = src; // Fix 2
    if ((p = strpbrk(src, delims)) != NULL)
    {
        *p = 0;
        src = ++p;
    }
    else
        src += strlen(src);

    return ret;
}

class GPSData
{
public:
    char frame[200];
    char GNSSrunstatus[2];
    char Fixstatus[2];
    char UTCdatetime[19];
    char latitude[11];
    char logitude[12];
    char altitude[9];
    char speedOTG[7];
    char course[7];
    char fixmode[2];
    char HDOP[5];
    char PDOP[5];
    char VDOP[5];
    char satellitesinview[3];
    char GNSSsatellitesused[3];
    char GLONASSsatellitesused[3];
    char cn0max[3];
    char HPA[7];
    char VPA[7];

    void parse(String gnsinf)
    {
        //   GNSINF: 1,1,20200817115814.000,28.076309,-82.709691,4.600,  0.00,142.4,1,,0.8,1.2,0.9,,11,10,,,40,,
        // +CGNSINF: 1,1,20160501124254.000,47.199897,  9.442750,473.500,0.35, 36.8,1,,1.1,1.9,1.6,,13, 7,,,39,,

        gnsinf.toCharArray(frame, 200, 0);

    
        // Parses the string
        strtok_single(frame, " ");
        
        strcpy(GNSSrunstatus, strtok_single(NULL, ",")); // Gets GNSSrunstatus
        strcpy(Fixstatus, strtok_single(NULL, ","));     // Gets Fix status
        strcpy(UTCdatetime, strtok_single(NULL, ","));   // Gets UTC date and time
        strcpy(latitude, strtok_single(NULL, ","));      // Gets latitude
        strcpy(logitude, strtok_single(NULL, ","));      // Gets longi}tude
        strcpy(altitude, strtok_single(NULL, ","));      // Gets MSL altitude
        strcpy(speedOTG, strtok_single(NULL, ","));      // Gets speed over ground
        strcpy(course, strtok_single(NULL, ","));        // Gets course over ground
        strcpy(fixmode, strtok_single(NULL, ","));       // Gets Fix Mode
        strtok_single(NULL, ",");
        strcpy(HDOP, strtok_single(NULL, ",")); // Gets HDO
        strcpy(PDOP, strtok_single(NULL, ",")); // Gets PDOP
        strcpy(VDOP, strtok_single(NULL, ",")); // Gets VDOP
        strtok_single(NULL, ",");
        strcpy(satellitesinview, strtok_single(NULL, ","));      // Gets GNSS Satellites in View
        strcpy(GNSSsatellitesused, strtok_single(NULL, ","));    // Gets GNSS Satellites used
        strcpy(GLONASSsatellitesused, strtok_single(NULL, ",")); // Gets GLONASS Satellites used
        strtok_single(NULL, ",");
        strcpy(cn0max, strtok_single(NULL, ",")); // Gets C/N0 max
        strcpy(HPA, strtok_single(NULL, ","));    // Gets HPA
        strcpy(VPA, strtok_single(NULL, "\r"));   // Gets VPA
    }


    String getJSON() 
    {
        if(String(Fixstatus) != "1")
        {
            const size_t capacity = JSON_OBJECT_SIZE(1);
            DynamicJsonDocument doc(capacity);

            doc["hasFix"] = false;
           
            String output;
            serializeJson(doc, output);

            return output;
        }
        else {
            const size_t capacity = JSON_OBJECT_SIZE(12);
            DynamicJsonDocument doc(capacity);

            doc["hasFix"] = true;
            doc["fixType"] = atoi(fixmode);
           
            doc["lat"] = atof(latitude);
            doc["lon"] = atof(logitude);
            doc["alt"] = atof(altitude);
            doc["cog"] = atof(course);
            doc["sog"] = atof(speedOTG);
            doc["satView"] = atoi(satellitesinview);
            doc["satUsed"] = atoi(GNSSsatellitesused);
            doc["pdop"] = atof(PDOP);
            doc["hdop"] = atof(HDOP);
            doc["vdop"] = atof(VDOP);
            
            String output;
            serializeJson(doc, output);

            return output;
        }
    }

    void debugPrint(Console *console)
    {
        console->print("GNSSrunstatus  : ");
        console->println(GNSSrunstatus);
        console->print("Fixstatus      : ");
        console->println(Fixstatus);
        console->print("UTCdatetime    : ");
        console->println(UTCdatetime);
        console->print("latitude       : ");
        console->println(latitude);
        console->print("logitude       : ");
        console->println(logitude);
        console->print("altitude       : ");
        console->println(altitude);
        console->print("speedOTG       : ");
        console->println(speedOTG);
        console->print("course         : ");
        console->println(course);
        console->print("fixmode        : ");
        console->println(fixmode);
        console->print("HDOP           : ");
        console->println(HDOP);
        console->print("PDOP           : ");
        console->println(PDOP);
        console->print("VDOP           : ");
        console->println(VDOP);
        console->print("Sat In View    : ");
        console->println(satellitesinview);
        console->print("GNSS Sat Used  : ");
        console->println(GNSSsatellitesused);
        console->print("GLONAS Sat Used: ");
        console->println(GLONASSsatellitesused);
        console->print("cn0max         : ");
        console->println(cn0max);
        console->print("HPA            : ");
        console->println(HPA);
        console->print("VPA            : ");
        console->println(VPA);
    }
};

#endif