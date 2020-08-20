
#define ESP8266 //comment this line for arduino mode

//board dependent library
#ifdef ESP8266
#include "ESP8266WiFi.h"
#include "WiFiUdp.h"
#include "ESP8266HTTPClient.h"
#include "ESP8266WebServer.h"
#else
#include "WiFiEsp.h"
#include "WiFiEspUdp.h"
#endif

#include "Wire.h"
#include "FastLED.h"
#include "RTClib.h"
#include "TimeLib.h"
#include "EEPROM.h"

const unsigned long intervalOneSec = 1000;
const unsigned long intervalHalfSec = 500;
unsigned long lastRunOneSec = 0;
unsigned long lastRunHalfSec = 0;

//led related
#define LED_PIN D6
#define NUM_LEDS 58

CRGB leds[NUM_LEDS];
byte brightness = 100;
byte brightnessLevels[3]{100, 160, 230};
bool tick = true;
unsigned long lastTicking;
int rgbColor[3];
int minutesToDisplay;

//Wifi related
// char ssid[] = "kurir JNE";
// char pass[] = "tanyayanglain";
// char ssid[] = "XLGO-7648";
// char pass[] = "hurufa8kali";

int statusCode;
String st;
String content;

int wifiStatus = WL_IDLE_STATUS;
bool packetSent = false;
bool failureData = false;

//clock related
char daysOfTheWeek[7][12] = {
    "Minggu",
    "Senin",
    "Selasa",
    "Rabu",
    "Kamis",
    "Jumat",
    "Sabtu"};
char timeServer[] = "time.google.com";
int utcTimeOffset = 7;
unsigned int localPort = 2390;
unsigned long ntpSyncInterval = 30000; //in millisecond
unsigned long lastNTPSync = 0;

unsigned long currentEpoch;

const int NTP_PACKET_SIZE = 48;
const int UDP_TIMEOUT = 2000;

RTC_DS3231 rtc;
time_t epochTime;
bool hour24Mode = true;
bool forceSync = false;

byte packetBuffer[NTP_PACKET_SIZE];

//others variable
#ifdef ESP8266
WiFiUDP udp;
const bool espBoard = true;
// ESP8266WebServer server(80);
#else
const bool espBoard = false;
WiFiEspUDP udp;
#endif

const int buttonA = D4;
bool buttonAState = false;
bool lastButtonAState = false;
unsigned long lastADebounceTime = 0;

const int buttonB = D3;
bool buttonBState = false;
bool lastButtonBState = false;
unsigned long lastBDebounceTime = 0;

unsigned long debounceDelayTime = 50;

int checkSection(int section);
void digitZero(int section, int colors[]);
void digitOne(int section, int colors[]);
void digitTwo(int section, int colors[]);
void digitThree(int section, int colors[]);
void digitFour(int section, int colors[]);
void digitFive(int section, int colors[]);
void digitSix(int section, int colors[]);
void digitSeven(int section, int colors[]);
void digitEight(int section, int colors[]);
void digitNine(int section, int colors[]);

bool connectWifi(void);
void launchWeb(void);
void setupAP(void);

void tickOn(int colors[])
{
  leds[28] = CRGB(colors[0], colors[1], colors[2]);
  leds[29] = CRGB(colors[0], colors[1], colors[2]);

  FastLED.show();
}

void tickOff()
{
  leds[28] = CRGB::Black;
  leds[29] = CRGB::Black;
  FastLED.show();
}

void changeColorAllWave(int rColor, int gColor, int bColor, int timeDelay = 100)
{
  for (int i = 0; i < NUM_LEDS; i++)
  {
    leds[i] = CRGB(rColor, gColor, bColor);
    delay(timeDelay);
    FastLED.show();
  }
}
void changeColorAll(int rColor, int gColor, int bColor)
{
  for (int i = 0; i < NUM_LEDS; i++)
  {
    leds[i] = CRGB(rColor, gColor, bColor);
  }
  FastLED.show();
}

typedef void (*GenericFP)(int, int[]);
GenericFP Digits[10] = {
    &digitZero,
    &digitOne,
    &digitTwo,
    &digitThree,
    &digitFour,
    &digitFive,
    &digitSix,
    &digitSeven,
    &digitEight,
    &digitNine};

void drawHours()
{
  DateTime now = rtc.now();
  int hours = now.hour();
  if (!hour24Mode)
  {
    if (hours > 12)
    {
      hours -= 12;
    }
  }
  int ones = hours % 10;
  int tens = (hours / 10) % 10;
  if (tens != 0)
  {
    Digits[tens](4, rgbColor);
  }
  else
  {
    int black[3] = {0, 0, 0};
    Digits[tens](4, black);
  }

  Digits[ones](3, rgbColor);
}

