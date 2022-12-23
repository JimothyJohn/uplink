// Source: https://aws.amazon.com/blogs/compute/building-an-aws-iot-core-device-using-aws-serverless-and-an-esp32/
#include <pgmspace.h>

#define THINGNAME ""

static const char WIFI_SSID[] = "";
static const char WIFI_PASSWORD[] = "";
static const char MQTT_SERVER[] = "";

// Amazon Root CA 1
static const char AWS_CERT_CA[] PROGMEM = "";
// Device certificate
static const char AWS_CERT_CRT[] PROGMEM = "";
// Device Private Key
static const char AWS_CERT_PRIVATE[] PROGMEM = "";
