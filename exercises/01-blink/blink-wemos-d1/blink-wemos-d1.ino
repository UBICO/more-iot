void setup() {
  Serial.begin(9600);
  pinMode(LED_BUILTIN, OUTPUT);
}

void loop() {
  Serial.println("--- Another loop ---");
  for (int i=0; i < 1000; i+=100) {
    digitalWrite(LED_BUILTIN, HIGH);  
    Serial.print("Switching on and waiting for ");
    Serial.print(i); 
    Serial.println(" milliseconds");
    delay(i);               
    digitalWrite(LED_BUILTIN, LOW);
    Serial.print("Switching off and waiting for ");
    Serial.print(i); 
    Serial.println(" milliseconds");   
    delay(i);                 
  }
}