void drawMinutes()
{
  DateTime now = rtc.now();
  int minutes = now.minute();
  int ones = minutes % 10;
  int tens = (minutes / 10) % 10;
  Digits[ones](1, rgbColor);
  Digits[tens](2, rgbColor);
}

void setup()
{
  pinMode(buttonA, INPUT_PULLUP);
  pinMode(buttonB, INPUT_PULLUP);
  FastLED.addLeds<WS2812, LED_PIN, GRB>(leds, NUM_LEDS);
  FastLED.setMaxPowerInVoltsAndMilliamps(5, 750);
  allOff();
  changeColorAllWave(20, 20, 150, 50);
  Serial.begin(115200);
  Serial.println();

//esp wifi shield init
#ifndef ESP8266
  Serial.println("Arduino with WiFi shield");
  Serial3.begin(115200);
  WiFi.init(&Serial3);
  if (WiFi.status() == WL_NO_SHIELD)
  {
    Serial.println("No WiFi Module");
    while (true)
      ;
  }
#else
  Serial.println("ESP Board");
#endif
  WiFi.mode(WIFI_STA);
  int attempt = 0;
  if (connectWifi())
  {
    Serial.println("Connected!!");
    udp.begin(localPort);
    while (!updateNTP() && attempt < 10)
    {
      delay(100);
      attempt++;
    }
    changeColorAll(20, 150, 20);
  }
  else
  {
    changeColorAll(150, 20, 20);
  }

  changeColorAllWave(0, 0, 0, 10);

  // timeClient.begin();
  if (!rtc.begin())
  {
    changeColorAll(150, 20, 20);
    Serial.println("Couldn't find RTC");
    Serial.flush();
    abort();
  }
  if (rtc.lostPower())
  {
    Serial.println("Setup RTC..");
    if (attempt > 10)
    {
      while (updateNTP())
      {
      }
    }

    setRTC();
  }
}

void loop()
{
  rgbColor[0] = 0;
  rgbColor[1] = 120;
  rgbColor[2] = 255;
  unsigned long currentMillis = millis();

  // Serial.println(digitalRead(buttonA));
  // Serial.println(digitalRead(buttonB));
  bool readingA = digitalRead(buttonA);

  if (readingA != lastButtonAState)
  {
    lastADebounceTime = millis();
  }

  if ((millis() - lastADebounceTime) > debounceDelayTime)
  {
    if (readingA != buttonAState)
    {
      buttonAState = readingA;
      if (!buttonAState)
      {
        //WHEN BUTTON A CLICKED
        Serial.println("Button A clicked");
        //change hour 24 mode
        hour24Mode = !hour24Mode;
        drawHours();
      }
    }
  }
  lastButtonAState = readingA;

  bool readingB = digitalRead(buttonB);

  if (readingB != lastButtonBState)
  {
    lastBDebounceTime = millis();
  }

  if ((millis() - lastBDebounceTime) > debounceDelayTime)
  {
    if (readingB != buttonBState)
    {
      buttonBState = readingB;
      if (!buttonBState)
      {
        //WHEN BUTTON A CLICKED
        Serial.println("Button B clicked");
        forceSync = true;
      }
    }
  }
  lastButtonBState = readingB;

  if ((currentMillis - lastNTPSync >= ntpSyncInterval) || forceSync)
  {
    if (updateNTP() && WiFi.status() == WL_CONNECTED)
    {
      setRTC();
      if (forceSync)
      {
        forceSync = false;
      }
    }
  }

  if (currentMillis - lastRunOneSec >= intervalOneSec)
  {
    lastRunOneSec = millis();

    drawMinutes();
    drawHours();
  }

  if (currentMillis - lastRunHalfSec >= intervalHalfSec)
  {
    lastRunHalfSec = millis();
    // Serial.println("Half sec update;");
    if (tick)
    {
      tickOn(rgbColor);
    }
    else
    {
      tickOff();
    }
    tick = !tick;
  }
}

bool connectWifi(void)
{
  int attempt = 0;
  while (attempt < 20)
  {
    if (WiFi.status() == WL_CONNECTED)
    {
      return true;
    }
    delay(500);
    Serial.print(".");
    attempt++;
  }
  Serial.println();
  Serial.println("Failed to connect");
  return false;
}

void setRTC()
{
  time_t localEpochTime = getEpochTime();
  setTime(localEpochTime);
  rtc.adjust(DateTime(year(), month(), day(), hour(), minute(), second()));
  Serial.println("rtc synced");
}

