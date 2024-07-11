#include <ESP8266WiFi.h>
#include <WiFiManager.h>
#include <ArduinoJson.h>
#include <FastBot.h>
#include <EEPROM.h>
#include <OneButton.h>
#include <ESP8266HTTPClient.h>
#include <ESP8266httpUpdate.h>

const char *ssid = "Telecom-15744621";
const char *password = "fZET6ouLwc3wYG6WyfDy4fUL";
const char* firmware_url = "https://github.com/Cheic1/RL/releases/download/V01_00_00/firmware.bin";



void checkForUpdates() ;

void setup() {
  Serial.begin(9600);
  Serial.println();
  Serial.println("Booting...");

  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());

  checkForUpdates();
}

void loop() {
  // Il tuo codice principale qui
}

void checkForUpdates() {
  WiFiClient client;
  client.set
  t_httpUpdate_return ret = ESPhttpUpdate.update(client, firmware_url);

  switch (ret) {
    case HTTP_UPDATE_FAILED:
      Serial.printf("HTTP_UPDATE_FAILED Error (%d): %s\n", ESPhttpUpdate.getLastError(), ESPhttpUpdate.getLastErrorString().c_str());
      break;

    case HTTP_UPDATE_NO_UPDATES:
      Serial.println("HTTP_UPDATE_NO_UPDATES");
      break;

    case HTTP_UPDATE_OK:
      Serial.println("HTTP_UPDATE_OK");
      break;
  }
}