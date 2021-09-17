//KY018 Photo resistor module
 
int sensorPin = 2; // select the input pin for the potentiometer
int sensorValue = 0; // variable to store the value coming from the sensor
void setup() {
  Serial.begin(9600);
}
void loop() {
  sensorValue = analogRead(sensorPin);
  
  Serial.println(sensorValue, DEC);
  delay(2000);
}