unsigned long getEpochTime()
{
  return currentEpoch + (utcTimeOffset * 60 * 60);
}

void printWifiStatus()
{
  Serial.print("SSID: ");
  Serial.println(WiFi.SSID());
  IPAddress ip = WiFi.localIP();
  Serial.print("Alamat IP: ");
  Serial.println(ip);
  long rssi = WiFi.RSSI();
  Serial.print("Kekuatan Sinyal (RSSI):");
  Serial.print(rssi);
  Serial.println(" dBm");
}

bool sendNTPpacket(char *ntpSrv)
{
  // set all bytes in the buffer to 0
  memset(packetBuffer, 0, NTP_PACKET_SIZE);
  // Initialize values needed to form NTP request
  // (see URL above for details on the packets)

  packetBuffer[0] = 0b11100011; // LI, Version, Mode
  packetBuffer[1] = 0;          // Stratum, or type of clock
  packetBuffer[2] = 6;          // Polling interval
  packetBuffer[3] = 0xEC;       // Peer Clock Precision
  // 8 bytes of zero for Root Delay & Root Dispersion
  packetBuffer[12] = 49;
  packetBuffer[13] = 0x4E;
  packetBuffer[14] = 49;
  packetBuffer[15] = 52;

  // all NTP fields have been given values, now
  // you can send a packet requesting a timestamp:
  udp.beginPacket(ntpSrv, 123); //NTP requests are to port 123

  udp.write(packetBuffer, NTP_PACKET_SIZE);

  udp.endPacket();

  return true;
}

unsigned long startMs;

bool updateNTP()
{

  // Serial.println("Update NTP..");

  if (packetSent)
  {
    failureData = false;
    if (!udp.available() && (millis() - startMs) >= UDP_TIMEOUT)
    {
      Serial.println("Timeout");
      lastNTPSync = millis() - UDP_TIMEOUT;
      packetSent = false;
      failureData = true;
    }
    else if (udp.parsePacket() && packetSent)
    {
      // Serial.println("packet received");
      lastNTPSync = millis();
      udp.read(packetBuffer, NTP_PACKET_SIZE);
      unsigned long highWord = word(packetBuffer[40], packetBuffer[41]);
      unsigned long lowWord = word(packetBuffer[42], packetBuffer[43]);
      unsigned long secsSince1900 = highWord << 16 | lowWord;
      const unsigned long seventyYears = 2208988800UL;
      currentEpoch = secsSince1900 - seventyYears;
      Serial.print("NTP Epoch: ");
      Serial.println(secsSince1900 - seventyYears);
      packetSent = false;
      return true;
    }
    else
    {
    }
  }
  else
  {
    // Serial.println("Sending packet");
    packetSent = sendNTPpacket(timeServer);
    startMs = millis();
    // Serial.print("start at: ");
    // Serial.println(startMs);
  }
  return false;
  // Serial.print("cb: ");
  // Serial.println(cb);
}

void allOff()
{
  for (int i = 0; i < NUM_LEDS; i++)
  {
    leds[i] = CRGB::Black;
  }
  FastLED.show();
}

void sectionOff(int section)
{
  section *= 7;
  for (int i = 0; i <= 7; i++)
  {
    leds[section + i] = CRGB::Black;
  }
  FastLED.show();
}

int checkSection(int section)
{
  switch (section)
  {
  case 1:
    return 0;
    break;
  case 2:
    return 14;
    break;
  case 3:
    return 30;
    break;
  case 4:
    return 44;
    break;
  default:
    return 0;
  }
}

