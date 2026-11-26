#include "BatterySupport.h"

constexpr int BATTERY_PIN = 0;      // battery voltage divider

constexpr int MAIN_POWER_PIN = 5;   // USB/5V source detect

constexpr int BATTERY_STAT = 10;  


void BatterySupport::setup() {
  pinMode(MAIN_POWER_PIN, INPUT);
  pinMode(BATTERY_STAT, INPUT);
  pinMode(BATTERY_PIN, INPUT);
}

void BatterySupport::loop() {
    m_payload->ioValues->setIOValue(0, isBatteryCharging() ? 1 : 0); 
    m_payload->ioValues->setIOValue(1, isUsbPresent() ? 1 : 0); 
    m_payload->ioValues->setADCValue(0, readBatteryVoltage());
}

bool BatterySupport::hasBatterySupport() {
    return true; /* we need a way to determine if this has a batter*/
}


bool BatterySupport::isUsbPresent() {
  return digitalRead(MAIN_POWER_PIN) == HIGH;
}


bool BatterySupport::isBatteryCharging() {
   return digitalRead(BATTERY_STAT) == LOW;
}


float BatterySupport::readBatteryVoltage() {
  uint32_t sumMv = 0;
  const int samples = 16;

  for (int i = 0; i < samples; i++) {
    sumMv += analogReadMilliVolts(BATTERY_PIN);
    delay(2);
  }
 
  float voltageAtPin = (sumMv / (float)samples) / 1000.0f;

  return voltageAtPin * 2.0f; // 100k/100k divider
}



