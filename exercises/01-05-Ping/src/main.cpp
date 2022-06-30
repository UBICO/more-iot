#include <Arduino.h>
#include <WiFi.h>
#include <ESP32Ping.h>

char* essid = "MORE-IOT";
char* pwd = "MORE-IOT-PWD";

void setup() {
  Serial.begin(115200);
  WiFi.begin(essid,pwd);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.println("Connecting to WiFi...");
  }
}

void loop() {
  bool ret = Ping.ping("www.google.it",3);
  float time = Ping.averageTime();

  Serial.println(time);
  delay(5000);
}