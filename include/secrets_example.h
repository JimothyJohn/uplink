// Source: https://aws.amazon.com/blogs/compute/building-an-aws-iot-core-device-using-aws-serverless-and-an-esp32/
#include <pgmspace.h>

#define SECRET
#define THINGNAME "my_thing"

#define AWS_IOT_PUBLISH_TOPIC   "esp32/pub"
#define AWS_IOT_SUBSCRIBE_TOPIC "esp32/sub"

const char WIFI_SSID[] = "";
const char WIFI_PASSWORD[] = "";
const char AWS_IOT_ENDPOINT[] = "";

// Amazon Root CA 1
static const char AWS_CERT_CA[] PROGMEM = "";
// Device certificate
static const char AWS_CERT_CRT[] PROGMEM = "";
// Device Private Key
static const char AWS_CERT_PRIVATE[] PROGMEM = "";
