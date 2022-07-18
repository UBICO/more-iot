#include <Arduino.h>
#include <WiFi.h>
#include <WebThingAdapter.h>
#include "DHT.h"

#define RED_PIN 19
#define BLUE_PIN 23
#define GREEN_PIN 18

#define DHTPIN 4     
#define DHTTYPE DHT11

DHT dht(DHTPIN, DHTTYPE);

const char* ssid = "MORE-IOT";
const char* password = "MORE-IOT-PWD";

WebThingAdapter *adapter;

const char *ledTypes[] = {"OnOffSwitch", "Light", nullptr};
ThingDevice led("led", "Built-in LED", ledTypes);

ThingProperty ledOn("on", "", BOOLEAN, "OnOffProperty");

const char* sensorTypes[] = {"TemperatureSensor", nullptr};
ThingDevice sensor("environment", "Environment sensors", sensorTypes);
ThingProperty sensorTemp("temperature", "", NUMBER, "TemperatureProperty");
ThingProperty sensorHumd("humidity", "", NUMBER, nullptr);

bool lastOn;

void setup() {
  Serial.begin(115200);
  WiFi.begin(ssid, password);
  Serial.print("Connecting WiFi");
  while (!WiFi.isConnected()) {
    Serial.print(".");
    delay(500);
  }
  Serial.println(" WiFi Connected.");

  pinMode(RED_PIN, OUTPUT);
  pinMode(BLUE_PIN, OUTPUT);
  pinMode(GREEN_PIN, OUTPUT);

  adapter = new WebThingAdapter("w25", WiFi.localIP());

  led.addProperty(&ledOn);
  adapter->addDevice(&led);

  sensor.addProperty(&sensorTemp);
  sensor.addProperty(&sensorHumd);
  adapter->addDevice(&sensor);
  
  adapter->begin();

  Serial.println("HTTP server started");
  Serial.print("http://");
  Serial.print(WiFi.localIP());
  Serial.print("/things/");
  Serial.println(led.id);
}

void loop(void) {
  float temp, humd;
  ThingPropertyValue Val;

  adapter->update();

  bool on = ledOn.getValue().boolean;
  digitalWrite(RED_PIN, on ? LOW : HIGH); // active low led
  if (on != lastOn) {
    Serial.print(led.id);
    Serial.print(": ");
    Serial.println(on);
    if (on) {
      digitalWrite(RED_PIN, LOW);
    } else {
      digitalWrite(RED_PIN, HIGH);
    }
  }
  lastOn = on;

  temp = dht.readTemperature();
  humd = dht.readHumidity();

  Val.number = temp;
  sensorTemp.setValue(Val);

  Val.number = humd;
  sensorHumd.setValue(Val);

  adapter->update();
}