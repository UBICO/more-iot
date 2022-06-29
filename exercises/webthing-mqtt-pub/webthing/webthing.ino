/**
 * Simple server compliant with Mozilla's proposed WoT API
 * Originally based on the HelloServer example
 * Tested on ESP8266, ESP32, Arduino boards with WINC1500 modules (shields or
 * MKR1000)
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <Arduino.h>
#include "Thing.h"
#include "WebThingAdapter.h"

#include "Adafruit_MQTT.h"
#include "Adafruit_MQTT_Client.h"

/**** MQTT ****/
#define AIO_SERVER      "192.168.0.254"
#define AIO_SERVERPORT  1883                   // use 8883 for SSL

WiFiClient client;
// or... use WiFiClientSecure for SSL
//WiFiClientSecure client;

// Setup the MQTT client class by passing in the WiFi client and MQTT server and login details.
Adafruit_MQTT_Client mqtt(&client, AIO_SERVER, AIO_SERVERPORT);
char host[] = "192.168.0.254";
char clientid[] = "curtain";
char username[] = "curtain";
char mqttpassword[] = "ledThingPwd";
char topicname[] = "devices/";

Adafruit_MQTT_Publish curtain_publish = Adafruit_MQTT_Publish(&mqtt, "/devices/actuators/curtain");
//void MQTT_connect();
/**** END MQTT ****/

/**** WiFi ****/
#include "secrets.h"
// Should define
// const char *ssid = "";
// const char *password = "-1"; 

/**** END WiFi ****/

#if defined(LED_BUILTIN)
const int ledPin = LED_BUILTIN;
#else
const int ledPin = 2; // manually configure LED pin
#endif


/**** WebThing ****/
WebThingAdapter *adapter;

const char *ledTypes[] = {"OnOffSwitch", "Light", nullptr};
ThingDevice led("led", "Built-in LED", ledTypes);
ThingProperty ledOn("on", "", BOOLEAN, "OnOffProperty");

/**** END WebThing ****/

bool lastOn = false;

void setup(void) {
  pinMode(ledPin, OUTPUT);
  digitalWrite(ledPin, HIGH);
  Serial.begin(115200);
  Serial.println("");
  Serial.print("Connecting to \"");
  Serial.print(ssid);
#if defined(ESP8266) || defined(ESP32)
  WiFi.mode(WIFI_STA);
#endif
  WiFi.begin(ssid, password);

  // Wait for connection
  bool blink = true;
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
    digitalWrite(ledPin, blink ? LOW : HIGH); // active low led
    blink = !blink;
  }
  digitalWrite(ledPin, HIGH); // active low led

  Serial.println("");
  Serial.print("Connected to ");
  Serial.print(ssid);
  Serial.print(". IP address: ");
  Serial.println(WiFi.localIP());
  adapter = new WebThingAdapter("w25", WiFi.localIP());

  led.addProperty(&ledOn);
  adapter->addDevice(&led);
  adapter->begin();
  Serial.print("HTTP server started. ");
  Serial.print("http://");
  Serial.print(WiFi.localIP());
  Serial.print("/things/");
  Serial.println(led.id);

  Serial.println("Registering to MQTT Broker");
  DynamicJsonDocument doc(1024);
  doc["name"]     = "curtain";
  doc["modelURI"] = "http://curtain.thing";
  doc["IP"]       = WiFi.localIP();
  char buf[256];
  serializeJson(doc, buf);

  MQTT_connect();

  // Publish our device to be discoverable
  while (! curtain_publish.publish(buf)) {
    Serial.println(F("Failed, retrying"));
  }
}

void loop(void) {
  MQTT_connect();
  
  adapter->update();
  bool on = ledOn.getValue().boolean;
  digitalWrite(ledPin, on ? HIGH : LOW); // active low led
  if (on != lastOn) {
    Serial.print(led.id);
    Serial.print(": ");
    Serial.println(on);
  }
  lastOn = on;
}

void MQTT_connect() {
  int8_t ret;

  // Stop if already connected.
  if (mqtt.connected()) {
    return;
  }

  Serial.print("Connecting to MQTT... ");

  uint8_t retries = 3;
  while ((ret = mqtt.connect()) != 0) { // connect will return 0 for connected
       Serial.println(mqtt.connectErrorString(ret));
       Serial.println("Retrying MQTT connection in 5 seconds...");
       mqtt.disconnect();
       delay(5000);  // wait 5 seconds
       retries--;
       if (retries == 0) {
         // basically die and wait for WDT to reset me
         while (1);
       }
  }
  Serial.println("MQTT Connected!");
}