/*
#include <Arduino.h>
#include <WiFi.h>
#include <esp_wifi.h>
#include <esp_sleep.h>
#include <esp_timer.h>
#include "driver/gpio.h"
#include "Freenove_WS2812_Lib_for_ESP32.h"

#define SERIAL Serial0

constexpr int SW1_PIN = 1;         // switch 1: mode toggle
constexpr int SW2_PIN = 6;         // switch 2: help message
constexpr int BATTERY_PIN = 0;      // battery voltage divider
constexpr int MAIN_POWER_PIN = 5;   // source detect divider
constexpr int BATTERY_STAT = 10;    // charge/full status

// LED settings
constexpr int LEDS_COUNT = 1;
constexpr int LEDS_PIN = 3;
constexpr int CHANNEL = 0;

constexpr uint32_t DEBOUNCE_MS = 35;
constexpr uint64_t DEEP_SLEEP_HEARTBEAT_US = 120ULL * 1000000ULL; // 2 min
constexpr float LOW_BATTERY_WARNING_V = 3.4f;

const char* WIFI_SSID = "2girlswifi";
const char* WIFI_PASS = "2girls123";

uint64_t boot_us = 0;
uint64_t active_us = 0;
uint64_t wifi_start_us = 0;
uint64_t wifi_connected_us = 0;

enum Mode : uint8_t
{
  MODE_ACTIVE = 0,
  MODE_DEEP_SLEEP,
  MODE_COUNT
};

void applyMode(Mode m, uint32_t triggerUs);

// Persist across deep sleep
RTC_DATA_ATTR uint8_t modeRtc = MODE_ACTIVE;
RTC_DATA_ATTR uint32_t bootCount = 0;

// strip object
Freenove_ESP32_WS2812 strip(LEDS_COUNT, LEDS_PIN, CHANNEL);

bool ledBlinkState = false;
uint32_t lastLedMs = 0;

const char* modeName(Mode m)
{
  switch (m)
  {
    case MODE_ACTIVE:     return "ACTIVE";
    case MODE_DEEP_SLEEP: return "DEEP_SLEEP";
    default:              return "UNKNOWN";
  }
}

void setLedColor(uint32_t color)
{
  strip.setLedColorData(0, color);
  strip.show();
}

bool isUsbPresent()
{
  // HIGH = USB/5V present, LOW = battery
  return digitalRead(MAIN_POWER_PIN) == HIGH;
}

bool isBatteryCharging()
{
  // LOW = charging, HIGH = full/not charging
  return digitalRead(BATTERY_STAT) == LOW;
}

float readBatteryVoltage()
{
  int raw = analogRead(BATTERY_PIN);
  float voltageAtPin = raw * (3.3f / 4095.0f);
  float batteryVoltage = voltageAtPin * 2.0f; // 100k/100k divider
  return batteryVoltage;
}

void updateStatusLed()
{
  if (modeRtc == MODE_DEEP_SLEEP)
  {
    setLedColor(0x000000);
  }

  uint32_t baseColor = isUsbPresent() ? 0x00FF00 : 0x0000FF; // green USB, blue battery

  if (WiFi.status() == WL_CONNECTED)
  {
    if (millis() - lastLedMs >= 1000)
    {
      lastLedMs = millis();
      ledBlinkState = !ledBlinkState;
      setLedColor(ledBlinkState ? baseColor : 0x000000);
    }
  }
  else
  {
    ledBlinkState = true;
    setLedColor(baseColor);
  }
}

bool wifiConnect(uint32_t timeoutMs = 15000)
{
  if (WiFi.status() == WL_CONNECTED)
  {
    wifi_connected_us = esp_timer_get_time();
    return true;
  }

  SERIAL.printf("Connecting to WiFi SSID: %s\n", WIFI_SSID);
  SERIAL.flush();

  WiFi.mode(WIFI_STA);
  wifi_start_us = esp_timer_get_time();
  WiFi.begin(WIFI_SSID, WIFI_PASS);

  while (WiFi.status() != WL_CONNECTED &&
         (esp_timer_get_time() - wifi_start_us) < ((uint64_t)timeoutMs * 1000ULL))
  {
    delay(250);
    SERIAL.print(".");
    SERIAL.flush();
  }
  SERIAL.println();

  if (WiFi.status() == WL_CONNECTED)
  {
    wifi_connected_us = esp_timer_get_time();
    SERIAL.println("WiFi connected!");
    SERIAL.printf("IP: %s\n", WiFi.localIP().toString().c_str());
    SERIAL.printf("RSSI: %d dBm\n", WiFi.RSSI());
    SERIAL.flush();
    return true;
  }
  else
  {
    wifi_connected_us = 0;
    SERIAL.println("WiFi connect timed out.");
    SERIAL.flush();
    return false;
  }
}

void wifiPrintStatus()
{
  wl_status_t st = WiFi.status();
  SERIAL.printf("WiFi status=%d (%s)\n",
                (int)st,
                (st == WL_CONNECTED ? "CONNECTED" :
                 st == WL_DISCONNECTED ? "DISCONNECTED" :
                 st == WL_IDLE_STATUS ? "IDLE" : "OTHER"));

  if (st == WL_CONNECTED)
  {
    SERIAL.printf("IP: %s | RSSI: %d dBm\n",
                  WiFi.localIP().toString().c_str(),
                  WiFi.RSSI());
  }
  SERIAL.flush();
}

void wifiEnableModemSleep(bool enable, wifi_ps_type_t ps = WIFI_PS_MIN_MODEM)
{
  if (WiFi.status() != WL_CONNECTED)
  {
    SERIAL.println("WiFi not connected yet; connecting first...");
    SERIAL.flush();
    wifiConnect();
  }

  if (enable)
  {
    esp_wifi_set_ps(ps);
    WiFi.setSleep(true);
    SERIAL.printf("Modem sleep enabled (ps=%d). WiFi stays connected.\n", (int)ps);
  }
  else
  {
    esp_wifi_set_ps(WIFI_PS_NONE);
    WiFi.setSleep(false);
    SERIAL.println("Modem sleep disabled (ps=NONE).");
  }
  SERIAL.flush();
}

void serialBootBanner()
{
  SERIAL.begin(115200);
  delay(1200);

  bootCount++;
  SERIAL.println();
  SERIAL.println("--- ESP32-C3 Two-Mode System ---");
  SERIAL.printf("bootCount=%lu  wakeCause=%d  currentMode=%s\n",
                (unsigned long)bootCount,
                (int)esp_sleep_get_wakeup_cause(),
                modeName((Mode)modeRtc));
  SERIAL.flush();
}

void sendStatusMessage(const char* msgType)
{
  float batteryV = readBatteryVoltage();
  bool usbPresent = isUsbPresent();
  bool lowBattery = batteryV <= LOW_BATTERY_WARNING_V;

  SERIAL.printf("[%s] ", msgType);
  SERIAL.printf("mode=%s | ", modeName((Mode)modeRtc));
  SERIAL.printf("battery=%.2fV | ", batteryV);
  SERIAL.printf("source=%s", usbPresent ? "USB_5V" : "BATTERY");

  if (usbPresent)
  {
    SERIAL.printf(" | charge=%s", isBatteryCharging() ? "CHARGING" : "FULL");
  }

  if (lowBattery)
  {
    SERIAL.print(" | low_battery=TRUE");
  }
  else
  {
    SERIAL.print(" | low_battery=FALSE");
  }

  SERIAL.println();
  SERIAL.flush();

  // Send data
}

void waitForModeButtonRelease()
{
  while (digitalRead(SW1_PIN) == HIGH)
  {
    delay(10);
  }
  delay(50);
}

void enterDeepSleepUntilButtonOrHeartbeat()
{
  SERIAL.println("Entering DEEP_SLEEP...");
  SERIAL.flush();

  waitForModeButtonRelease();
  setLedColor(0x000000);
  delay(50);

  esp_sleep_enable_timer_wakeup(DEEP_SLEEP_HEARTBEAT_US);

  // Wake on mode button press
  gpio_deep_sleep_wakeup_enable((gpio_num_t)SW1_PIN, GPIO_INTR_HIGH_LEVEL);
  esp_deep_sleep_enable_gpio_wakeup(1ULL << SW1_PIN, ESP_GPIO_WAKEUP_GPIO_HIGH);

  SERIAL.end();
  delay(50);

  esp_deep_sleep_start();
}

void applyMode(Mode m, uint32_t triggerUs)
{
  uint32_t switchUs = micros();
  SERIAL.printf("Mode switch start: %s | trigger->switch = %lu us\n",
                modeName(m), (unsigned long)(switchUs - triggerUs));
  SERIAL.flush();

  switch (m)
  {
    case MODE_ACTIVE:
      SERIAL.println("Now ACTIVE.");
      SERIAL.flush();

      active_us = esp_timer_get_time();
      wifiEnableModemSleep(false);

      if (wifiConnect())
      {
        SERIAL.printf("Boot -> ACTIVE: %.2f ms\n",
                      (active_us - boot_us) / 1000.0);

        SERIAL.printf("ACTIVE -> WiFi connected: %.2f ms\n",
                      (wifi_connected_us - active_us) / 1000.0);

        SERIAL.printf("Boot -> WiFi connected: %.2f ms\n",
                      (wifi_connected_us - boot_us) / 1000.0);
      }
      else
      {
        SERIAL.printf("WiFi failed after %.2f ms from start of connection attempt\n",
                      (esp_timer_get_time() - wifi_start_us) / 1000.0);
      }

      wifiPrintStatus();
      break;

    case MODE_DEEP_SLEEP:
      enterDeepSleepUntilButtonOrHeartbeat();
      break;

    default:
      break;
  }
}

bool buttonPressedEvent(int pin)
{
  static bool lastRawBoot = LOW;
  static bool stableBoot = LOW;
  static bool lastStableBoot = LOW;
  static uint32_t lastChangeBootMs = 0;

  static bool lastRawHelp = LOW;
  static bool stableHelp = LOW;
  static bool lastStableHelp = LOW;
  static uint32_t lastChangeHelpMs = 0;

  bool raw = digitalRead(pin);

  if (pin == SW1_PIN)
  {
    if (raw != lastRawBoot)
    {
      lastChangeBootMs = millis();
      lastRawBoot = raw;
    }

    if (millis() - lastChangeBootMs > DEBOUNCE_MS)
    {
      stableBoot = raw;
    }

    bool pressed = (lastStableBoot == LOW && stableBoot == HIGH);
    lastStableBoot = stableBoot;
    return pressed;
  }

  if (pin == SW2_PIN)
  {
    if (raw != lastRawHelp)
    {
      lastChangeHelpMs = millis();
      lastRawHelp = raw;
    }

    if (millis() - lastChangeHelpMs > DEBOUNCE_MS)
    {
      stableHelp = raw;
    }

    bool pressed = (lastStableHelp == LOW && stableHelp == HIGH);
    lastStableHelp = stableHelp;
    return pressed;
  }

  return false;
}

void setup()
{
  boot_us = esp_timer_get_time();

  pinMode(SW1_PIN, INPUT_PULLDOWN);
  pinMode(SW2_PIN, INPUT_PULLDOWN);
  pinMode(MAIN_POWER_PIN, INPUT);
  pinMode(BATTERY_STAT, INPUT);

  strip.begin();
  delay(25);
  setLedColor(0x000000);

  if (modeRtc >= MODE_COUNT)
  {
    modeRtc = MODE_ACTIVE;
  }

  esp_sleep_wakeup_cause_t wakeCause = esp_sleep_get_wakeup_cause();

  // Cold boot / reset / upload
  if (wakeCause == ESP_SLEEP_WAKEUP_UNDEFINED)
  {
    modeRtc = MODE_ACTIVE;
  }

  serialBootBanner();

  // Deep-sleep timer wake: wake briefly, connect, send heartbeat, go back to sleep
  if (modeRtc == MODE_DEEP_SLEEP && wakeCause == ESP_SLEEP_WAKEUP_TIMER)
  {
    applyMode(MODE_ACTIVE, micros());
    modeRtc = MODE_DEEP_SLEEP;
    sendStatusMessage("HEARTBEAT");
    enterDeepSleepUntilButtonOrHeartbeat();
    return;
  }

  // Button wake from deep sleep, return to ACTIVE
  if (modeRtc == MODE_DEEP_SLEEP && wakeCause == ESP_SLEEP_WAKEUP_GPIO)
  {
    SERIAL.println("Mode button woke device from DEEP_SLEEP. Returning to ACTIVE.");
    SERIAL.flush();
    modeRtc = MODE_ACTIVE;
  }

  applyMode((Mode)modeRtc, micros());
}

void loop()
{
  static uint32_t lastBatteryMs = 0;
  static uint32_t lastWifiPrintMs = 0;
  static uint32_t lastUsbUpdateMs = 0;

  updateStatusLed();

  // In ACTIVE, send updates when USB present
  if (modeRtc == MODE_ACTIVE && isUsbPresent() &&
      millis() - lastUsbUpdateMs >= 10000)
  {
    lastUsbUpdateMs = millis();
    sendStatusMessage("USB_STATUS");
  }

  if (modeRtc == MODE_ACTIVE && millis() - lastWifiPrintMs >= 5000)
  {
    lastWifiPrintMs = millis();
    wifiPrintStatus();
  }

  if (millis() - lastBatteryMs >= 3000)
  {
    lastBatteryMs = millis();

    float v = readBatteryVoltage();
    bool usbPresent = isUsbPresent();

    SERIAL.printf("Battery: %.2f V | Source: %s",
                  v,
                  usbPresent ? "USB_5V" : "BATTERY");

    if (usbPresent)
    {
      SERIAL.printf(" | ChargeState=%s",
                    isBatteryCharging() ? "CHARGING" : "FULL");
    }

    if (v <= LOW_BATTERY_WARNING_V)
    {
      SERIAL.print(" | WARNING: LOW_BATTERY");
    }

    SERIAL.println();
    SERIAL.flush();
  }

  // SW1: Active to Deep Sleep
  if (buttonPressedEvent(SW1_PIN))
  {
    uint32_t pressUs = micros();
    SERIAL.printf("MODE button pressed at ~%lu us\n", (unsigned long)pressUs);
    SERIAL.flush();

    Mode next = (modeRtc == MODE_ACTIVE) ? MODE_DEEP_SLEEP : MODE_ACTIVE;
    modeRtc = (uint8_t)next;
    applyMode(next, pressUs);
  }

  // SW 2: send HELP message (if awake)
  if (buttonPressedEvent(SW2_PIN))
  {
    uint32_t pressUs = micros();
    SERIAL.printf("HELP button pressed at ~%lu us\n", (unsigned long)pressUs);
    SERIAL.flush();

    if (modeRtc != MODE_ACTIVE)
    {
      modeRtc = MODE_ACTIVE;
      applyMode(MODE_ACTIVE, pressUs);
    }

    sendStatusMessage("HELP");
  }
}

*/