void digitZero(int section, int colors[])
{
  int ledOrder = checkSection(section);

  if (section % 2)
  {
    leds[ledOrder + 0] = CRGB(0, 0, 0);
    leds[ledOrder + 1] = CRGB(0, 0, 0);
    leds[ledOrder + 2] = CRGB(colors[0], colors[1], colors[2]);
    leds[ledOrder + 3] = CRGB(colors[0], colors[1], colors[2]);
    leds[ledOrder + 4] = CRGB(colors[0], colors[1], colors[2]);
    leds[ledOrder + 5] = CRGB(colors[0], colors[1], colors[2]);
    leds[ledOrder + 6] = CRGB(colors[0], colors[1], colors[2]);
    leds[ledOrder + 7] = CRGB(colors[0], colors[1], colors[2]);
    leds[ledOrder + 8] = CRGB(colors[0], colors[1], colors[2]);
    leds[ledOrder + 9] = CRGB(colors[0], colors[1], colors[2]);
    leds[ledOrder + 10] = CRGB(colors[0], colors[1], colors[2]);
    leds[ledOrder + 11] = CRGB(colors[0], colors[1], colors[2]);
    leds[ledOrder + 12] = CRGB(colors[0], colors[1], colors[2]);
    leds[ledOrder + 13] = CRGB(colors[0], colors[1], colors[2]);
  }
  else
  {
    leds[ledOrder + 13] = CRGB(0, 0, 0);
    leds[ledOrder + 12] = CRGB(0, 0, 0);
    leds[ledOrder + 11] = CRGB(colors[0], colors[1], colors[2]);
    leds[ledOrder + 10] = CRGB(colors[0], colors[1], colors[2]);
    leds[ledOrder + 9] = CRGB(colors[0], colors[1], colors[2]);
    leds[ledOrder + 8] = CRGB(colors[0], colors[1], colors[2]);
    leds[ledOrder + 7] = CRGB(colors[0], colors[1], colors[2]);
    leds[ledOrder + 6] = CRGB(colors[0], colors[1], colors[2]);
    leds[ledOrder + 5] = CRGB(colors[0], colors[1], colors[2]);
    leds[ledOrder + 4] = CRGB(colors[0], colors[1], colors[2]);
    leds[ledOrder + 3] = CRGB(colors[0], colors[1], colors[2]);
    leds[ledOrder + 2] = CRGB(colors[0], colors[1], colors[2]);
    leds[ledOrder + 1] = CRGB(colors[0], colors[1], colors[2]);
    leds[ledOrder + 0] = CRGB(colors[0], colors[1], colors[2]);
  }

  FastLED.show();
}

void digitOne(int section, int colors[])
{
  int ledOrder = checkSection(section);

  if (section % 2)
  {
    leds[ledOrder + 0] = CRGB(0, 0, 0);
    leds[ledOrder + 1] = CRGB(0, 0, 0);
    leds[ledOrder + 2] = CRGB(0, 0, 0);
    leds[ledOrder + 3] = CRGB(0, 0, 0);
    leds[ledOrder + 4] = CRGB(0, 0, 0);
    leds[ledOrder + 5] = CRGB(0, 0, 0);
    leds[ledOrder + 6] = CRGB(colors[0], colors[1], colors[2]);
    leds[ledOrder + 7] = CRGB(colors[0], colors[1], colors[2]);
    leds[ledOrder + 8] = CRGB(colors[0], colors[1], colors[2]);
    leds[ledOrder + 9] = CRGB(colors[0], colors[1], colors[2]);
    leds[ledOrder + 10] = CRGB(0, 0, 0);
    leds[ledOrder + 11] = CRGB(0, 0, 0);
    leds[ledOrder + 12] = CRGB(0, 0, 0);
    leds[ledOrder + 13] = CRGB(0, 0, 0);
  }
  else
  {
    leds[ledOrder + 0] = CRGB(colors[0], colors[1], colors[2]);
    leds[ledOrder + 1] = CRGB(colors[0], colors[1], colors[2]);
    leds[ledOrder + 2] = CRGB(0, 0, 0);
    leds[ledOrder + 3] = CRGB(0, 0, 0);
    leds[ledOrder + 4] = CRGB(0, 0, 0);
    leds[ledOrder + 5] = CRGB(0, 0, 0);
    leds[ledOrder + 6] = CRGB(0, 0, 0);
    leds[ledOrder + 7] = CRGB(0, 0, 0);
    leds[ledOrder + 8] = CRGB(0, 0, 0);
    leds[ledOrder + 9] = CRGB(0, 0, 0);
    leds[ledOrder + 10] = CRGB(colors[0], colors[1], colors[2]);
    leds[ledOrder + 11] = CRGB(colors[0], colors[1], colors[2]);
    leds[ledOrder + 12] = CRGB(0, 0, 0);
    leds[ledOrder + 13] = CRGB(0, 0, 0);
  }
  FastLED.show();
}

