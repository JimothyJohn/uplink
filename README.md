# Power Cycle Monitoring

Experiment in analyzing input current of a power system.

## Prerequisites

- [PlatformIO](docs/PLATFORMIO.md) to build and upload the project
- [Edge Impulse account](https://studio.edgeimpulse.com/signup)
- [Edge Impulse CLI](https://docs.edgeimpulse.com/docs/edge-impulse-cli/cli-installation) to upload the data

## Hardware

This code has only been tested on an [ESP32](https://www.olimex.com/Products/IoT/ESP32/ESP32-POE/open-source-hardware) module.

## Self-guided

1. Create a new [project](https://studio.edgeimpulse.com/studio/select-project)
2. Follow Data-forwarder [docs](https://docs.edgeimpulse.com/docs/edge-impulse-cli/cli-data-forwarder#protocol)
3. Follow [tutorial](https://docs.edgeimpulse.com/docs/tutorials/continuous-motion-recognition) or [video](https://www.youtube.com/watch?v=FseGCn-oBA0)

## To-do

- Verify scaling, responsiveness, and resolution
- Incorporate uServer
- Explore additional axes
- Connect to external database/endpoint
- Test on manufacturing workcell
