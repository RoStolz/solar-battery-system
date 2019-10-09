#include <WiFi.h>
#include <ESPmDNS.h>
#include <WiFiUdp.h>
#include <ArduinoOTA.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BMP280.h>
#include <Wire.h>
#include "SSD1306Wire.h" // legacy include: `#include "SSD1306.h"`
#include <SDL_Arduino_INA3221.h>

SDL_Arduino_INA3221 ina3221;
const char* ssid = "WIFI_Name";
const char* password = "WIFI_Password";

SSD1306Wire  display(0x3c, 21, 22);

//Stromsensor
#define SOLAR_CELL_CHANNEL 1
#define BATTERY_CHARGER_CHANNEL 2
#define BATTERY_OUTPUT_CHANNEL 3

float batterieSpannung;
float ladeStrom;
float solarStrom;
float ausgangsStrom;

//#define KALIBRIERUNG


//Kalibrierfunktion Stromsensoren
//Stromanzeige bei 0A Teststrom
#define mVbei0Ach1 0
#define mVbei0Ach2 0
#define mVbei0Ach3 0
#define R1 5.25
#define R2 0.34
#define R3 5.22
#define R4 3.37
#define R5 1.49

//Stromanzeige bei 1A Teststrom
#define mVbei1Ach1 5
#define mVbei1Ach2 5
#define mVbei1Ach3 5

//Formel: (mVgemessen-mVbei0Ach1)/(mVbei1Ach1-mVbei0Ach1)

Adafruit_BMP280 bme;

float pre = 101000;

void setup() {
  Serial.begin(115200);
  Serial.println("Booting");
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  while (WiFi.waitForConnectResult() != WL_CONNECTED) {
    Serial.println("Connection Failed! Rebooting...");
    delay(5000);
    ESP.restart();
  }

  // Port defaults to 3232
  // ArduinoOTA.setPort(3232);

  // Hostname defaults to esp3232-[MAC]
  // ArduinoOTA.setHostname("myesp32");

  // No authentication by default
  // ArduinoOTA.setPassword("admin");

  // Password can be set with it's md5 value as well
  // MD5(admin) = 21232f297a57a5a743894a0e4a801fc3
  // ArduinoOTA.setPasswordHash("21232f297a57a5a743894a0e4a801fc3");

  ArduinoOTA
  .onStart([]() {
    String type;
    if (ArduinoOTA.getCommand() == U_FLASH)
      type = "sketch";
    else // U_SPIFFS
      type = "filesystem";

    // NOTE: if updating SPIFFS this would be the place to unmount SPIFFS using SPIFFS.end()
    Serial.println("Start updating " + type);
  })
  .onEnd([]() {
    Serial.println("\nEnd");
  })
  .onProgress([](unsigned int progress, unsigned int total) {
    Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
  })
  .onError([](ota_error_t error) {
    Serial.printf("Error[%u]: ", error);
    if (error == OTA_AUTH_ERROR) Serial.println("Auth Failed");
    else if (error == OTA_BEGIN_ERROR) Serial.println("Begin Failed");
    else if (error == OTA_CONNECT_ERROR) Serial.println("Connect Failed");
    else if (error == OTA_RECEIVE_ERROR) Serial.println("Receive Failed");
    else if (error == OTA_END_ERROR) Serial.println("End Failed");
  });

  ArduinoOTA.begin();

  Serial.println("Ready");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());


  Wire.begin();
  ina3221.begin();
  bme.begin(0x76);
  display.init();
  display.flipScreenVertically();
  display.setFont(ArialMT_Plain_10);
}

void loop() {
  ArduinoOTA.handle();
  display.clear();
  display.setTextAlignment(TEXT_ALIGN_LEFT);
  display.setFont(ArialMT_Plain_10);
  display.drawString(0, 0, "T: " + String(bme.readTemperature() - 2));
  display.drawString(0, 10, "P: " + String(bme.readPressure()));

#ifdef KALIBRIERUNG
  display.drawString(0, 20, "U1: " + String(ina3221.getBusVoltage_V(1)) + "V" + String(ina3221.getShuntVoltage_mV(1)) + "mV");
  display.drawString(0, 30, "U2: " + String(ina3221.getBusVoltage_V(2)) + "V" + String(ina3221.getShuntVoltage_mV(2)) + "mV");
  display.drawString(0, 40, "U3: " + String(ina3221.getBusVoltage_V(3)) + "V" + String(ina3221.getShuntVoltage_mV(3)) + "mV");
#else
  //Anzeige des momentanen Stromes einfügen
  //Formel für I3 ist falsch
  display.drawString(0, 20, "I1: " + String(getCurrent(1)) + "A");
  display.drawString(0, 30, "I2: " + String(getCurrent(2)) + "A");
  display.drawString(0, 40, "I3: " + String(getCurrent(3)) + "A");
#endif
  display.drawString(0, 50, "5. OTA Update");
  display.display();
  delay(1000);
}


float getCurrent(int channel) {
  float currentCH1;
  float currentCH2;
  float currentCH3;
  float shuntVoltageCH1 = ina3221.getShuntVoltage_mV(1);
  float shuntVoltageCH2 = ina3221.getShuntVoltage_mV(2);
  float shuntVoltageCH3 = ina3221.getShuntVoltage_mV(3);
  currentCH1 = shuntVoltageCH1 / R1;
  shuntVoltageCH2 = shuntVoltageCH2 + currentCH1 / R2;
  currentCH2 = shuntVoltageCH2 / R3;
  shuntVoltageCH3 = shuntVoltageCH3 + currentCH1 / R2 + (currentCH1 + currentCH2) / R4;
  currentCH3 = shuntVoltageCH3 / R5;
  switch (channel) {
    case 1:
      return currentCH1;
    case 2:
      return currentCH2;
    case 3:
      return currentCH3;
  }
}