void digitTwo(int section, int colors[])
{
  int ledOrder = checkSection(section);

  if (section % 2)
  {
    leds[ledOrder + 0] = CRGB(colors[0], colors[1], colors[2]);
    leds[ledOrder + 1] = CRGB(colors[0], colors[1], colors[2]);
    leds[ledOrder + 2] = CRGB(0, 0, 0);
    leds[ledOrder + 3] = CRGB(0, 0, 0);
    leds[ledOrder + 4] = CRGB(colors[0], colors[1], colors[2]);
    leds[ledOrder + 5] = CRGB(colors[0], colors[1], colors[2]);
    leds[ledOrder + 6] = CRGB(colors[0], colors[1], colors[2]);
    leds[ledOrder + 7] = CRGB(colors[0], colors[1], colors[2]);
    leds[ledOrder + 8] = CRGB(0, 0, 0);
    leds[ledOrder + 9] = CRGB(0, 0, 0);
    leds[ledOrder + 10] = CRGB(colors[0], colors[1], colors[2]);
    leds[ledOrder + 11] = CRGB(colors[0], colors[1], colors[2]);
    leds[ledOrder + 12] = CRGB(colors[0], colors[1], colors[2]);
    leds[ledOrder + 13] = CRGB(colors[0], colors[1], colors[2]);
  }
  else
  {
    leds[ledOrder + 0] = CRGB(0, 0, 0);
    leds[ledOrder + 1] = CRGB(0, 0, 0);
    leds[ledOrder + 2] = CRGB(colors[0], colors[1], colors[2]);
    leds[ledOrder + 3] = CRGB(colors[0], colors[1], colors[2]);
    leds[ledOrder + 4] = CRGB(colors[0], colors[1], colors[2]);
    leds[ledOrder + 5] = CRGB(colors[0], colors[1], colors[2]);
    leds[ledOrder + 6] = CRGB(0, 0, 0);
    leds[ledOrder + 7] = CRGB(0, 0, 0);
    leds[ledOrder + 8] = CRGB(colors[0], colors[1], colors[2]);
    leds[ledOrder + 9] = CRGB(colors[0], colors[1], colors[2]);
    leds[ledOrder + 10] = CRGB(colors[0], colors[1], colors[2]);
    leds[ledOrder + 11] = CRGB(colors[0], colors[1], colors[2]);
    leds[ledOrder + 12] = CRGB(colors[0], colors[1], colors[2]);
    leds[ledOrder + 13] = CRGB(colors[0], colors[1], colors[2]);
  }

  FastLED.show();
}

void digitThree(int section, int colors[])
{
  int ledOrder = checkSection(section);

  if (section % 2)
  {
    leds[ledOrder + 0] = CRGB(colors[0], colors[1], colors[2]);
    leds[ledOrder + 1] = CRGB(colors[0], colors[1], colors[2]);
    leds[ledOrder + 2] = CRGB(0, 0, 0);
    leds[ledOrder + 3] = CRGB(0, 0, 0);
    leds[ledOrder + 4] = CRGB(colors[0], colors[1], colors[2]);
    leds[ledOrder + 5] = CRGB(colors[0], colors[1], colors[2]);
    leds[ledOrder + 6] = CRGB(colors[0], colors[1], colors[2]);
    leds[ledOrder + 7] = CRGB(colors[0], colors[1], colors[2]);
    leds[ledOrder + 8] = CRGB(colors[0], colors[1], colors[2]);
    leds[ledOrder + 9] = CRGB(colors[0], colors[1], colors[2]);
    leds[ledOrder + 10] = CRGB(colors[0], colors[1], colors[2]);
    leds[ledOrder + 11] = CRGB(colors[0], colors[1], colors[2]);
    leds[ledOrder + 12] = CRGB(0, 0, 0);
    leds[ledOrder + 13] = CRGB(0, 0, 0);
  }
  else
  {
    leds[ledOrder + 0] = CRGB(colors[0], colors[1], colors[2]);
    leds[ledOrder + 1] = CRGB(colors[0], colors[1], colors[2]);
    leds[ledOrder + 2] = CRGB(colors[0], colors[1], colors[2]);
    leds[ledOrder + 3] = CRGB(colors[0], colors[1], colors[2]);
    leds[ledOrder + 4] = CRGB(0, 0, 0);
    leds[ledOrder + 5] = CRGB(0, 0, 0);
    leds[ledOrder + 6] = CRGB(0, 0, 0);
    leds[ledOrder + 7] = CRGB(0, 0, 0);
    leds[ledOrder + 8] = CRGB(colors[0], colors[1], colors[2]);
    leds[ledOrder + 9] = CRGB(colors[0], colors[1], colors[2]);
    leds[ledOrder + 10] = CRGB(colors[0], colors[1], colors[2]);
    leds[ledOrder + 11] = CRGB(colors[0], colors[1], colors[2]);
    leds[ledOrder + 12] = CRGB(colors[0], colors[1], colors[2]);
    leds[ledOrder + 13] = CRGB(colors[0], colors[1], colors[2]);
  }

  FastLED.show();
}

