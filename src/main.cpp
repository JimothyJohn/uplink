// Load relevant librariPrettyPrettyPrettyPrettyPrettyPrettyPrettyPrettyPrettyPrettyPrettyPrettyPrettyPrettyPrettyes
#include <Arduino.h>         //Base framework
// TODO: Security - Remove Wi-Fi functionality
#include <ESP_WiFiManager.h> // AP login and maintenance
#include <ESPmDNS.h>         // Connect by hostname
// Load relevant libraries
#include <FS.h>              // Get FS functions
#include <SPIFFS.h>          // Enable file system
#include <PubSubClient.h>    // Enable MQTT
#include <HTTPClient.h>      // Extract object data
#include <ArduinoJson.h>     // Handle JSON messages
#include <arduinoFFT.h>      // Spectrum analysis
#include "certs.h"

// Current sensor detection range
#define ACRANGE 20

// For Arduino Zero, Due, MKR Family, ESP32, etc. 3V3 controllers, change VREF to 3.3
#define VREF 3.3

// Allow for larger JSON and MQTT messages
#define JSON_SIZE 1024
#define MQTT_MAX_PACKET_SIZE 1024

// Allow for more verbose outputs
#if DEBUG
// Simulated signal index types 
#define SCL_INDEX 0x00
#define SCL_TIME 0x01
#define SCL_FREQUENCY 0x02
#define SCL_PLOT 0x03
#endif

#if defined(ESP32)
#define DEVICE "ESP32"
#elif defined(ESP8266)
#define DEVICE "ESP8266"
#endif

// Set pin for current sensor (must be ADC DIO, not CH2)
const uint8_t ACPIN = 32;

// Dual-core tasks
TaskHandle_t MQTTHandler;
TaskHandle_t CurrentHandler;

// Network clients
// TODO Replace Wi-Fi with Ethernet client
static WiFiClient wifiClient;
static PubSubClient pubsubClient(wifiClient);
uint16_t port = 1883;

// Simulated signal frequency (DEBUG only)
const float signalFrequency = 5.567;
const uint8_t amplitude = 100;
// Measurement parameters
const float samplingFrequency = 16;
const uint8_t smoothingFactor = 4;
const uint8_t samplingDelay = static_cast<uint8_t>(1000 / samplingFrequency / smoothingFactor);
const uint16_t samples = 128; // This value MUST ALWAYS be a power of 2

//Initialize measurements
float energy;
double a_max;
double a_rms;
double freq_max;
double freq_rms;
double peak_hz;

// Sparkplug B Topic Initalizer
// namespace = company name
char nspace[32] = "Armenta";
// grouo_id = business unit
char group_id[32] = "Home";
char message_type[32] = "signal";
// edge_node_id = location
char edge_node_id[32] = "Bedford";
// device_id = machine name 
char device_id[32] = "uptime";
char topic[128];
 
void print_index(uint8_t scaleType)
{
  /*
  Takes an index type argument and 
  prints a CSV line of values
  */
  float abscissa;
  uint8_t finalSamples = samples;
  // Print abscissa value
  switch (scaleType)
  {
  case SCL_INDEX:
    Serial.print("Index,");
    break;
  case SCL_TIME:
    Serial.print("Time (ms),");
    break;
  case SCL_FREQUENCY:
    Serial.print("Frequency (Hz),");
    finalSamples /= 2;
    break;
  }

  for (uint16_t i = 0; i < finalSamples; i++)
  {
    // Print abscissa value
    switch (scaleType)
    {
    case SCL_INDEX:
      abscissa = (i * 1.0);
      break;
    case SCL_TIME:
      abscissa = ((i * 1.0) / samplingFrequency);
      break;
    case SCL_FREQUENCY:
      abscissa = ((i * 1.0 * samplingFrequency) / finalSamples);
      break;
    }
    Serial.print(abscissa, 2);
    Serial.print(",");
  }
  Serial.println();
}

