// Load relevant libraries
#include <WiFiClientSecure.h>
#include <MQTTClient.h>  // Enable MQTT
#include <ArduinoJson.h> // Handle JSON messages
#include "uptime.h"      // Project library
#include "secrets.h"     // AWS IoT credentials

// Dual-core tasks
TaskHandle_t MQTTHandler;
TaskHandle_t CurrentHandler;

// Network clients
WiFiClientSecure net = WiFiClientSecure();
MQTTClient client = MQTTClient(JSON_SIZE);

const Settings settings = {
    16,
    128,
    4,
    20};

// Measurement parameters
const uint8_t samplingDelay = static_cast<uint8_t>(1000 / settings.fs / settings.smoothingFactor);

const char *topic;

const Sparkplug oldconfig = {
    "Armenta",
    "Home",
    "signal",
    "Bedford",
    "uptime"};

Device device{
    "okuma",
    "lb-ex",
    2019,
    "lathe"};

void messageHandler(String &topic, String &payload)
{
  Serial.println("incoming: " + topic + " - " + payload);
}

void connect_wifi()
{
  Serial.print("Connecting to Wi-Fi");
  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
    Serial.print(".");
  }
}

void connectAWS()
{
  connect_wifi();

  // Configure WiFiClientSecure to use the AWS IoT device credentials
  net.setCACert(AWS_CERT_CA);
  net.setCertificate(AWS_CERT_CRT);
  net.setPrivateKey(AWS_CERT_PRIVATE);

  // Connect to the MQTT broker on the AWS endpoint we defined earlier
  client.begin(MQTT_SERVER, MQTT_PORT, net);

  // Create a message handler
  client.onMessage(messageHandler);

  Serial.print("\rConnecting to AWS IOT");

  while (!client.connect(THINGNAME))
  {
    Serial.print(".");
    delay(100);
  }

  if (!client.connected())
  {
    Serial.println("\rAWS IoT Timeout!        ");
    return;
  }

  Serial.println("\rAWS IoT connected!          ");
}

// Auxiiliary Task
void MQTTProcess(void *pvParameters)
{
  /*
  Core 0 process
  Acquires signal sample and publishes
  measurements as JSON over MQTT
  */

  StaticJsonDocument<JSON_SIZE> mqttPayload;
  Measurement metrics;
  char buffer[JSON_SIZE];
  double readings[samples];
  double readingSamples;

  // Initialize global JSONmqttPayload
  mqttPayload["settings"]["fs"] = settings.fs;
  mqttPayload["settings"]["samples"] = settings.samples;
  mqttPayload["settings"]["a_lim"] = settings.a_lim;
  mqttPayload["device"]["manufacturer"] = device.manufacturer;
  mqttPayload["device"]["model"] = device.model;
  mqttPayload["device"]["year"] = device.year;
  mqttPayload["device"]["process"] = device.operation;

  size_t n;

  for (;;)
  {
    // Make sure MQTT client is connected
    if (!client.connected())
    {
      connectAWS();
    }
    else
    {
      // Get current signal sample
      for (int i = 0; i < samples; i++)
      {
        readingSamples = 0.0;
        // Average in-between readings to stabilize signal
        for (uint8_t j = 0; j < settings.smoothingFactor; j++)
        {
          readingSamples += get_current();
          // TODO Convert to uint8_t
          delay(samplingDelay);
        }
        readings[i] = readingSamples / settings.smoothingFactor;
      }

      // Analyze signal
      metrics = run_fft(readings, settings);

      mqttPayload["metrics"]["peak_hz"] = metrics.peak_hz;
      mqttPayload["metrics"]["energy"] = metrics.energy;
      mqttPayload["metrics"]["a_max"] = metrics.a_max;
      mqttPayload["metrics"]["a_rms"] = metrics.a_rms;
      mqttPayload["metrics"]["freq_max"] = metrics.freq_max;
      mqttPayload["metrics"]["freq_rms"] = metrics.freq_rms;

      // Prepare JSON message
      n = serializeJson(mqttPayload, buffer);

      if (client.publish(topic, buffer, n))
      {
        Serial.print("\rSent payload to topic \"");
        Serial.print(topic);
        Serial.print("\"");
      }
      else
      {
        Serial.println(buffer);
        Serial.print("Unable to send to ");
        Serial.println(topic);
      }

      client.loop();
      // DELETING THIS DELAY WILL CRASH THE MCU
      delay(20);
    }
  }
}

// TODO Convert to NFC / error handling process
void CurrentProcess(void *pvParameters)
{
  for (;;)
  {
    delay(1000);
  }
}

// Setup sequence
void setup()
{
  Serial.begin(115200);
  while (!Serial)
    ;
  delay(2000);

  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

  topic = read_nfc();

  // save_topic(topic);

  xTaskCreatePinnedToCore(
      MQTTProcess,  /* Task function. */
      "MQTT",       /* name of task. */
      10000,        /* Stack size of task */
      NULL,         /* parameter of the task */
      1,            /* priority of the task */
      &MQTTHandler, /* Task handle to keep track of created task */
      0);           /* pin task to core 0 */

  xTaskCreatePinnedToCore(
      CurrentProcess,  /* Task function. */
      "Current",       /* name of task. */
      10000,           /* Stack size of task */
      NULL,            /* parameter of the task */
      2,               /* priority of the task */
      &CurrentHandler, /* Task handle to keep track of created task */
      1);              /* pin task to core 1 */
}

// Main loop
void loop() {}
