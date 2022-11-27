# Uptime

Monitor machinery status with a simple microcontroller.

## Quickstart (Linux or Mac)

```bash
bash utils/Quickstart.sh
```

## Prerequisites

- [PlatformIO](docs/PLATFORMIO.md) to build and upload the project

## Hardware

This code has only been tested on an [ESP32](https://www.olimex.com/Products/IoT/ESP32/ESP32-POE/open-source-hardware) module.

## To-do

[x] Connect to external MQTT Broker (AWS IoT) 
[x] Analyze waveforms on device
[x] Commission AWS Thing with Terraform 
[ ] Conform to [Sparkplug B](https://s3.amazonaws.com/cirrus-link-com/Sparkplug+Specification+Version+1.0.pdf) spec
[ ] NFC-powered mobile configuration
[ ] Improve Quickstart platform compatibility (for developers) 
[ ] Create user manual
