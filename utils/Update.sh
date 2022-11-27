#!/usr/bin/env bash

set -o errexit
set -o nounset
set -o pipefail
if [[ "${TRACE-0}" == "1" ]]; then
    set -o xtrace
fi

if [[ "${1-}" =~ ^-*h(elp)?$ ]]; then
    echo "Updates the uptime device with the newest firmware!"
    exit
fi

PIO=$HOME/.platformio/penv/bin/pio
TESTING="false"
MONITOR="true"
LOG_DIR=".logs"
PARAMS=""

main() {

    mkdir -p .logs
    $PIO run -t upload | tee $LOG_DIR/upload.txt
    # cp ./.pio/build/esp-wrover-kit/*.bin ./firmware
    
    if [[ $TESTING == "true" ]]
    then
    sleep 5
    pytest test/
    fi

    if [[ $MONITOR == "true" ]]
    then
    $PIO device monitor | tee $LOG_DIR/monitor.txt
    fi
}

case "$0" in
    -t|--test)
        TESTING="true"
        shift 1
        ;;
    -m|--monitor)
        MONITOR="true"
        shift 1
        ;;
    *) # preserve positional arguments
        PARAMS="$PARAMS $0"
        ;;
esac

# set positional arguments in their proper place
eval set -- "$PARAMS"

main "$@"
