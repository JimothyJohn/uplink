// Load relevant librariPrettyPrettyPrettyPrettyPrettyPrettyPrettyPrettyPrettyPrettyPrettyPrettyPrettyPrettyPrettyes
#include <Arduino.h>         //Base framework
#include <WiFiClientSecure.h>
// Load relevant libraries
#include <SPIFFS.h>          // Enable file system
#include <MQTTClient.h>    // Enable MQTT
#include <ArduinoJson.h>     // Handle JSON messages
#include <arduinoFFT.h>      // Spectrum analysis
#include "secrets.h"

// Current sensor detection range
#define ACRANGE 20

// For Arduino Zero, Due, MKR Family, ESP32, etc. 3V3 controllers, change VREF to 3.3
#define VREF 3.3

// Allow for larger JSON and MQTT messages
#define JSON_SIZE 1024 
#define MQTT_MAX_PACKET_SIZE 2048 

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
WiFiClientSecure net = WiFiClientSecure();
MQTTClient client = MQTTClient(JSON_SIZE);
uint16_t port = 8883;

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
char nspace[] = "Armenta";
// grouo_id = business unit
char group_id[] = "Home";
char message_type[] = "signal";
// edge_node_id = location
char edge_node_id[] = "Bedford";
// device_id = machine name 
char device_id[] = "uptime";
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

void messageHandler(String &topic, String &payload) {
  Serial.println("incoming: " + topic + " - " + payload);

//  StaticJsonDocument<200> doc;
//  deserializeJson(doc, payload);
//  const char* message = doc["message"];
}

void connectAWS()
{
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

  Serial.println("Connecting to Wi-Fi");

  while (WiFi.status() != WL_CONNECTED){
    delay(500);
    Serial.print(".");
  }

  // Configure WiFiClientSecure to use the AWS IoT device credentials
  net.setCACert(AWS_CERT_CA);
  net.setCertificate(AWS_CERT_CRT);
  net.setPrivateKey(AWS_CERT_PRIVATE);

  // Connect to the MQTT broker on the AWS endpoint we defined earlier
  client.begin(AWS_IOT_ENDPOINT, 8883, net);

  // Create a message handler
  client.onMessage(messageHandler);

  Serial.print("Connecting to AWS IOT");

  while (!client.connect(THINGNAME)) {
    Serial.print(".");
    delay(100);
  }

  if(!client.connected()){
    Serial.println("AWS IoT Timeout!");
    return;
  }

  // Subscribe to a topic
  client.subscribe(AWS_IOT_SUBSCRIBE_TOPIC);

  Serial.println("AWS IoT Connected!");
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
  */
  
  StaticJsonDocument<JSON_SIZE> doc;
  
  char buffer[JSON_SIZE];
  Serial.print("Topic: ");
  Serial.println(topic);

  double readings[samples];
  double readingSamples;
  
  // Initialize global JSON doc
  doc["fs"] = samplingFrequency;
  doc["a_lim"] = 20;
  doc["cycle_time"] = 4;
  doc["cost_hr"] = 1000;
  doc["mfgr"] = "custom";
  doc["model"] = "custom";
  doc["yr"] = 2022;
  doc["process"] = "manufacturing";

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
      
      // client.publish(AWS_IOT_PUBLISH_TOPIC, buffer);
      if (!client.publish(AWS_IOT_PUBLISH_TOPIC, buffer, n))
      {
        // Only report errors
        Serial.println(buffer);
        Serial.print("Unable to send to ");
        Serial.println(AWS_IOT_PUBLISH_TOPIC);
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
