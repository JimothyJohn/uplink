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