void digitFour(int section, int colors[])
{
  int ledOrder = checkSection(section);

  if (section % 2)
  {
    leds[ledOrder + 0] = CRGB(colors[0], colors[1], colors[2]);
    leds[ledOrder + 1] = CRGB(colors[0], colors[1], colors[2]);
    leds[ledOrder + 2] = CRGB(colors[0], colors[1], colors[2]);
    leds[ledOrder + 3] = CRGB(colors[0], colors[1], colors[2]);
    leds[ledOrder + 4] = CRGB(0, 0, 0);
    leds[ledOrder + 5] = CRGB(0, 0, 0);
    leds[ledOrder + 6] = CRGB(colors[0], colors[1], colors[2]);
    leds[ledOrder + 7] = CRGB(colors[0], colors[1], colors[2]);
    leds[ledOrder + 8] = CRGB(colors[0], colors[1], colors[2]);
    leds[ledOrder + 9] = CRGB(colors[0], colors[1], colors[2]);
    leds[ledOrder + 10] = CRGB(0, 0, 0);
    leds[ledOrder + 11] = CRGB(0, 0, 0);
    leds[ledOrder + 12] = CRGB(0, 0, 0);
    leds[ledOrder + 13] = CRGB(0, 0, 0);
  }
  else
  {
    leds[ledOrder + 0] = CRGB(colors[0], colors[1], colors[2]);
    leds[ledOrder + 1] = CRGB(colors[0], colors[1], colors[2]);
    leds[ledOrder + 2] = CRGB(0, 0, 0);
    leds[ledOrder + 3] = CRGB(0, 0, 0);
    leds[ledOrder + 4] = CRGB(0, 0, 0);
    leds[ledOrder + 5] = CRGB(0, 0, 0);
    leds[ledOrder + 6] = CRGB(colors[0], colors[1], colors[2]);
    leds[ledOrder + 7] = CRGB(colors[0], colors[1], colors[2]);
    leds[ledOrder + 8] = CRGB(0, 0, 0);
    leds[ledOrder + 9] = CRGB(0, 0, 0);
    leds[ledOrder + 10] = CRGB(colors[0], colors[1], colors[2]);
    leds[ledOrder + 11] = CRGB(colors[0], colors[1], colors[2]);
    leds[ledOrder + 12] = CRGB(colors[0], colors[1], colors[2]);
    leds[ledOrder + 13] = CRGB(colors[0], colors[1], colors[2]);
  }

  FastLED.show();
}

void digitFive(int section, int colors[])
{
  int ledOrder = checkSection(section);

  if (section % 2)
  {
    leds[ledOrder + 0] = CRGB(colors[0], colors[1], colors[2]);
    leds[ledOrder + 1] = CRGB(colors[0], colors[1], colors[2]);
    leds[ledOrder + 2] = CRGB(colors[0], colors[1], colors[2]);
    leds[ledOrder + 3] = CRGB(colors[0], colors[1], colors[2]);
    leds[ledOrder + 4] = CRGB(colors[0], colors[1], colors[2]);
    leds[ledOrder + 5] = CRGB(colors[0], colors[1], colors[2]);
    leds[ledOrder + 6] = CRGB(0, 0, 0);
    leds[ledOrder + 7] = CRGB(0, 0, 0);
    leds[ledOrder + 8] = CRGB(colors[0], colors[1], colors[2]);
    leds[ledOrder + 9] = CRGB(colors[0], colors[1], colors[2]);
    leds[ledOrder + 10] = CRGB(colors[0], colors[1], colors[2]);
    leds[ledOrder + 11] = CRGB(colors[0], colors[1], colors[2]);
    leds[ledOrder + 12] = CRGB(0, 0, 0);
    leds[ledOrder + 13] = CRGB(0, 0, 0);
  }
  else
  {
    leds[ledOrder + 0] = CRGB(colors[0], colors[1], colors[2]);
    leds[ledOrder + 1] = CRGB(colors[0], colors[1], colors[2]);
    leds[ledOrder + 2] = CRGB(colors[0], colors[1], colors[2]);
    leds[ledOrder + 3] = CRGB(colors[0], colors[1], colors[2]);
    leds[ledOrder + 4] = CRGB(0, 0, 0);
    leds[ledOrder + 5] = CRGB(0, 0, 0);
    leds[ledOrder + 6] = CRGB(colors[0], colors[1], colors[2]);
    leds[ledOrder + 7] = CRGB(colors[0], colors[1], colors[2]);
    leds[ledOrder + 8] = CRGB(colors[0], colors[1], colors[2]);
    leds[ledOrder + 9] = CRGB(colors[0], colors[1], colors[2]);
    leds[ledOrder + 10] = CRGB(0, 0, 0);
    leds[ledOrder + 11] = CRGB(0, 0, 0);
    leds[ledOrder + 12] = CRGB(colors[0], colors[1], colors[2]);
    leds[ledOrder + 13] = CRGB(colors[0], colors[1], colors[2]);
  }

  FastLED.show();
}

