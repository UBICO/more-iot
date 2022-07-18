#include <SPI.h>
#include <WiFi.h>
#include <PubSubClient.h>

const char* ssid = "MORE-IOT";
const char* password = "MORE-IOT-PWD";
const char* mqtt_server = "192.168.1.185";
WiFiClient espClient;
//PubSubClient client(espClient);
PubSubClient client(espClient);

void callback(char* topic, byte* payload, unsigned int length) {
  // Allocate the correct amount of memory for the payload copy
  byte* p = (byte*)malloc(length);
  // Copy the payload to the new buffer
  memcpy(p,payload,length);
  client.publish("outTopic", p, length);
  // Free the memory
  free(p);
}

void setup()
{
  Serial.begin(9600);
  WiFi.disconnect();

  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
      delay(500);
      Serial.print(".");
  }

  Serial.println("Now connected");
  client.setServer(mqtt_server, 1883);

  Serial.print("Connecting to MQTT broker.");
  while (!client.connect("arduinoClient")) {
    Serial.print(".");
    delay(1000);
  }
  Serial.println();
  client.publish("outTopic","hello world");
  client.subscribe("inTopic");
  
  Serial.println("Client connected, subscribing to topic.");
 

}

void loop()
{
  client.loop();
}