#!/usr/bin/env bash

# Install PlatformIO
python3 -c "$(curl -fsSL https://raw.githubusercontent.com/platformio/platformio/master/scripts/get-platformio.py)"

# Upload program and monitor
./Update.sh
