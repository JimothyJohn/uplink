// Load relevant libraries
#include <WiFiClientSecure.h>
#include <MQTTClient.h>  // Enable MQTT
#include <ArduinoJson.h> // Handle JSON messages
#include <EEPROM.h>      // Handle JSON messages
#include "uptime.h"      // Project library
#include "secrets.h"     // AWS IoT credentials

// Dual-core tasks
TaskHandle_t MQTTHandler;
TaskHandle_t CurrentHandler;

// Network clients
WiFiClientSecure net = WiFiClientSecure();
MQTTClient client = MQTTClient(JSON_SIZE);

StaticJsonDocument<JSON_SIZE> payloadDoc;

const Settings settings = {
    16,
    128,
    4,
    20};

// Measurement parameters
const uint8_t samplingDelay = static_cast<uint8_t>(1000 / settings.fs / settings.smoothingFactor);

// char *topic[];
char tagData[TOPIC_LENGTH];
const char *topic;

void update_device(Device device)
{
  payloadDoc["device"]["manufacturer"] = device.manufacturer;
  payloadDoc["device"]["model"] = device.model;
  payloadDoc["device"]["year"] = device.year;
  payloadDoc["device"]["process"] = device.operation;
  EEPROM.begin(EEPROM_SIZE);
  EEPROM.put(Config::DEVICE, device);
  EEPROM.commit();
  EEPROM.end();
}

void update_metrics(Measurement metrics)
{
  payloadDoc["metrics"]["energy"] = metrics.energy;
  payloadDoc["metrics"]["a_max"] = metrics.a_max;
  payloadDoc["metrics"]["a_rms"] = metrics.a_rms;
  payloadDoc["metrics"]["freq_max"] = metrics.freq_max;
  payloadDoc["metrics"]["freq_rms"] = metrics.freq_rms;
  payloadDoc["metrics"]["peak_hz"] = metrics.peak_hz;
}

void update_settings(Settings settings)
{
  payloadDoc["settings"]["fs"] = settings.fs;
  payloadDoc["settings"]["samples"] = settings.samples;
  payloadDoc["settings"]["a_lim"] = settings.a_lim;
  EEPROM.begin(EEPROM_SIZE);
  EEPROM.put(Config::SETTINGS, settings);
  EEPROM.commit();
  EEPROM.end();
}

void update_topic(const char *topic)
{
  char topicsections[][32] = {"", "", "", "", ""};
  uint8_t n = strlen(topic);
  uint8_t parsedChars = 0;
  uint8_t index = 0;

  for (uint8_t i = 0; i < n; i++)
  {
    // Only grab non forward slash characters
    if (topic[i] != '/')
    {
      // Add character to array index then index
      topicsections[index][parsedChars] = topic[i];
      parsedChars++;
    }
    else
    {
      // Increase index and reset parsing counter
      index++;
      parsedChars = 0;
    }
  }

  Sparkplug newTopic = {
      *topicsections[0],
      *topicsections[1],
      *topicsections[2],
      *topicsections[3],
      *topicsections[4],
  };

  payloadDoc["config"]["namespace"] = topicsections[0];
  payloadDoc["config"]["group_id"] = topicsections[1];
  payloadDoc["config"]["message_type"] = topicsections[2];
  payloadDoc["config"]["edge_node_id"] = topicsections[3];
  payloadDoc["config"]["device_id"] = topicsections[4];

  EEPROM.begin(EEPROM_SIZE);
  EEPROM.put(Config::TOPIC, newTopic);
  EEPROM.commit();
  EEPROM.end();
}

void messageHandler(String &topic, String &payload)
{
  StaticJsonDocument<JSON_SIZE> devicePayload;
  size_t n = sizeof(payload);
  char json_payload[n];

  DeserializationError error = deserializeJson(devicePayload, payload);
  if (error.c_str() != "Ok")
  {
    Serial.print("JSON Deserialization error: ");
    Serial.println(error.c_str());
    Serial.println("JSON payload: ");
    Serial.println(json_payload);
  }

  EEPROM.begin(EEPROM_SIZE);

  if (topic == "Armenta/Home/cmd/Bedford/uptime")
  {
    Device device = {
        devicePayload["manufacturer"].as<String>(),
        devicePayload["model"].as<String>(),
        devicePayload["year"].as<uint16_t>(),
        devicePayload["operation"].as<String>()};

    EEPROM.put(0, device);
    update_device(device);
  }

  EEPROM.commit();
  EEPROM.end();
}

uint8_t connectAWS()
{
  while (WiFi.status() != WL_CONNECTED)
  {
    Serial.print("\rConnecting to Wi-Fi...");
    delay(1000);
  }

  // Configure WiFiClientSecure to use the AWS IoT device credentials
  net.setCACert(AWS_CERT_CA);
  net.setCertificate(AWS_CERT_CRT);
  net.setPrivateKey(AWS_CERT_PRIVATE);

  // Connect to the MQTT broker on the AWS endpoint we defined earlier
  client.begin(MQTT_SERVER, MQTT_PORT, net);

  // Create a message handler
  client.onMessage(messageHandler);

  while (!client.connect(THING_NAME))
  {
    Serial.print("\rConnecting to AWS IOT with client ID \"");
    Serial.print(THING_NAME);
    Serial.print("\"...");
    delay(1000);
  }

  if (!client.connected())
  {
    Serial.println("\rAWS IoT Timeout!        ");
    return 1;
  }

  client.subscribe("uptime/*");
  Serial.print("\rConnected to AWS IoT with client ID ");
  Serial.print(THING_NAME);
  Serial.print("!");
  return 0;
}

// Auxiiliary Task
void MQTTProcess(void *pvParameters)
{
  /*
  Core 0 process
  Acquires signal sample and publishes
  measurements as JSON over MQTT
  */

  Measurement metrics;
  Device device;
  EEPROM.begin(EEPROM_SIZE);
  EEPROM.get(0, device);
  EEPROM.end();
  char buffer[JSON_SIZE];
  double readings[samples];
  double readingSamples;
  uint32_t payloadCount = 0;

  // Initialize global JSONpayloadDoc
  update_settings(settings);
  update_device(device);

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
      // metrics = run_fft(readings, settings);
      // Test metrics
      // Creates a random float between 0 and 3
      double samples[] = {0.0, 1.0, 5.0};
      uint8_t randInt = random(0, 3);
      metrics = Measurement{
          0.0,
          0.0,
          samples[randInt],
          0.0,
          0.0,
          0.0};

      update_metrics(metrics);

      // Prepare JSON message
      n = serializeJson(payloadDoc, buffer);

      if (client.publish(topic, buffer, n))
      {
        payloadCount++;
        Serial.print("\rSent payload to topic \"");
        Serial.print(topic);
        Serial.print("\" ");
        Serial.print(payloadCount);
        Serial.print(" times...");
      }
      else
      {
        Serial.print("\rUnable to send to ");
        Serial.print(buffer);
        Serial.print(" to ");
        Serial.print(topic);
        Serial.println("\"!");
      }
      client.loop();
      // DELETING THIS DELAY WILL CRASH THE MCU
      delay(10);
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

  // TODO - Move into main() loop when tag is updated
  read_nfc(tagData);
  topic = tagData;
  Serial.print("Topic: ");
  Serial.println(topic);
  // update_topic(topic);

  // Figure out a use for the second core
  xTaskCreatePinnedToCore(MQTTProcess,  /* Task function. */
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
