
#include <Wire.h>
#include "WeatherSensor.h"

#define TEMPERATURE_PRECISION 12

WeatherSensor::WeatherSensor(MessagePayload *payload, NuvIoTState *state)
{
	m_payload = payload;
	m_state = state;
}

void WeatherSensor::setup()
{
	init();
}

void WeatherSensor::loop()
{
	m_isOnline = true;
	Wire.beginTransmission(Si7021_ADDRESS);
	uint8_t error = Wire.endTransmission();
	m_isOnline = error == 0;
	if (m_isOnline)
	{
		m_payload->ambientHumidity = getRH();

		m_payload->ambientTemperature = (getTemp() * 9.0f / 5.0f) + 32;
		m_payload->isWeatherOnline = true;
	}
	else
	{
		Serial.println("Weather sensor is offline.");
		m_payload->isWeatherOnline = false;
		setup();
	}
}

/****************Si7021 & HTU21D Functions**************************************/

float WeatherSensor::getRH()
{
	// Measure the relative humidity
	uint16_t RH_Code = makeMeasurment(HUMD_MEASURE_NOHOLD);
	float result = (125.0 * RH_Code / 65536) - 6;
	return result;
}

float WeatherSensor::readTemp()
{
	// Read temperature from previous RH measurement.
	uint16_t temp_Code = makeMeasurment(TEMP_PREV);
	float result = (175.72 * temp_Code / 65536) - 46.85;
	return result;
}

float WeatherSensor::getTemp()
{
	// Measure temperature
	uint16_t temp_Code = makeMeasurment(TEMP_MEASURE_NOHOLD);
	float result = (175.72 * temp_Code / 65536) - 46.85;
	return result;
}
//Give me temperature in fahrenheit!
float WeatherSensor::readTempF()
{
	return ((readTemp() * 1.8) + 32.0); // Convert celsius to fahrenheit
}

float WeatherSensor::getTempF()
{
	return ((getTemp() * 1.8) + 32.0); // Convert celsius to fahrenheit
}

void WeatherSensor::heaterOn()
{
	// Turns on the ADDRESS heater
	uint8_t regVal = readReg();
	regVal |= _BV(HTRE);
	//turn on the heater
	writeReg(regVal);
}

void WeatherSensor::heaterOff()
{
	// Turns off the ADDRESS heater
	uint8_t regVal = readReg();
	regVal &= ~_BV(HTRE);
	writeReg(regVal);
}

void WeatherSensor::changeResolution(uint8_t i)
{
	// Changes to resolution of ADDRESS measurements.
	// Set i to:
	//      RH         Temp
	// 0: 12 bit       14 bit (default)
	// 1:  8 bit       12 bit
	// 2: 10 bit       13 bit
	// 3: 11 bit       11 bit

	uint8_t regVal = readReg();
	// zero resolution bits
	regVal &= 0b011111110;
	switch (i)
	{
	case 1:
		regVal |= 0b00000001;
		break;
	case 2:
		regVal |= 0b10000000;
		break;
	case 3:
		regVal |= 0b10000001;
	default:
		regVal |= 0b00000000;
		break;
	}
	// write new resolution settings to the register
	writeReg(regVal);
}

void WeatherSensor::init()
{
	Wire.begin();

	uint8_t ID_Temp_Hum = checkID();

	int x = 0;

	if (ID_Temp_Hum == 0x15) //Ping CheckID register
		Serial.println("Si7021 Found");
	else if (ID_Temp_Hum == 0x32)
		Serial.println("HTU21D Found");
	else if (ID_Temp_Hum = 0x14)
		Serial.println("GY-21 Found");		
	else 
		Serial.println("Unknown Temperature Sensor Found.");		
}

void WeatherSensor::reset()
{
	//Reset user resister
	writeReg(SOFT_RESET);
}

uint8_t WeatherSensor::checkID()
{
	uint8_t ID_1;

	// Check device ID
	Wire.beginTransmission(Si7021_ADDRESS);
	Wire.write(0xFC);
	Wire.write(0xC9);
	Wire.endTransmission();

	Wire.requestFrom(Si7021_ADDRESS, 1);

	ID_1 = Wire.read();

	return (ID_1);
}

uint16_t WeatherSensor::makeMeasurment(uint8_t command)
{
	// Take one ADDRESS measurement given by command.
	// It can be either temperature or relative humidity
	// TODO: implement checksum checking

	uint16_t nBytes = 3;
	// if we are only reading old temperature, read olny msb and lsb
	if (command == 0xE0)
		nBytes = 2;

	Wire.beginTransmission(Si7021_ADDRESS);
	Wire.write(command);
	Wire.endTransmission();
	// When not using clock stretching (*_NOHOLD commands) delay here
	// is needed to wait for the measurement.
	// According to datasheet the max. conversion time is ~22ms
	delay(100);

	Wire.requestFrom(Si7021_ADDRESS, nBytes);
	if (Wire.available() != nBytes)
		return 100;

	unsigned int msb = Wire.read();
	unsigned int lsb = Wire.read();
	// Clear the last to bits of LSB to 00.
	// According to datasheet LSB of RH is always xxxxxx10
	lsb &= 0xFC;
	unsigned int mesurment = msb << 8 | lsb;

	return mesurment;
}

void WeatherSensor::writeReg(uint8_t value)
{
	// Write to user register on ADDRESS
	Wire.beginTransmission(Si7021_ADDRESS);
	Wire.write(WRITE_USER_REG);
	Wire.write(value);
	Wire.endTransmission();
}

uint8_t WeatherSensor::readReg()
{
	// Read from user register on ADDRESS
	Wire.beginTransmission(Si7021_ADDRESS);
	Wire.write(READ_USER_REG);
	Wire.endTransmission();
	Wire.requestFrom(Si7021_ADDRESS, 1);
	uint8_t regVal = Wire.read();
	return regVal;
}
