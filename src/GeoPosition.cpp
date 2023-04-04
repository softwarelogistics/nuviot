#include "GeoPosition.h"
#include <math.h>

// Earth's mean radius in meters
#define EARTHRADIUS 6371000.0

GeoPosition::GeoPosition(): _R(EARTHRADIUS), _latitude(0.0), _longitude(0.0)
{
}

GeoPosition::GeoPosition(double latitude, double longitude): _R(EARTHRADIUS), _latitude(latitude), _longitude(longitude)
{
}
    
/*
double GeoPosition::easting()
{
}

double GeoPosition::northing()
{
}
*/

double GeoPosition::latitude()
{
    return _latitude;
}

double GeoPosition::longitude()
{
    return _longitude;
}

void GeoPosition::set(double latitude, double longitude)
{
    _latitude = latitude;
    _longitude = longitude;
}

void GeoPosition::set(GeoPosition pos)
{
    _latitude = pos.latitude();
    _longitude = pos.longitude();
}

/*
void GeoPosition::set(UTM coord)
{
}
*/

void GeoPosition::move(float course, float distance)
{
    double d = distance / _R;
    double c = radians(course);
    double rlat1 = radians(_latitude);
    double rlon1 = radians(_longitude);

    double rlat2 = asin(sin(rlat1)*cos(d) + cos(rlat1)*sin(d)*cos(c));
    double rlon2 = rlon1 + atan2(sin(c)*sin(d)*cos(rlat1), cos(d)-sin(rlat1)*sin(rlat2));

    _latitude  = degrees(rlat2);
    _longitude = degrees(rlon2);

    // bring back within the range -180 to +180
    while (_longitude < -180.0) _longitude += 360.0;
    while (_longitude > 180.0) _longitude -= 360.0;
}

/*
void GeoPosition::move(Direction toWhere)
{
}

Direction GeoPosition::direction(GeoPosition startingPoint)
{
}
*/

float GeoPosition::bearing(GeoPosition from)
{
    double lat1 = radians(from.latitude());
    double lon1 = radians(from.longitude());
    double lat2 = radians(_latitude);
    double lon2 = radians(_longitude);
    double dLon = lon2 - lon1;

    double y = sin(dLon) * cos(lat2);
    double x = cos(lat1) * sin(lat2) - sin(lat1) * cos(lat2) * cos(dLon);

    return degrees(atan2(y, x)); 
}

/*
float GeoPosition::bearing(LatLong startingPoint)
{
}

float GeoPosition::bearing(UTM startingPoint)
{
}
*/

float GeoPosition::distance(GeoPosition from)
{
    double lat1 = radians(from.latitude());
    double lon1 = radians(from.longitude());
    double lat2 = radians(_latitude);
    double lon2 = radians(_longitude);
    double dLat = lat2 - lat1;
    double dLon = lon2 - lon1;

    double a = sin(dLat/2.0) * sin(dLat/2.0) + 
               cos(lat1) * cos(lat2) *
               sin(dLon/2.0) * sin(dLon/2.0);
    double c = 2.0 * atan2(sqrt(a), sqrt(1-a));
    
    return _R * c;
}

/*
float GeoPosition::distance(LatLong startingPoint)
{
}

float GeoPosition::distance(UTM startingPoint)
{
}
*/

void GeoPosition::setTimestamp(int time) {
    _time = time;
}

int GeoPosition::getTimestamp(void) {
    return _time;
}


    