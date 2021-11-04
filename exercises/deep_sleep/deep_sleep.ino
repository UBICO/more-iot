#define MILLIS 1000000
#define TIME_TO_SLEEP  30        

RTC_DATA_ATTR int boot = 0;

void setup(){
  Serial.begin(9600);
  delay(50); 

  boot++;
  Serial.println("Boot number: " + String(boot));

  esp_sleep_enable_timer_wakeup(TIME_TO_SLEEP * MILLIS);
  Serial.println("Sleeping for " + String(TIME_TO_SLEEP) + " Seconds");

  Serial.println("Sleep.start()");
  delay(10);
  Serial.flush(); 
  esp_deep_sleep_start();
  
}

void loop(){}
