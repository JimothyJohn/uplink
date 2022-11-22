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

#define ACPIN 36
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
unsigned long currentTime;

#define SCL_INDEX 0x00
#define SCL_TIME 0x01
#define SCL_FREQUENCY 0x02
#define SCL_PLOT 0x03
const double signalFrequency = 0.167;
const double samplingFrequency = 20;
const uint16_t fftsamples = 128; // This value MUST ALWAYS be a power of 2

void print_index(uint8_t scaleType)
{
  double abscissa;
  uint16_t samples = fftsamples; // This value MUST ALWAYS be a power of 2
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
    samples /= 2;
    break;
  }

  for (uint16_t i = 0; i < samples; i++)
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
      abscissa = ((i * 1.0 * samplingFrequency) / fftsamples);
      break;
    }
    Serial.print(abscissa, 2);
    Serial.print(",");
  }
  Serial.println();
}

void PrintVector(double *vData, uint16_t bufferSize, uint8_t scaleType)
{
  double sentValue = vData[0];
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
  Serial.print(signalFrequency);
  Serial.print(",");
  Serial.print(samplingFrequency);
  Serial.print(",");
  Serial.println(fftsamples/samplingFrequency);
}

double run_fft()
{
  arduinoFFT FFT = arduinoFFT(); /* Create FFT object */

  const uint8_t amplitude = 100;
  /*
  These are the input and output vectors
  Input vectors receive computed results from FFT
  */
  // Replace vReal with ACTUAL data
  double vReal[fftsamples];
  double vImag[fftsamples];
  double energy = 0;
  double peak = 0;

  /* Build raw data */
  double cycles = (((fftsamples - 1) * signalFrequency) / samplingFrequency); // Number of signal cycles that the sampling will read
  for (uint16_t i = 0; i < fftsamples; i++)
  {
    vReal[i] = int8_t((amplitude * (sin((i * (twoPi * cycles)) / fftsamples))) / 2.0); /* Build data with positive and negative values*/
    // Modulate by quadruple the frequency
    vReal[i] += int8_t((amplitude/5 * (sin((4 * i * (twoPi * cycles)) / fftsamples))) / 2.0); /* Build data with positive and negative values*/
    // Modulate and phase by doubling the frequency
    vReal[i] -= int8_t((amplitude/5 * (sin((0.1 + 2 * i * (twoPi * cycles)) / fftsamples))) / 2.0); /* Build data with positive and negative values*/
    // vReal[i] = uint8_t((amplitude * (sin((i * (twoPi * cycles)) / samples) + 1.0)) / 2.0);/* Build data displaced on the Y axis to include only positive values*/
    vImag[i] = 0.0; // Imaginary part must be zeroed in case of looping to avoid wrong calculations and overflows
    energy += abs(vReal[i]);
    peak = max(abs(vReal[i]), peak);
  }
  energy /= samplingFrequency;
  double mean = energy / fftsamples;
  /*
  double stddev;
  for (uint16_t i = 0; i < fftsamples; i++)
  {
    stddev += sqrt(pow(vReal[i] - mean,2) / fftsamples);
  }
  */
  /* Print the results of the simulated sampling according to time */
  doc["energy"] = energy*170;
  doc["peak"] = peak;
  doc["rms"] = energy / fftsamples;

  print_index(SCL_TIME);
  Serial.print("Reading,");
  PrintVector(vReal, fftsamples, SCL_TIME);
  FFT.Windowing(vReal, fftsamples, FFT_WIN_TYP_HAMMING, FFT_FORWARD); /* Weigh data */
  FFT.Compute(vReal, vImag, fftsamples, FFT_FORWARD); /* Compute FFT */
  FFT.ComplexToMagnitude(vReal, vImag, fftsamples); /* Compute magnitudes */
  print_index(SCL_FREQUENCY);
  Serial.print("Magnitudes,");
  PrintVector(vReal, (fftsamples >> 1), SCL_FREQUENCY);
  doc["max_freq"] = FFT.MajorPeak(vReal, fftsamples, samplingFrequency);
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

float get_current()
{
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
  // Serial.print("MQTT task running on core ");
  // Serial.println(xPortGetCoreID());

  char buffer[JSON_SIZE];
  
  float sampleFreq = 20;
  float sampleTime = 1;
  uint8_t samples = static_cast<uint8_t>(sampleFreq * sampleTime);
  uint8_t timeGap = static_cast<uint8_t>(1000 / sampleFreq);

  const char *topic = "/jimothyjohn/current-monitor/test";
  doc["device_name"] = "Olimex";
  doc["device_id"] = 0;
  doc["measurement"] = "current";
  doc["units"] = "A";
  doc["freq"] = sampleFreq;
  JsonArray readings = doc.createNestedArray("readings");
  size_t n = serializeJson(doc, buffer);

  print_setup();
  currentTime = millis();
  Serial.println("Processing time (ms)");
  Serial.println(millis()-currentTime);

  for (;;)
  {
    
    // ArduinoOTA.handle()
    if (!pubsubClient.connected())
    {
      reconnect();
    }
    else
    {
      run_fft();
      n = serializeJson(doc, buffer);
 
      if (!pubsubClient.publish(topic, buffer, n))
      {
        Serial.println(buffer);
        Serial.print("Unable to send to ");
        Serial.println(topic);
      }
      
      pubsubClient.loop();
      // DELETING THIS DELAY COULD CRASH THE MCU
      delay(1000);
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
