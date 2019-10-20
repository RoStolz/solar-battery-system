#include "config.h"
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
  display.drawString(0, 20, "I1: " + String(-ina3221.getShuntVoltage_mV(1) / R1) + "A");
  display.drawString(0, 30, "I2: " + String(-(ina3221.getShuntVoltage_mV(2) + ((ina3221.getShuntVoltage_mV(1) / R1)*R2)) / R3) + "A");
  //display.drawString(0, 40, "I3+: " + String((ina3221.getShuntVoltage_mV(3) + ((ina3221.getShuntVoltage_mV(1) / R1)*R2) + ((1 * (ina3221.getShuntVoltage_mV(1) / R1) + ina3221.getShuntVoltage_mV(2) / R3)*R4)) / R5) + "A");
  //falsch ~1:6  display.drawString(0, 40, "I3+: " + String((ina3221.getShuntVoltage_mV(3) + ((ina3221.getShuntVoltage_mV(1) / R1)*R2) + ((ina3221.getShuntVoltage_mV(1) / R1 + ((ina3221.getShuntVoltage_mV(2) + ((ina3221.getShuntVoltage_mV(1) / R1)*R2)) / R3))*R4)) / R5) + "A");
  display.drawString(0, 40, "I32: " + String((ina3221.getShuntVoltage_mV(3)+(ina3221.getShuntVoltage_mV(2) + ((ina3221.getShuntVoltage_mV(1) / R1)*R2)) / R3*R4)/R5) + "A");
#endif
  display.drawString(0, 50, "5. OTA Update");
  display.display();
  delay(1000);
}
