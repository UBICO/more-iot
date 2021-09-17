#include <DHTesp.h>

#define SENSOR_PIN 19

DHTesp dht;

void setup() {
  Serial.begin(9600);
  dht.setup(SENSOR_PIN, DHTesp::DHT11);
}

void loop() {
  
  TempAndHumidity val = dht.getTempAndHumidity();
  Serial.print("Temperature ");
  Serial.println(val.temperature); 

  Serial.print("Humidity ");
  Serial.println(val.humidity); 
  
  delay(2000);
}
