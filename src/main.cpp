// Load relevant librariPrettyPrettyPrettyPrettyPrettyPrettyPrettyPrettyPrettyPrettyPrettyPrettyPrettyPrettyPrettyes
#include <Arduino.h>         //Base framework
#include <ESP_WiFiManager.h> // AP login and maintenance
#include <ESPmDNS.h>         // Connect by hostname
#include <FS.h>              // Get FS functions
#include <SPIFFS.h>          // Enable file system
#include <PubSubClient.h>    // Enable MQTT
#include <HTTPClient.h>      // Extract object data
#include <ArduinoJson.h>     // Handle JSON messages
#include <arduinoFFT.h>      // Spectrum analysis

#if defined(ESP32)
#define DEVICE "ESP32"
#elif defined(ESP8266)
#define DEVICE "ESP8266"
#endif

const uint8_t ACPIN = 32;
// set Non-invasive AC Current Sensor tection range
#define ACRANGE 20

// For Arduino Zero, Due, MKR Family, ESP32, etc. 3V3 controllers, change VREF to 3.3
#define VREF 3.3
#define FS 100

#define JSON_SIZE 1024
#define MQTT_MAX_PACKET_SIZE 1024

#define DEBUG true 

// Dual-core tasks
TaskHandle_t MQTTHandler;
TaskHandle_t CurrentHandler;

StaticJsonDocument<JSON_SIZE> doc;

// Network clients
static WiFiClient wifiClient;
static PubSubClient pubsubClient(wifiClient);

// Current value
float ACCurrentValue = 0;

// Current time
unsigned long startTime = millis();
unsigned long currentTime = millis();

#define SCL_INDEX 0x00
#define SCL_TIME 0x01
#define SCL_FREQUENCY 0x02
#define SCL_PLOT 0x03
const float signalFrequency = 0.567;
const float samplingFrequency = 16;
const uint8_t smoothingFactor = 4;
const uint8_t samplingDelay = static_cast<uint8_t>(1000 / samplingFrequency / smoothingFactor);
const uint16_t samples = 128; // This value MUST ALWAYS be a power of 2