void digitSix(int section, int colors[])
{
  int ledOrder = checkSection(section);

  if (section % 2)
  {
    leds[ledOrder + 0] = CRGB(colors[0], colors[1], colors[2]);
    leds[ledOrder + 1] = CRGB(colors[0], colors[1], colors[2]);
    leds[ledOrder + 2] = CRGB(colors[0], colors[1], colors[2]);
    leds[ledOrder + 3] = CRGB(colors[0], colors[1], colors[2]);
    leds[ledOrder + 4] = CRGB(colors[0], colors[1], colors[2]);
    leds[ledOrder + 5] = CRGB(colors[0], colors[1], colors[2]);
    leds[ledOrder + 6] = CRGB(0, 0, 0);
    leds[ledOrder + 7] = CRGB(0, 0, 0);
    leds[ledOrder + 8] = CRGB(colors[0], colors[1], colors[2]);
    leds[ledOrder + 9] = CRGB(colors[0], colors[1], colors[2]);
    leds[ledOrder + 10] = CRGB(colors[0], colors[1], colors[2]);
    leds[ledOrder + 11] = CRGB(colors[0], colors[1], colors[2]);
    leds[ledOrder + 12] = CRGB(colors[0], colors[1], colors[2]);
    leds[ledOrder + 13] = CRGB(colors[0], colors[1], colors[2]);
  }
  else
  {
    leds[ledOrder + 0] = CRGB(colors[0], colors[1], colors[2]);
    leds[ledOrder + 1] = CRGB(colors[0], colors[1], colors[2]);
    leds[ledOrder + 2] = CRGB(colors[0], colors[1], colors[2]);
    leds[ledOrder + 3] = CRGB(colors[0], colors[1], colors[2]);
    leds[ledOrder + 4] = CRGB(0, 0, 0);
    leds[ledOrder + 5] = CRGB(0, 0, 0);
    leds[ledOrder + 6] = CRGB(colors[0], colors[1], colors[2]);
    leds[ledOrder + 7] = CRGB(colors[0], colors[1], colors[2]);
    leds[ledOrder + 8] = CRGB(colors[0], colors[1], colors[2]);
    leds[ledOrder + 9] = CRGB(colors[0], colors[1], colors[2]);
    leds[ledOrder + 10] = CRGB(colors[0], colors[1], colors[2]);
    leds[ledOrder + 11] = CRGB(colors[0], colors[1], colors[2]);
    leds[ledOrder + 12] = CRGB(colors[0], colors[1], colors[2]);
    leds[ledOrder + 13] = CRGB(colors[0], colors[1], colors[2]);
  }

  FastLED.show();
}

void digitSeven(int section, int colors[])
{
  int ledOrder = checkSection(section);

  if (section % 2)
  {
    leds[ledOrder + 0] = CRGB(0, 0, 0);
    leds[ledOrder + 1] = CRGB(0, 0, 0);
    leds[ledOrder + 2] = CRGB(0, 0, 0);
    leds[ledOrder + 3] = CRGB(0, 0, 0);
    leds[ledOrder + 4] = CRGB(colors[0], colors[1], colors[2]);
    leds[ledOrder + 5] = CRGB(colors[0], colors[1], colors[2]);
    leds[ledOrder + 6] = CRGB(colors[0], colors[1], colors[2]);
    leds[ledOrder + 7] = CRGB(colors[0], colors[1], colors[2]);
    leds[ledOrder + 8] = CRGB(colors[0], colors[1], colors[2]);
    leds[ledOrder + 9] = CRGB(colors[0], colors[1], colors[2]);
    leds[ledOrder + 10] = CRGB(0, 0, 0);
    leds[ledOrder + 11] = CRGB(0, 0, 0);
    leds[ledOrder + 12] = CRGB(0, 0, 0);
    leds[ledOrder + 13] = CRGB(0, 0, 0);
  }
  else
  {
    leds[ledOrder + 0] = CRGB(colors[0], colors[1], colors[2]);
    leds[ledOrder + 1] = CRGB(colors[0], colors[1], colors[2]);
    leds[ledOrder + 2] = CRGB(0, 0, 0);
    leds[ledOrder + 3] = CRGB(0, 0, 0);
    leds[ledOrder + 4] = CRGB(0, 0, 0);
    leds[ledOrder + 5] = CRGB(0, 0, 0);
    leds[ledOrder + 6] = CRGB(0, 0, 0);
    leds[ledOrder + 7] = CRGB(0, 0, 0);
    leds[ledOrder + 8] = CRGB(colors[0], colors[1], colors[2]);
    leds[ledOrder + 9] = CRGB(colors[0], colors[1], colors[2]);
    leds[ledOrder + 10] = CRGB(colors[0], colors[1], colors[2]);
    leds[ledOrder + 11] = CRGB(colors[0], colors[1], colors[2]);
    leds[ledOrder + 12] = CRGB(0, 0, 0);
    leds[ledOrder + 13] = CRGB(0, 0, 0);
  }

  FastLED.show();
}