void PrintVector(double vData[samples], uint16_t bufferSize, uint8_t scaleType)
{
  /*
  Takes an array of floats and outputs as a
  CSV-formatted line over serial
  */
  float sentValue;
  for (uint16_t i = 0; i < bufferSize; i++)
  {
    sentValue = vData[i];
    if (scaleType == SCL_FREQUENCY)
    {
      sentValue /= 10;
    }
    Serial.print(sentValue, 2);
    Serial.print(",");
  }
  Serial.println();
}

// https://github.com/kosme/arduinoFFT/blob/master/Examples/FFT_01/FFT_01.ino
void run_fft(double vReal[samples])
{
  /*
  Takes an array and writes spectrum
  characteristics to global JSON doc
  */
  arduinoFFT FFT = arduinoFFT(); /* Create FFT object */

  // Initialize variables
  double vImag[samples];
  energy = 0.0;
  a_max = 0.0;
  a_rms = 0.0;
  freq_max = 0.0;
  freq_rms = 0.0;

  // Build raw data
  // Number of signal cycles that the sampling will read
  float cycles = (((samples - 1) * signalFrequency) / samplingFrequency);  // Find power characteristics
  for (uint16_t i = 0; i < samples; i++)
  {
    // Create simulated signal with some modulation
    vReal[i] = int8_t((amplitude * (sin((i * (twoPi * cycles)) / samples))) / 2.0); /* Build data with positive and negative values*/
    vReal[i] += int8_t((amplitude/5 * (sin((4 * i * (twoPi * cycles)) / samples))) / 2.0); /* Build data with positive and negative values*/
    vReal[i] -= int8_t((amplitude/2 * (sin((0.2 + 5 * i * (twoPi * cycles)) / samples))) / 2.0); /* Build data with positive and negative values*/
    // Build data displaced on the Y axis to include only positive values
    // vReal[i] = uint8_t((amplitude * (sin((i * (twoPi * cycles)) / samples) + 1.0)) / 2.0);
    vImag[i] = 0.0; // Imaginary part must be zeroed in case of looping to avoid wrong calculations and overflows
    energy += abs(vReal[i]);
    a_max = max(abs(vReal[i]), a_max);
    a_rms += pow(vReal[i], 2);
  }
  energy /= samplingFrequency;
  a_rms = sqrt(a_rms / samples);

  // Print the results of the simulated sampling according to time
  if (DEBUG)
  {
    print_index(SCL_TIME);
    Serial.print("Reading,");
    PrintVector(vReal, samples, SCL_TIME);
  }
  // Calculate frequency components
  FFT.Windowing(vReal, samples, FFT_WIN_TYP_HAMMING, FFT_FORWARD); /* Weigh data */
  FFT.Compute(vReal, vImag, samples, FFT_FORWARD); /* Compute FFT */
  FFT.ComplexToMagnitude(vReal, vImag, samples); /* Compute magnitudes */
  peak_hz = FFT.MajorPeak(vReal, samples, samplingFrequency);
  if (DEBUG)
  {
    print_index(SCL_FREQUENCY);
    Serial.print("Magnitudes,");
    PrintVector(vReal, (samples >> 1), SCL_FREQUENCY);
  }

  // Find signal characteristics
  for (uint16_t i = 0; i < samples; i++)
  {
    freq_max = max(abs(vReal[i]), freq_max);
    freq_rms += pow(vReal[i], 2);
  }
  freq_rms = sqrt(freq_rms / samples);

}

// Initialize Wi-Fi manager and connect to Wi-Fi
// https://github.com/khoih-prog/ESP_WiFiManager
// TODO Replace with Ethernet
void SetupWiFi()
{
  ESP_WiFiManager wm("uServer");
  wm.autoConnect("uServer");
  if (!WiFi.status() == WL_CONNECTED)
  {
    Serial.println(wm.getStatus(WiFi.status()));
  }
}

// Message received callback function
// TODO Put to use
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