void print_index(uint8_t scaleType)
{
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

void PrintVector(double *vData, uint16_t bufferSize, uint8_t scaleType)
{
  float sentValue = vData[0];
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

void print_setup() {
  Serial.println("Ground Truth (Hz),Sample Rate (Hz),Sample Length (ms)");
  Serial.print("Running");
  Serial.print(",");
  Serial.print(samplingFrequency);
  Serial.print(",");
  Serial.println(samples/samplingFrequency);
}

void run_fft(double vReal[samples])
{
  arduinoFFT FFT = arduinoFFT(); /* Create FFT object */

  const uint8_t amplitude = 100;
  /*
  These are the input and output vectors
  Input vectors receive computed results from FFT
  */
  // Replace vReal with ACTUAL data
  double vImag[samples];
  float energy = 0;
  double a_max = 0;
  double a_rms = 0;
  double freq_max = 0;
  double freq_rms = 0;

  /* Build raw data */
  float cycles = (((samples - 1) * signalFrequency) / samplingFrequency); // Number of signal cycles that the sampling will read
  for (uint16_t i = 0; i < samples; i++)
  {
    vReal[i] = int8_t((amplitude * (sin((i * (twoPi * cycles)) / samples))) / 2.0); /* Build data with positive and negative values*/
    // Modulate by quadruple the frequency
    vReal[i] += int8_t((amplitude/5 * (sin((4 * i * (twoPi * cycles)) / samples))) / 2.0); /* Build data with positive and negative values*/
    // Modulate and phase by doubling the frequency
    vReal[i] -= int8_t((amplitude/2 * (sin((0.2 + 5 * i * (twoPi * cycles)) / samples))) / 2.0); /* Build data with positive and negative values*/
    // vReal[i] = uint8_t((amplitude * (sin((i * (twoPi * cycles)) / samples) + 1.0)) / 2.0);/* Build data displaced on the Y axis to include only positive values*/
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
  FFT.Windowing(vReal, samples, FFT_WIN_TYP_HAMMING, FFT_FORWARD); /* Weigh data */
  FFT.Compute(vReal, vImag, samples, FFT_FORWARD); /* Compute FFT */
  FFT.ComplexToMagnitude(vReal, vImag, samples); /* Compute magnitudes */
  if (DEBUG)
  {
    print_index(SCL_FREQUENCY);
    Serial.print("Magnitudes,");
    PrintVector(vReal, (samples >> 1), SCL_FREQUENCY);
  }

  for (uint16_t i = 0; i < samples; i++)
  {
    freq_max = max(abs(vReal[i]), freq_max);
    freq_rms += pow(vReal[i], 2);
  }
  freq_rms = sqrt(freq_rms / samples);

  doc["peak_hz"] = FFT.MajorPeak(vReal, samples, samplingFrequency);
  doc["energy"] = energy * 0.170;
  doc["a_max"] = a_max;
  doc["a_rms"] = a_rms;
  doc["freq_max"] = freq_max;
  doc["freq_rms"] = freq_rms;
}

// Initialize Wi-Fi manager and connect to Wi-Fi
// https://github.com/khoih-prog/ESP_WiFiManager
void SetupWiFi()
{
  ESP_WiFiManager wm("uServer");
  wm.autoConnect("uServer");
  if (!WiFi.status() == WL_CONNECTED)
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
  const char *hostname = "broker.hivemq.com";

  pubsubClient.setServer(hostname, 1883);
  pubsubClient.setCallback(callback);

  // Create a random client ID
  String clientId = "ESP32-";
  clientId += String(random(0xffff), HEX);
  // Attempt to connect
  if (pubsubClient.connect(clientId.c_str()))
  {
    // Once connected, publish an announcement...
    pubsubClient.publish("/home/status/esp32", "Hello world!");
  }
  else
  {
    Serial.print("failed, rc=");
    Serial.print(pubsubClient.state());
    delay(5000);
  }
}

double get_current()
{
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
  // Serial.print("MQTT task running on core ");
  // Serial.println(xPortGetCoreID());
  char buffer[JSON_SIZE];
  const char *topic = "/jimothyjohn/current-monitor/test";
  double readings[samples];
  double readingSamples;

  doc["device_name"] = "Olimex";
  doc["device_id"] = 0;
  doc["measurement"] = "current";
  doc["units"] = "A";
  doc["sample_rate"] = samplingFrequency;
  
  size_t n = serializeJson(doc, buffer);

  print_setup();

  for (;;)
  {
    if (!pubsubClient.connected())
    {
      reconnect();
    }
    else
    {
      for (int i = 0; i < samples; i++)
      {
        readingSamples = 0.0;
        for (uint8_t j = 0; j < smoothingFactor; j++)
        {
          readingSamples += get_current();
          // TODO Convert to uint8_t
          delay(samplingDelay);
        }
        readings[i] = readingSamples / smoothingFactor;
      }

      run_fft(readings);

      // currentTime = millis();
      // doc["timestamp"] = currentTime;
      n = serializeJson(doc, buffer);
      
      if (!pubsubClient.publish(topic, buffer, n))
      {
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
      MQTTProcess,  /* Task function. */
      "MQTT",       /* name of task. */
      10000,        /* Stack size of task */
      NULL,         /* parameter of the task */
      2,            /* priority of the task */
      &MQTTHandler, /* Task handle to keep track of created task */
      0);           /* pin task to core 0 */

  xTaskCreatePinnedToCore(
      CurrentProcess,  /* Task function. */
      "Current",       /* name of task. */
      10000,           /* Stack size of task */
      NULL,            /* parameter of the task */
      1,               /* priority of the task */
      &CurrentHandler, /* Task handle to keep track of created task */
      1);              /* pin task to core 1 */
}

// Main loop
void loop() {}
