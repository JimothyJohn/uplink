// Load relevant librariPrettyPrettyPrettyPrettyPrettyPrettyPrettyPrettyPrettyPrettyPrettyPrettyPrettyPrettyPrettyes
#include <Arduino.h>           //Base framework
#include <ESP_WiFiManager.h>   // AP login and maintenance
// #include <ArduinoOTA.h>        // Enable OTA updates
#include <ESPmDNS.h>      // Connect by hostname
#include <FS.h>           // Get FS functions
#include <SPIFFS.h>       // Enable file system
#include <PubSubClient.h> // Enable MQTT
#include <HTTPClient.h>   // Extract object data
#include <ArduinoJson.h>       // Handle JSON messages

#if defined(ESP32)
#define DEVICE "ESP32"
#elif defined(ESP8266)
#define DEVICE "ESP8266"
#endif

#define ACPIN 36
// set Non-invasive AC Current Sensor tection range
#define ACRANGE 20

// For Arduino Zero, Due, MKR Family, ESP32, etc. 3V3 controllers, change VREF to 3.3
#define VREF 3.3
#define FS 100
// Dual-core tasks
TaskHandle_t MQTTHandler;
TaskHandle_t CurrentHandler;

// Network clients
static WiFiClient wifiClient;
static PubSubClient pubsubClient(wifiClient);

// Current value
float ACCurrentValue = 0;

// Current time
unsigned long startTime = millis();
unsigned long currentTime = startTime;

// Initialize Wi-Fi manager and connect to Wi-Fi
// https://github.com/khoih-prog/ESP_WiFiManager
void SetupWiFi()
{
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
  // Add back hostname when pulling repo
  const char *hostname = "0.0.0.0";
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


float get_current() {
  float peakVoltage = analogRead(ACPIN);
  // change the peak voltage to the Virtual Value of voltage
  float voltageVirtualValue = peakVoltage * 0.707;
  /*The circuit is amplified by 2 times, so it is divided by 2.*/
  voltageVirtualValue = (voltageVirtualValue / 1024 * VREF) / 2;
  return voltageVirtualValue * ACRANGE;
}

// Auxiiliary Task
void MQTTProcess(void *pvParameters)
{
  Serial.print("MQTT task running on core ");
  Serial.println(xPortGetCoreID());
  
  char buffer[JSON_SIZE];
  StaticJsonDocument<JSON_SIZE> doc;
  float sampleFreq = 25;
  float sampleTime = 2;
  uint8_t samples = static_cast<uint8_t>(sampleFreq*sampleTime);
  uint8_t timeGap = static_cast<uint8_t>(1000 / sampleFreq);

  const char *topic = "/test";
  doc["device_name"] = "Olimex";
  doc["device_id"] = 0;
  doc["measurement"] = "current";
  doc["units"] = "A";
  doc["freq"] = sampleFreq;
  JsonArray readings = doc.createNestedArray("readings");
  
  for (;;)
  {
    // ArduinoOTA.handle()
    if (!pubsubClient.connected())
    {
      reconnect();
    } else
    {
      for (int i = 0; i < samples; i++)
      {
        // uint8_t normalCurrent = static_cast<uint8_t>(255/get_current());
        sprintf(buffer, "%.2f", get_current());
        readings[i] = buffer;
        delay(timeGap);
      }

      size_t n = serializeJson(doc, buffer);

      Serial.println(buffer);
      if (!pubsubClient.publish(topic, buffer, n)) {
        Serial.print("Unable to send to ");
      } else {
        Serial.print("Sent to ");
      }
      Serial.println(topic);
      pubsubClient.loop();
      // DELETING THIS DELAY COULD CRASH THE MCU
      Serial.print("Cycle time: ");
      Serial.println(millis());
      delay(1000);
    }
  }
}

void CurrentProcess(void *pvParameters)
{
  Serial.print("No task running on core ");
  Serial.println(xPortGetCoreID());

  for (;;)
  {
    delay(1000);
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

  xTaskCreatePinnedToCore(
      MQTTProcess,   /* Task function. */
      "MQTT", /* name of task. */
      10000,       /* Stack size of task */
      NULL,        /* parameter of the task */
      2,           /* priority of the task */
      &MQTTHandler,      /* Task handle to keep track of created task */
      0);          /* pin task to core 0 */

  xTaskCreatePinnedToCore(
      CurrentProcess,   /* Task function. */
      "Current", /* name of task. */
      10000,       /* Stack size of task */
      NULL,        /* parameter of the task */
      1,           /* priority of the task */
      &CurrentHandler,      /* Task handle to keep track of created task */
      1);          /* pin task to core 1 */
}

// Main loop
void loop() {}
