// Source: https://aws.amazon.com/blogs/compute/building-an-aws-iot-core-device-using-aws-serverless-and-an-esp32/
#include <Arduino.h>

#define NDEF_MAX_SIZE 256

// For Arduino Zero, Due, MKR Family, ESP32, etc. 3V3 controllers, change VREF to 3.3
static const double VREF = 3.3;
static const double ACRANGE = 20;
// Allow for larger memory allocation 
static const size_t EEPROM_SIZE = 512;
static const size_t JSON_SIZE = 512;

static uint16_t MQTT_PORT = 8883;
static const char MQTT_USER[] = "";
static const char MQTT_PASS[] = "";

// Simulated signal index types
static const byte SCL_INDEX = 0x00;
static const byte SCL_TIME = 0x01;
static const byte SCL_FREQUENCY = 0x02;
static const byte SCL_PLOT = 0x03;

// NFC Tag pins
// GPO is interruption pin configurable on RF events
// field change, memory write, activity, Fast Transfer end, user set/reset/pulse)
static const uint8_t GPO_PIN = 0;
// LPD is interrupt pin for NFC field detection
static const uint8_t LPD_PIN = 33;
// Set pin for current sensor (must be ADC DIO, not CH2)
static const uint8_t ACPIN = 32;
// Number of signal samples
static const uint16_t samples = 128;

struct Sparkplug
{
  // namespace = company name
  char nspace[32];
  // group_id = business unit
  char group_id[32];
  char message_type[32];
  // edge_node_id = location
  char edge_node_id[32];
  // device_id = machine name 
  char device_id[32];
};

struct Device {
  char manufacturer[32];
  char model[32];
  uint16_t year;
  char operation[32];
};

struct Broker {
  char host[32];
  uint16_t port;
  char user[32];
  char pass[32];
};

struct Settings {
  uint8_t fs;
  // This value must always be a power of 2
  uint16_t samples;
  uint8_t smoothingFactor;
  uint8_t a_lim;
};

struct Measurement {
  float energy;
  double a_max;
  double a_rms;
  double freq_max;
  double freq_rms;
  double peak_hz;
};

Measurement run_fft(double vReal[], Settings settings);

double get_current();

const char *read_nfc();

void save_topic(const char *topic);
