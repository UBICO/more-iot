#include <WiFi.h>
#include <WiFiUdp.h>
#include <coap-simple.h>

const char* ssid     = "MORE-IOT";
const char* password = "MORE-IOT-PWD";

// CoAP client response callback
void callback_response(CoapPacket &packet, IPAddress ip, int port);

// CoAP server endpoint url callback
void callback_light(CoapPacket &packet, IPAddress ip, int port);

// UDP and CoAP class
WiFiUDP udp;
Coap coap(udp);

// LED STATE
bool LEDSTATE;

void setup() {
  Serial.begin(115200);

  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
      delay(500);
      Serial.print(".");
  }

  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());

  // LED State
  pinMode(9, OUTPUT);
  digitalWrite(9, HIGH);
  LEDSTATE = true;
  
  // Add server url endpoint
  // You can add multiple endpoint URLs
  //      coap.server(callback_switch, "switch");
  //      coap.server(callback_env, "env/temp");
  //      coap.server(callback_env, "env/humidity");
  Serial.println("Setup Callback Light");
  // In fact, the registration server handles the callback function
  // Add the handler pointer and url to uri.add 
  coap.server(callback_light, "light");

  // Registers the callback function for the client response.
  // this endpoint is single callback.
  Serial.println("Setup Response Callback");
  // It is the same as above. In fact, it is to register the callback function pointer in resp
  coap.response(callback_response);

  // Start the soap server / client using the default port 5683 
  coap.start();
}

void loop() {
  // As a client, send a GET or put soap request to the soap server
  // Can be sent to another ESP32 
  // msgid = coap.put(IPAddress(192, 168, 128, 101), 5683, "light", "0");
  // msgid = coap.get(IPAddress(192, 168, 128, 101), 5683, "light");

  delay(1000);
  coap.loop();
}

// The CoAP server endpoint URL processes and responds to commands sent by the client
void callback_light(CoapPacket &packet, IPAddress ip, int port) 
{
  // This is a callback function that simulates the control lamp by receiving the command
  Serial.println("[Light] ON/OFF");
  Serial.println(packet.messageid);

  // Send response
  char p[packet.payloadlen + 1];
  memcpy(p, packet.payload, packet.payloadlen);
  p[packet.payloadlen] = NULL;
  
  String message(p);

  if (message.equals("0"))
    LEDSTATE = false;
  else if(message.equals("1"))
    LEDSTATE = true;
      
  if (LEDSTATE) {
    digitalWrite(9, HIGH) ; 
      Serial.println("[Light] ON");

    coap.sendResponse(ip, port, packet.messageid, "1");
  } else { 
    digitalWrite(9, LOW) ; 
    Serial.println("[Light] OFF");
    coap.sendResponse(ip, port, packet.messageid, "0");
  }
}

// CoAP client response callback
void callback_response(CoapPacket &packet, IPAddress ip, int port) 
{
  Serial.println("[Coap Response got]");
  
  char p[packet.payloadlen + 1];
  memcpy(p, packet.payload, packet.payloadlen);
  p[packet.payloadlen] = NULL;
  
  Serial.println(p);
}