/*
Arduino driver for Benewake TFMini time-of-flight distance sensor. 
by Peter Jansen (December 11/2017)
This code is open source software in the public domain.

THIS SOFTWARE IS PROVIDED ''AS IS'' AND ANY
EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL THE AUTHOR(S) BE LIABLE FOR ANY
DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.  

The names of the contributors may not be used to endorse or promote products
derived from this software without specific prior written permission.
*/

#include "TFMini.h"

// Constructor
TFMini::TFMini() { 
  // Empty constructor
}


boolean TFMini::begin(HardwareSerial* serial) {
  // Store reference to stream/serial object
  hwdSerial = serial;

  // Clear state
  distance = -1;
  strength = -1;
  state = READY;
  
  // Set standard output mode
  setStandardOutputMode();
  
  return true;
}


// Public: The main function to measure distance. 
uint16_t TFMini::getDistance() {
  int numMeasurementAttempts = 0;
  while (takeMeasurement() != 0) {
    numMeasurementAttempts++;
    if (numMeasurementAttempts > TFMINI_MAX_MEASUREMENT_ATTEMPTS) {
      Serial.println ("TF Mini error: too many measurement attempts");
      Serial.println ("Last error:");
      if (state == ERROR_SERIAL_NOHEADER)     Serial.println("ERROR_SERIAL_NOHEADER");
      if (state == ERROR_SERIAL_BADCHECKSUM)  Serial.println("ERROR_SERIAL_BADCHECKSUM");
      if (state == ERROR_SERIAL_TOOMANYTRIES) Serial.println("ERROR_SERIAL_TOOMANYTRIES");      
      
      state = ERROR_SERIAL_TOOMANYTRIES;
      distance = -1;
      strength = -1;      
      return -1;      
    }
  }

  if (state == MEASUREMENT_OK) {
    return distance;
  } else {
    return -1;
  }
}

// Public: Return the most recent signal strength measuremenet from the TF Mini
uint16_t TFMini::getRecentSignalStrength() {
  return strength;
}


// Private: Set the TF Mini into the correct measurement mode
void TFMini::setStandardOutputMode() {
  // Set to "standard" output mode (this is found in the debug documents)
  hwdSerial->write((uint8_t)0x42);
  hwdSerial->write((uint8_t)0x57);
  hwdSerial->write((uint8_t)0x02);
  hwdSerial->write((uint8_t)0x00);
  hwdSerial->write((uint8_t)0x00);
  hwdSerial->write((uint8_t)0x00);
  hwdSerial->write((uint8_t)0x01);
  hwdSerial->write((uint8_t)0x06);
}

// Set configuration mode
void TFMini::setConfigMode() {
  // advanced parameter configuration mode
  hwdSerial->write((uint8_t)0x42);
  hwdSerial->write((uint8_t)0x57);
  hwdSerial->write((uint8_t)0x02);
  hwdSerial->write((uint8_t)0x00);
  hwdSerial->write((uint8_t)0x00);
  hwdSerial->write((uint8_t)0x00);
  hwdSerial->write((uint8_t)0x01);
  hwdSerial->write((uint8_t)0x02);  
}

// Set single scan mode (external trigger)
void TFMini::setSingleScanMode() {
  setConfigMode();
  // setting trigger source to external
  hwdSerial->write((uint8_t)0x42);
  hwdSerial->write((uint8_t)0x57);
  hwdSerial->write((uint8_t)0x02);
  hwdSerial->write((uint8_t)0x00);
  hwdSerial->write((uint8_t)0x00);
  hwdSerial->write((uint8_t)0x00);
  hwdSerial->write((uint8_t)0x00);
  hwdSerial->write((uint8_t)0x40);
}

// Send external trigger
void TFMini::externalTrigger() {
  setConfigMode();      
  // send trigger
  hwdSerial->write((uint8_t)0x42);
  hwdSerial->write((uint8_t)0x57);
  hwdSerial->write((uint8_t)0x02);
  hwdSerial->write((uint8_t)0x00);
  hwdSerial->write((uint8_t)0x00);
  hwdSerial->write((uint8_t)0x00);
  hwdSerial->write((uint8_t)0x00);
  hwdSerial->write((uint8_t)0x41);
}

// Private: Handles the low-level bits of communicating with the TFMini, and detecting some communication errors.
int TFMini::takeMeasurement() {
  int numCharsRead = 0;
  int attemptCount = 0;
  uint8_t lastChar = 0x00;  
  
  // Step 1: Read the serial stream until we see the beginning of the TF Mini header, or we timeout reading too many characters.
  while (attemptCount++ < 20) {

    if (hwdSerial->available()) {      
      uint8_t curChar = hwdSerial->read();

      if ((lastChar == 0x59) && (curChar == 0x59)) {
        // Break to begin frame
        break;
        
      } else {
        // We have not seen two 0x59's in a row -- store the current character and continue reading.         
        lastChar = curChar;
        numCharsRead += 1; 
      }           
    }
    else {
        Serial.println("No data from hw serial read.");
    }

    // Error detection:  If we read more than X characters without finding a frame header, then it's likely there is an issue with 
    // the Serial connection, and we should timeout and throw an error. 
    if (numCharsRead > TFMINI_MAXBYTESBEFOREHEADER) {
      state = ERROR_SERIAL_NOHEADER;
      distance = -1;
      strength = -1;     
      if (TFMINI_DEBUGMODE == 1) Serial.println("ERROR: no header");
      return -1;      
    }
    
  }
  
  // Step 2: Read one frame from the TFMini
  uint8_t frame[TFMINI_FRAME_SIZE];

  uint8_t checksum = 0x59 + 0x59;
  for (int i=0; i<TFMINI_FRAME_SIZE; i++) {
    // Read one character
    while (!hwdSerial->available()) {
      // wait for a character to become available
    }    
    frame[i] = hwdSerial->read();

    // Store running checksum
    if (i < TFMINI_FRAME_SIZE-2) {
      checksum += frame[i];
    }
  }

  // Step 2A: Compare checksum
  // Last byte in the frame is an 8-bit checksum 
  uint8_t checksumByte = frame[TFMINI_FRAME_SIZE-1];
  if (checksum != checksumByte) {
    state = ERROR_SERIAL_BADCHECKSUM;
    distance = -1;
    strength = -1;
    if (TFMINI_DEBUGMODE == 1) Serial.println("ERROR: bad checksum");
    return -1;
  }

  if(attemptCount == 0) {
    Serial.println("No data from TF MIni");
    return -1;
  }


  // Step 3: Interpret frame
  uint16_t dist = (frame[1] << 8) + frame[0];
  uint16_t st = (frame[3] << 8) + frame[2];
  //uint8_t reserved = frame[4];
  //uint8_t originalSignalQuality = frame[5];


  // Step 4: Store values
  distance = dist;
  strength = st;
  state = MEASUREMENT_OK;

  // Return success
  return 0;  
}

