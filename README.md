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

### TODO

[ ] IN PROGRESS - Move metadata characteristics and MQTT data to cloud

[ ] Add Azure IoT functionality via Terraform

[ ] Learn historical states at edge 

[ ] Improve Quickstart platform compatibility (for developers) 

[ ] Detect NFC activity

[x] Mobile setup over NFC 

[x] Connect to secure MQTT Broker (AWS IoT) 

[x] Analyze waveforms on device

[x] Commission AWS Thing with Terraform 

[x] Conform topics to [Sparkplug B](https://s3.amazonaws.com/cirrus-link-com/Sparkplug+Specification+Version+1.0.pdf) spec
