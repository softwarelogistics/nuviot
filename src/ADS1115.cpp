#include "Arduino.h"

#include <Wire.h>

#include "ADS1115.h"

/**************************************************************************/
/*!
    @brief  Abstract away platform differences in Arduino wire library
*/
/**************************************************************************/
static uint8_t i2cread(TwoWire *wire)
{
  return wire->read();
}

/**************************************************************************/
/*!
    @brief  Abstract away platform differences in Arduino wire library
*/
/**************************************************************************/
static void i2cwrite(TwoWire *wire, uint8_t x)
{
  wire->write((uint8_t)x);
}

/**************************************************************************/
/*!
    @brief  Writes 16-bits to the specified destination register
*/
/**************************************************************************/
static uint8_t writeRegister(TwoWire *wire, uint8_t i2cAddress, uint8_t reg, uint16_t value)
{
  wire->setClock(400000);
  wire->beginTransmission(i2cAddress);
  i2cwrite(wire, (uint8_t)reg);
  i2cwrite(wire, (uint8_t)(value >> 8));
  i2cwrite(wire, (uint8_t)(value & 0xFF));
  return wire->endTransmission();
}

/**************************************************************************/
/*!
    @brief  Writes 16-bits to the specified destination register
*/
/**************************************************************************/
static uint16_t readRegister(TwoWire *wire, uint8_t i2cAddress, uint8_t reg)
{
  wire->setClock(400000);
  wire->beginTransmission(i2cAddress);
  i2cwrite(wire, ADS1115_REG_POINTER_CONVERT);
  wire->endTransmission();
  wire->requestFrom(i2cAddress, (uint8_t)2);
  return ((i2cread(wire) << 8) | i2cread(wire));
}

/**************************************************************************/
/*!
    @brief  Instantiates a new ADS1115 class w/appropriate properties
*/
/**************************************************************************/
ADS1115::ADS1115(uint8_t i2cAddress)
{
  m_i2cAddress = i2cAddress;
  m_conversionDelay = ADS1115_CONVERSIONDELAY;
  m_bitShift = 0;
  m_gain = GAIN_TWOTHIRDS; /* +/- 6.144V range (limited to VDD +0.3V max!) */
}

/**************************************************************************/
/*!
    @brief  Instantiates a new ADS1115 class w/appropriate properties
*/
/**************************************************************************/
ADS1115::ADS1115(TwoWire *twoWire, uint8_t i2cAddress, uint8_t bank, Console *pConsole)
{
  m_wire = twoWire;
  m_bank = bank;
  m_i2cAddress = i2cAddress;
  m_conversionDelay = ADS1115_CONVERSIONDELAY;
  m_bitShift = 0;
  m_gain = GAIN_TWOTHIRDS; /* +/- 6.144V range (limited to VDD +0.3V max!) */
  m_pConsole = pConsole;
}

/**************************************************************************/
/*!
    @brief  Sets up the HW (reads coefficients values, etc.)
*/
/**************************************************************************/
void ADS1115::begin()
{

}

/**************************************************************************/
/*!
    @brief  Sets the gain and input voltage range
*/
/**************************************************************************/
void ADS1115::setGain(adsGain_t gain)
{
  m_gain = gain;
}

/**************************************************************************/
/*!
    @brief  Gets a gain and input voltage range
*/
/**************************************************************************/
adsGain_t ADS1115::getGain()
{
  return m_gain;
}