// Connect to MQTT brokeer
void reconnect(const char hostname[], char topic[128])
{
  // Connect and set callback function
  pubsubClient.setServer(hostname, port);
  pubsubClient.setCallback(callback);

  // Create a random client ID
  String clientId = "ESP32-";
  clientId += String(random(0xffff), HEX);
  // Attempt to connect
  if (pubsubClient.connect(clientId.c_str()))
  {
    // TODO Add variable topic and finalize hello message
    pubsubClient.publish(topic, "Born again");
  }
  else
  {
    Serial.print("failed, rc=");
    Serial.print(pubsubClient.state());
    delay(1000);
  }
}

double get_current()
{
  /*
  Reads current from sensor
  and calculates real value 
  */
  double peakVoltage = analogRead(ACPIN);
  // change the peak voltage to the Virtual Value of voltage
  double voltageVirtualValue = peakVoltage * 0.707;
  /*The circuit is amplified by 2 times, so it is divided by 2.*/
  voltageVirtualValue = (voltageVirtualValue / 1024 * VREF) / 2;
  return voltageVirtualValue * ACRANGE;
}

// Auxiiliary Task
void MQTTProcess(void *pvParameters)
{
  /*
  Core 0 process
  Acquires signal sample and publishes
  measurements as JSON over MQTT 
  // Serial.print("MQTT task running on core ");
  // Serial.println(xPortGetCoreID());
  */
  
  StaticJsonDocument<JSON_SIZE> doc;
  
  char buffer[JSON_SIZE];
  Serial.print("Topic: ");
  Serial.println(topic);

  double readings[samples];
  double readingSamples;
  
  // Initialize global JSON doc
  doc["sample_rate"] = samplingFrequency;
  doc["cycle_time"] = 4;
  doc["hourly_cost"] = 1000;
  doc["units"] = "Hz";
 
  size_t n;

  for (;;)
  {
    // Make sure MQTT client is connected
    if (!pubsubClient.connected())
    {
      reconnect(hostname, topic);
    }
    else
    {
      // Get current signal sample
      for (int i = 0; i < samples; i++)
      {
        readingSamples = 0.0;
        // Average in-between readings to stabilize signal
        for (uint8_t j = 0; j < smoothingFactor; j++)
        {
          readingSamples += get_current();
          // TODO Convert to uint8_t
          delay(samplingDelay);
        }
        readings[i] = readingSamples / smoothingFactor;
      }

      // Analyze signal
      run_fft(readings);

      // Write values to global JSON doc
      doc["peak_hz"] = peak_hz;
      doc["energy"] = energy * 0.170;
      doc["a_max"] = a_max;
      doc["a_rms"] = a_rms;
      doc["freq_max"] = freq_max;
      doc["freq_rms"] = freq_rms;

      // Prepare JSON message
      n = serializeJson(doc, buffer);
      
      if (!pubsubClient.publish(topic, buffer, n))
      {
        // Only report errors
        Serial.println(buffer);
        Serial.print("Unable to send to ");
        Serial.println(topic);
      }
      pubsubClient.loop();
      // DELETING THIS DELAY WILL CRASH THE MCU
      delay(20);
    }
  }
}

// TODO Convert to NFC / error handling process
void CurrentProcess(void *pvParameters)
{
  // Serial.print("No task running on core ");
  // Serial.println(xPortGetCoreID());

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

  // Give device monitor time to connect
  // delay(2000);

  // Uses soft AP to connect to Wi-Fi (if saved credentials aren't valid)
  // TODO replace with Ethernet
  SetupWiFi();

  // mDNS allows for connection at http://userver.local/
  // TODO Remove once Ethernet is in place
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

  // Assemble topic
  strcpy(topic, nspace);
  strcat(topic,"/");
  strcat(topic,group_id);
  strcat(topic,"/");
  strcat(topic,message_type);
  strcat(topic,"/");
  strcat(topic,edge_node_id);
  strcat(topic,"/");
  strcat(topic,device_id);
}

// Main loop
void loop() {}