void digitEight(int section, int colors[])
{
  int ledOrder = checkSection(section);

  leds[ledOrder + 0] = CRGB(colors[0], colors[1], colors[2]);
  leds[ledOrder + 1] = CRGB(colors[0], colors[1], colors[2]);
  leds[ledOrder + 2] = CRGB(colors[0], colors[1], colors[2]);
  leds[ledOrder + 3] = CRGB(colors[0], colors[1], colors[2]);
  leds[ledOrder + 4] = CRGB(colors[0], colors[1], colors[2]);
  leds[ledOrder + 5] = CRGB(colors[0], colors[1], colors[2]);
  leds[ledOrder + 6] = CRGB(colors[0], colors[1], colors[2]);
  leds[ledOrder + 7] = CRGB(colors[0], colors[1], colors[2]);
  leds[ledOrder + 8] = CRGB(colors[0], colors[1], colors[2]);
  leds[ledOrder + 9] = CRGB(colors[0], colors[1], colors[2]);
  leds[ledOrder + 10] = CRGB(colors[0], colors[1], colors[2]);
  leds[ledOrder + 11] = CRGB(colors[0], colors[1], colors[2]);
  leds[ledOrder + 12] = CRGB(colors[0], colors[1], colors[2]);
  leds[ledOrder + 13] = CRGB(colors[0], colors[1], colors[2]);

  FastLED.show();
}

void digitNine(int section, int colors[])
{
  int ledOrder = checkSection(section);

  if (section % 2)
  {
    leds[ledOrder + 0] = CRGB(colors[0], colors[1], colors[2]);
    leds[ledOrder + 1] = CRGB(colors[0], colors[1], colors[2]);
    leds[ledOrder + 2] = CRGB(colors[0], colors[1], colors[2]);
    leds[ledOrder + 3] = CRGB(colors[0], colors[1], colors[2]);
    leds[ledOrder + 4] = CRGB(colors[0], colors[1], colors[2]);
    leds[ledOrder + 5] = CRGB(colors[0], colors[1], colors[2]);
    leds[ledOrder + 6] = CRGB(colors[0], colors[1], colors[2]);
    leds[ledOrder + 7] = CRGB(colors[0], colors[1], colors[2]);
    leds[ledOrder + 8] = CRGB(colors[0], colors[1], colors[2]);
    leds[ledOrder + 9] = CRGB(colors[0], colors[1], colors[2]);
    leds[ledOrder + 10] = CRGB(colors[0], colors[1], colors[2]);
    leds[ledOrder + 11] = CRGB(colors[0], colors[1], colors[2]);
    leds[ledOrder + 12] = CRGB(0, 0, 0);
    leds[ledOrder + 13] = CRGB(0, 0, 0);
  }
  else
  {
    leds[ledOrder + 13] = CRGB(colors[0], colors[1], colors[2]);
    leds[ledOrder + 12] = CRGB(colors[0], colors[1], colors[2]);
    leds[ledOrder + 11] = CRGB(colors[0], colors[1], colors[2]);
    leds[ledOrder + 10] = CRGB(colors[0], colors[1], colors[2]);
    leds[ledOrder + 9] = CRGB(colors[0], colors[1], colors[2]);
    leds[ledOrder + 8] = CRGB(colors[0], colors[1], colors[2]);
    leds[ledOrder + 7] = CRGB(colors[0], colors[1], colors[2]);
    leds[ledOrder + 6] = CRGB(colors[0], colors[1], colors[2]);
    leds[ledOrder + 5] = CRGB(0, 0, 0);
    leds[ledOrder + 4] = CRGB(0, 0, 0);
    leds[ledOrder + 3] = CRGB(colors[0], colors[1], colors[2]);
    leds[ledOrder + 2] = CRGB(colors[0], colors[1], colors[2]);
    leds[ledOrder + 1] = CRGB(colors[0], colors[1], colors[2]);
    leds[ledOrder + 0] = CRGB(colors[0], colors[1], colors[2]);
  }
  FastLED.show();
}
