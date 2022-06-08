#include <Arduino.h>

//KY018 Photo resistor module
 
int sensorPin = 4; // select the input pin for the potentiometer
int sensorValue = 0; // variable to store the value coming from the sensor
int LED = 2;

void setup() {
  Serial.begin(115200);
  pinMode(LED, OUTPUT);
}

void loop() {
  sensorValue = analogRead(sensorPin);
  
  if (sensorValue < 200) {
    digitalWrite(LED, HIGH);
  } else {
    digitalWrite(LED, LOW);
  }

  Serial.println(sensorValue, DEC);
  delay(2000);
}