/**************************************************************************/
/*!
    @brief  Gets a single-ended ADC reading from the specified channel
*/
/**************************************************************************/
uint16_t ADS1115::readADC_SingleEnded(uint8_t channel)
{
  if (channel > 3)
  {
    return 0;
  }

  // Start with default values
  uint16_t config = ADS1115_REG_CONFIG_CQUE_NONE |    // Disable the comparator (default val)
                    ADS1115_REG_CONFIG_CLAT_NONLAT |  // Non-latching (default val)
                    ADS1115_REG_CONFIG_CPOL_ACTVLOW | // Alert/Rdy active low   (default val)
                    ADS1115_REG_CONFIG_CMODE_TRAD |   // Traditional comparator (default val)
                    ADS1115_REG_CONFIG_DR_1600SPS |   // 1600 samples per second (default)
                    ADS1115_REG_CONFIG_MODE_SINGLE;   // Single-shot mode (default)

  // Set PGA/voltage range
  config |= m_gain;

  // Set single-ended input channel
  switch (channel)
  {
  case (0):
    config |= ADS1115_REG_CONFIG_MUX_SINGLE_0;
    break;
  case (1):
    config |= ADS1115_REG_CONFIG_MUX_SINGLE_1;
    break;
  case (2):
    config |= ADS1115_REG_CONFIG_MUX_SINGLE_2;
    break;
  case (3):
    config |= ADS1115_REG_CONFIG_MUX_SINGLE_3;
    break;
  }

  // Set 'start single-conversion' bit
  config |= ADS1115_REG_CONFIG_OS_SINGLE;

  // Write config register to the ADC
  

  // Wait for the conversion to complete
  

  // Read the conversion results
  // Shift 12-bit results right 4 bits for the ADS1115
  uint16_t raw = 0;
  uint8_t retryCount = 0;

  while(retryCount++ < 5 && (raw > 0x8000 || raw < 100)) { 
    uint8_t writeResult = writeRegister(m_wire, m_i2cAddress, ADS1115_REG_POINTER_CONFIG, config);
    delay(m_conversionDelay);
    raw = readRegister(m_wire, m_i2cAddress, ADS1115_REG_POINTER_CONVERT) >> m_bitShift;    
  }

  return raw;
}

/**************************************************************************/
/*! 
    @brief  Reads the conversion results, measuring the voltage
            difference between the P (AIN0) and N (AIN1) input.  Generates
            a signed value since the difference can be either
            positive or negative.
*/
/**************************************************************************/
int16_t ADS1115::readADC_Differential_0_1()
{
  // Start with default values
  uint16_t config = ADS1115_REG_CONFIG_CQUE_NONE |    // Disable the comparator (default val)
                    ADS1115_REG_CONFIG_CLAT_NONLAT |  // Non-latching (default val)
                    ADS1115_REG_CONFIG_CPOL_ACTVLOW | // Alert/Rdy active low   (default val)
                    ADS1115_REG_CONFIG_CMODE_TRAD |   // Traditional comparator (default val)
                    ADS1115_REG_CONFIG_DR_1600SPS |   // 1600 samples per second (default)
                    ADS1115_REG_CONFIG_MODE_SINGLE;   // Single-shot mode (default)

  // Set PGA/voltage range
  config |= m_gain;

  // Set channels
  config |= ADS1115_REG_CONFIG_MUX_DIFF_0_1; // AIN0 = P, AIN1 = N

  // Set 'start single-conversion' bit
  config |= ADS1115_REG_CONFIG_OS_SINGLE;

  // Write config register to the ADC
  writeRegister(m_wire, m_i2cAddress, ADS1115_REG_POINTER_CONFIG, config);

  // Wait for the conversion to complete
  delay(m_conversionDelay);

  // Read the conversion results
  uint16_t res = readRegister(m_wire, m_i2cAddress, ADS1115_REG_POINTER_CONVERT) >> m_bitShift;
  if (m_bitShift == 0)
  {
    return (int16_t)res;
  }
  else
  {
    // Shift 12-bit results right 4 bits for the ADS1115,
    // making sure we keep the sign bit intact
    if (res > 0x07FF)
    {
      // negative number - extend the sign to 16th bit
      res |= 0xF000;
    }
    return (int16_t)res;
  }
}

/**************************************************************************/
/*! 
    @brief  Reads the conversion results, measuring the voltage
            difference between the P (AIN2) and N (AIN3) input.  Generates
            a signed value since the difference can be either
            positive or negative.
*/
/**************************************************************************/
int16_t ADS1115::readADC_Differential_2_3()
{
  // Start with default values
  uint16_t config = ADS1115_REG_CONFIG_CQUE_NONE |    // Disable the comparator (default val)
                    ADS1115_REG_CONFIG_CLAT_NONLAT |  // Non-latching (default val)
                    ADS1115_REG_CONFIG_CPOL_ACTVLOW | // Alert/Rdy active low   (default val)
                    ADS1115_REG_CONFIG_CMODE_TRAD |   // Traditional comparator (default val)
                    ADS1115_REG_CONFIG_DR_1600SPS |   // 1600 samples per second (default)
                    ADS1115_REG_CONFIG_MODE_SINGLE;   // Single-shot mode (default)

  // Set PGA/voltage range
  config |= m_gain;

  // Set channels
  config |= ADS1115_REG_CONFIG_MUX_DIFF_2_3; // AIN2 = P, AIN3 = N

  // Set 'start single-conversion' bit
  config |= ADS1115_REG_CONFIG_OS_SINGLE;

  // Write config register to the ADC
  writeRegister(m_wire, m_i2cAddress, ADS1115_REG_POINTER_CONFIG, config);

  // Wait for the conversion to complete
  delay(m_conversionDelay);

  // Read the conversion results
  uint16_t res = readRegister(m_wire, m_i2cAddress, ADS1115_REG_POINTER_CONVERT) >> m_bitShift;
  if (m_bitShift == 0)
  {
    return (int16_t)res;
  }
  else
  {
    // Shift 12-bit results right 4 bits for the ADS1115,
    // making sure we keep the sign bit intact
    if (res > 0x07FF)
    {
      // negative number - extend the sign to 16th bit
      res |= 0xF000;
    }
    return (int16_t)res;
  }
}

