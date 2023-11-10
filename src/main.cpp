// Load relevant libraries
#include <Wire.h>
#include <WiFiClientSecure.h>
#include <ETH.h>
#include <MQTTClient.h>  // Enable MQTT
#include <ArduinoJson.h> // Handle JSON messages
#include "uplink.h"      // Project library
#include "secrets.h"     // AWS IoT credentials

// Dual-core tasks
TaskHandle_t MQTTHandler;
TaskHandle_t CurrentHandler;

// Network clients
WiFiClientSecure net = WiFiClientSecure();
MQTTClient client = MQTTClient(JSON_SIZE);

// MQTT Message
StaticJsonDocument<JSON_SIZE> payloadDoc;

// Default measurement settings
const Settings settings = {
    16,
    128,
    4,
    20};

// Measurement parameters
const uint8_t samplingDelay = static_cast<uint8_t>(1000 / settings.fs / settings.smoothingFactor);

// Ethernet event handler
void EthEvent(WiFiEvent_t event)
{
  switch (event)
  {
  case ARDUINO_EVENT_ETH_START:
    ETH.setHostname(THING_NAME);
    break;
  case ARDUINO_EVENT_ETH_CONNECTED:
    break;
  case ARDUINO_EVENT_ETH_GOT_IP:
    Serial.print("\rConnected to network!");
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

// MQTT Broker connection
uint8_t connectAWS()
{
  Serial.print("Connecting to network...");
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

  // Create a message handler
  client.onMessage(messageHandler);

  Serial.print("\rConnecting to AWS IOT with client ID: ");
  Serial.print(THING_NAME);
  Serial.print("...");
  while (!client.connect(THING_NAME))
  {
    Serial.print(".");
    delay(1000);
  }

  if (!client.connected())
  {
    // Leave space at end to clear out previous message
    Serial.println("\rAWS IoT Timeout!                ");
    return 0;
  }

  Serial.print("\rConnected to AWS IOT with client ID: ");
  Serial.print(THING_NAME);
  Serial.print("!                   ");

  return 1;
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
  char buffer[JSON_SIZE];
  double readings[samples];
  double readingSamples;

  // Initialize global JSONpayloadDoc
  payloadDoc["settings"]["fs"] = settings.fs;
  payloadDoc["settings"]["samples"] = settings.samples;
  payloadDoc["settings"]["a_lim"] = settings.a_lim;

  size_t payloadSize;

  char tagData[TOPIC_LENGTH];
  const char *topic;

  for (;;)
  {
    // Make sure MQTT client is connected
    if (!client.connected())
    {
      if (!connectAWS())
      {
        Serial.println("Unable to connect to AWS!");
      }
    }
    else
    {
      // Read topic from NFC tag
      if (!read_nfc(tagData))
      {
        delay(1000);
        continue;
      }
      topic = tagData;

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

      // Test metrics (replace with code above)
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

      // Update <etrics
      payloadDoc["metrics"]["energy"] = metrics.energy;
      payloadDoc["metrics"]["a_max"] = metrics.a_max;
      payloadDoc["metrics"]["a_rms"] = metrics.a_rms;
      payloadDoc["metrics"]["freq_max"] = metrics.freq_max;
      payloadDoc["metrics"]["freq_rms"] = metrics.freq_rms;
      payloadDoc["metrics"]["peak_hz"] = metrics.peak_hz;

      // Prepare JSON message
      payloadSize = serializeJson(payloadDoc, buffer);

      if (client.publish(topic, buffer, payloadSize))
      {
        // Print 24 spaces to clear the line
        Serial.print("\r                                            ");
        Serial.print("\rSending payload to topic \"");
        Serial.print(topic);
        Serial.print('"');
      }
      else
      {
        Serial.print("\rUnable to send ");
        Serial.print(buffer);
        Serial.print(" to \"");
        Serial.print(topic);
        Serial.print("\"!            ");
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

  WiFi.onEvent(EthEvent);
  ETH.begin();

  // Initialize I2C bus with SDA_PIN and SCL_PIN
  Wire.begin(SDA_PIN, SCL_PIN);

  // Figure out a use for the second core
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
