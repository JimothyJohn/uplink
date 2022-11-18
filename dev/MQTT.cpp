// Load relevant libraries
#include <Arduino.h>           //Base framework
#include <ESP_WiFiManager.h>   // AP login and maintenance
#include <AsyncTCP.h>          // Generic async library
#include <ESPAsyncWebServer.h> // ESP32 async library
// #include <ArduinoOTA.h>        // Enable OTA updates
#include <ESPmDNS.h>      // Connect by hostname
#include <FS.h>           // Get FS functions
#include <SPIFFS.h>       // Enable file system
#include <PubSubClient.h> // Enable MQTT
#include <ArduinoJson.h>  // Handle JSON messages
#include <HTTPClient.h>   // Extract object data

#if defined(ESP32)
#define DEVICE "ESP32"
#elif defined(ESP8266)
#define DEVICE "ESP8266"
#endif

#if defined(DEV_BUILD)
#define INCLUDE_LIBS true
#else
#define INCLUDE_LIBS false
#endif

// Dual-core tasks
TaskHandle_t Task1;

// Network clients
static WiFiClient wifiClient;
static PubSubClient pubsubClient(wifiClient);

// Initialize Wi-Fi manager and connect to Wi-Fi
// https://github.com/khoih-prog/ESP_WiFiManager
void SetupWiFi()
{
  Serial.print(F("\nStarting AutoConnect_ESP32_minimal on "));
  Serial.println(ARDUINO_BOARD);
  Serial.println(ESP_WIFIMANAGER_VERSION);
  ESP_WiFiManager wm("uServer");
  wm.autoConnect("uServer");
  if (WiFi.status() == WL_CONNECTED)
  {
    Serial.print(F("Connected. Local IP: "));
    Serial.println(WiFi.localIP());
  }
  else
  {
    Serial.println(wm.getStatus(WiFi.status()));
  }
}

// MQTT Handling functions
// https://github.com/knolleary/pubsubclient/blob/master/examples/mqtt_basic/mqtt_basic.ino
void callback(char *topic, uint8_t *payload, unsigned int length)
{
  // Create spot in memory for message
  char message[length + 1];
  // Write payload to String
  strncpy(message, (char *)payload, length);
  // Nullify last character to eliminate garbage at end
  message[length] = '\0';

  // Create correct object
  Serial.print("Received \"");
  Serial.print(message);
  Serial.print("\" from ");
  Serial.println(topic);
}

void reconnect()
{
  const char *hostname = "broker.hivemq.com";
  pubsubClient.setServer(hostname, 1883);
  pubsubClient.setCallback(callback);
  
  Serial.print("Attempting MQTT connection...");
  // Create a random client ID
  String clientId = "ESP32-";
  clientId += String(random(0xffff), HEX);
  // Attempt to connect
  if (pubsubClient.connect(clientId.c_str()))
  {
    Serial.println("connected");
    // Once connected, publish an announcement...
    pubsubClient.publish("/home/status/esp32", "Hello world!");
  }
  else
  {
    Serial.print("failed, rc=");
    Serial.print(pubsubClient.state());
  }
}


// Auxiiliary Task
void MQTTHandler(void *pvParameters)
{
  Serial.print("Auxiliary task running on core ");
  Serial.println(xPortGetCoreID());

  for (;;)
  {
    // ArduinoOTA.handle();
    if (!pubsubClient.connected())
    {
      reconnect();
    } else
    {
      const char *payload = "payload";
      const char *topic = "/test/topic";
      if (!pubsubClient.publish(topic, payload)) {
        Serial.print("Unable to send");
        Serial.print(payload);
        Serial.print(" to ");
        Serial.println(topic);
      } else {
        Serial.print("Sent");
        Serial.print(payload);
        Serial.print(" to ");
        Serial.println(topic);
      }
      pubsubClient.loop();
      // DELETING THIS DELAY COULD CRASH THE MCU
      delay(1000);
    }
  }
}

// Setup sequence
void setup()
{
  // Start serial server and connect to WiFi
  delay(200);
  Serial.begin(115200);
  while (!Serial)
    ;

  delay(2000); // pwrLevel-up safety delay

  // Uses soft AP to connect to Wi-Fi (if saved credentials aren't valid)
  SetupWiFi();

  // mDNS allows for connection at http://userver.local/
  if (!MDNS.begin("userver"))
  {
    Serial.println("Error starting mDNS!");
    ESP.restart();
  }

  // Initialize SPIFFS
  if (!SPIFFS.begin(true))
  {
    Serial.println("An error has occurred while mounting SPIFFS");
    ESP.restart();
  }

  //create a task that will be executed in the MQTTHandler() function, with priority 1 and executed on core 0
  xTaskCreatePinnedToCore(
      MQTTHandler,   /* Task function. */
      "Auxiliary", /* name of task. */
      10000,       /* Stack size of task */
      NULL,        /* parameter of the task */
      1,           /* priority of the task */
      &Task1,      /* Task handle to keep track of created task */
      0);          /* pin task to core 0 */
}

// Main loop
void loop() {}