/**************************************************************************/
/*!
    @brief  Sets up the comparator to operate in basic mode, causing the
            ALERT/RDY pin to assert (go from high to low) when the ADC
            value exceeds the specified threshold.

            This will also set the ADC in continuous conversion mode.
*/
/**************************************************************************/
void ADS1115::startComparator_SingleEnded(uint8_t channel, int16_t threshold)
{
  // Start with default values
  uint16_t config = ADS1115_REG_CONFIG_CQUE_1CONV |   // Comparator enabled and asserts on 1 match
                    ADS1115_REG_CONFIG_CLAT_LATCH |   // Latching mode
                    ADS1115_REG_CONFIG_CPOL_ACTVLOW | // Alert/Rdy active low   (default val)
                    ADS1115_REG_CONFIG_CMODE_TRAD |   // Traditional comparator (default val)
                    ADS1115_REG_CONFIG_DR_1600SPS |   // 1600 samples per second (default)
                    ADS1115_REG_CONFIG_MODE_CONTIN |  // Continuous conversion mode
                    ADS1115_REG_CONFIG_MODE_CONTIN;   // Continuous conversion mode

  // Set PGA/voltage range
  config |= m_gain;

  // Set single-ended input channel
  switch (channel)
  {
  case (0):
    config |= ADS1115_REG_CONFIG_MUX_SINGLE_0;
    break;
  case (1):
    config |= ADS1115_REG_CONFIG_MUX_SINGLE_1;
    break;
  case (2):
    config |= ADS1115_REG_CONFIG_MUX_SINGLE_2;
    break;
  case (3):
    config |= ADS1115_REG_CONFIG_MUX_SINGLE_3;
    break;
  }

  // Set the high threshold register
  // Shift 12-bit results left 4 bits for the ADS1115
  writeRegister(m_wire, m_i2cAddress, ADS1115_REG_POINTER_HITHRESH, threshold << m_bitShift);

  // Write config register to the ADC
  writeRegister(m_wire, m_i2cAddress, ADS1115_REG_POINTER_CONFIG, config);
}

/**************************************************************************/
/*!
    @brief  In order to clear the comparator, we need to read the
            conversion results.  This function reads the last conversion
            results without changing the config value.
*/
/**************************************************************************/
int16_t ADS1115::getLastConversionResults()
{
  // Wait for the conversion to complete
  delay(m_conversionDelay);

  // Read the conversion results
  uint16_t res = readRegister(m_wire, m_i2cAddress, ADS1115_REG_POINTER_CONVERT) >> m_bitShift;
  if (m_bitShift == 0)
  {
    return (int16_t)res;
  }
  else
  {
    // Shift 12-bit results right 4 bits for the ADS1115,
    // making sure we keep the sign bit intact
    if (res > 0x07FF)
    {
      // negative number - extend the sign to 16th bit
      res |= 0xF000;
    }
    return (int16_t)res;
  }
}

float ADS1115::readADC_Voltage(uint8_t channel)
{
  uint16_t raw = readADC_SingleEnded(channel);
  float output = 0.0;
  
  if(raw < 0x8000 && raw > 100) {
    output = ( raw / 26789.0f) * 5.0f;    
  }  
  else {
    output = raw;
  }

  return output;
}

bool ADS1115::isOnline() {
  m_wire->beginTransmission(m_i2cAddress);
  switch(m_wire->endTransmission()){
    case 0:
      m_isOnline = true;
      break;
    default:
      m_isOnline = false;
      break;
  }

  return m_isOnline;
}
