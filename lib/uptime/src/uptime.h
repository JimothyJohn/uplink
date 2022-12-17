// Source: https://aws.amazon.com/blogs/compute/building-an-aws-iot-core-device-using-aws-serverless-and-an-esp32/
#include <Arduino.h>

#define THINGNAME "uptime"

static uint16_t MQTT_PORT = 8883;
static const char MQTT_USER[] = "";
static const char MQTT_PASS[] = "";

struct Sparkplug {
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

struct Machine {
  char manufacturer[32];
  char model[32];
  char year[32];
  char operation[32];
};

struct Broker {
  char host[32];
  uint16_t port;
  char user[32];
  char pass[32];
};

struct Metrics {
  uint8_t fs;
  uint8_t length;
  float energy;
  double a_max;
  double a_rms;
  double freq_max;
  double freq_rms;
  double peak_hz;
};