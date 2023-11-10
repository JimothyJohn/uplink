// https://github.com/OLIMEX/ESP32-POE/blob/master/SOFTWARE/ARDUINO/ESP32_PoE_Ethernet_Arduino/ESP32_PoE_Ethernet_Arduino.ino
/*
    This sketch shows the Ethernet event usage
*/
#include <Arduino.h>
#include <ETH.h>
#include <WiFiClientSecure.h>
#include <MQTTClient.h>  // Enable MQTT
#include "secrets.h"     // AWS IoT credentials

static uint16_t MQTT_PORT = 8883;
static const uint16_t JSON_SIZE = 512;

// Network clients
WiFiClientSecure net = WiFiClientSecure();
MQTTClient client = MQTTClient(JSON_SIZE);


static bool eth_connected = false;

void WiFiEvent(WiFiEvent_t event)
{
  switch (event) {
    case ARDUINO_EVENT_ETH_START:
      Serial.println("ETH Started");
      //set eth hostname here
      ETH.setHostname("esp32-ethernet");
      break;
    case ARDUINO_EVENT_ETH_CONNECTED:
      Serial.println("ETH Connected");
      break;
    case ARDUINO_EVENT_ETH_GOT_IP:
      Serial.print("ETH MAC: ");
      Serial.print(ETH.macAddress());
      Serial.print(", IPv4: ");
      Serial.print(ETH.localIP());
      if (ETH.fullDuplex()) {
        Serial.print(", FULL_DUPLEX");
      }
      Serial.print(", ");
      Serial.print(ETH.linkSpeed());
      Serial.println("Mbps");
      eth_connected = true;
      break;
    case ARDUINO_EVENT_ETH_DISCONNECTED:
      Serial.println("ETH Disconnected");
      eth_connected = false;
      break;
    case ARDUINO_EVENT_ETH_STOP:
      Serial.println("ETH Stopped");
      eth_connected = false;
      break;
    default:
      break;
  }
}

void testClient(const char * host, uint16_t port)
{
  Serial.print("\nconnecting to ");
  Serial.println(host);

  WiFiClient client;
  if (!client.connect(host, port)) {
    Serial.println("connection failed");
    return;
  }
  client.printf("GET / HTTP/1.1\r\nHost: %s\r\n\r\n", host);
  while (client.connected() && !client.available());
  while (client.available()) {
    Serial.write(client.read());
  }

  Serial.println("closing connection\n");
  client.stop();
}

uint8_t connectAWS()
{
  Serial.print("\rConnecting to Wi-Fi...");
  while (!eth_connected)
  {
    Serial.print(".");
    delay(1000);
  }

  // Configure WiFiClientSecure to use the AWS IoT device credentials
  net.setCACert(AWS_CERT_CA);
  net.setCertificate(AWS_CERT_CRT);
  net.setPrivateKey(AWS_CERT_PRIVATE);

  // Connect to the MQTT broker on the AWS endpoint we defined earlier
  client.begin(MQTT_SERVER, MQTT_PORT, net);

  Serial.print("\rConnecting to AWS IOT with client ID: ");
  Serial.print(THING_NAME);
  while (!client.connect(THING_NAME))
  {
    Serial.print(".");
    delay(1000);
  }

  if (!client.connected())
  {
    // Leave space at end to clear out previous message
    Serial.println("\rAWS IoT Timeout!        ");
    return 0;
  }

  Serial.print("\rConnected to AWS IOT with client ID: ");
  Serial.println(THING_NAME);

  return 1;
}

void setup()
{
  Serial.begin(115200);
  WiFi.onEvent(WiFiEvent);
  ETH.begin();
}


void loop()
{
  Serial.print("ETH MAC: ");
  Serial.print(ETH.macAddress());
  Serial.print(", IPv4: ");
  Serial.println(ETH.localIP());
    if (!client.connected())
    {
        connectAWS();
    }
  delay(1000);
}