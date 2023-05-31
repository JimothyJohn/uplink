# Uptime

Monitor machinery with a microcontroller.

### [Buy one](https://uptime.advin.io/products/uptime)

## Quickstart (Linux)

Installs dependencies and uploads the program.

```bash
bash utils/Quickstart.sh
```

### [Architecture](https://drive.google.com/file/d/15SiOMNvi3Y8zdCXreA7-BQg8EMkJQj_z/view?usp=sharing)

### Prerequisites

- [PlatformIO](https://docs.platformio.org/en/latest/core/installation/methods/installer-script.html#super-quick-macos-linux) to build and upload the project
- [Terraform](https://developer.hashicorp.com/terraform/downloads) to configure your IoT cloud infrastructure
- [AWS account](https://console.aws.amazon.com/iot)

## Hardware

- [Development board](https://www.olimex.com/Products/IoT/ESP32/ESP32-POE/open-source-hardware)
- [Current Sensor](https://wiki.dfrobot.com/Gravity_Analog_AC_Current_Sensor__SKU_SEN0211_)
- [NFC Tag](https://www.adafruit.com/product/4701)

### Features 

[ ] [MQTT](https://mqtt.org) broker agnostic 

[x] Mobile setup over [NFC](https://nfc-forum.org) 

[x] Secure MQTT broker on [AWS IoT](https://aws.amazon.com/iot-core/?nc=sn&loc=2&dn=3)

[x] Edge-side spectrum analysis with [arduinoFFT](https://github.com/kosme/arduinoFFT)

[x] Deployed via [Terraform](https://developer.hashicorp.com/terraform/downloads)

[x] Conforms to [Sparkplug B](https://s3.amazonaws.com/cirrus-link-com/Sparkplug+Specification+Version+1.0.pdf) spec

### Fix list

[ ] Retrieve device data from cloud

[ ] Detect NFC activity

[ ] Create tests 

[ ] Edge-side state determination

[ ] Improve Quickstart platform compatibility 
