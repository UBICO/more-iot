#include <Arduino.h>
#include <WiFi.h>
#include <ESP32Ping.h>

const char* ssid = "MORE-IOT";
const char* password = "MORE-IOT-PWD";

void setup() {
  Serial.begin(115200);
  WiFi.begin(ssid, password);

  while(WiFi.status() != WL_CONNECTED) { 
    delay(500);
    Serial.print(".");
  }
  Serial.println("Connected");
  Serial.print("My IP: ");
  Serial.println(WiFi.localIP());
}

void loop() {
  bool success = Ping.ping("192.168.0.1", 3);
  float avg_time_ms = Ping.averageTime();
  Serial.print("Average time: ");
  Serial.println(avg_time_ms);
}