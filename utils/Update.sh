#!/usr/bin/env bash

PIO=$HOME/.platformio/penv/bin/pio

mkdir .logs
$PIO run -t upload | tee ./.logs/upload.txt
# cp ./.pio/build/esp-wrover-kit/*.bin ./firmware
sleep 1
$PIO device monitor | tee ./.logs/monitor.json
