#!/usr/bin/env bash

set -o errexit
set -o nounset
set -o pipefail
if [[ "${TRACE-0}" == "1" ]]; then
    set -o xtrace
fi

if [[ "${1-}" =~ ^-*h(elp)?$ ]]; then
    echo "Adds a new thing"
    exit
fi

main() {
    terraform apply

    PRIVATE_KEY=$(cat terraform.tfstate \
        | jq .resources[2].instances[0].attributes.private_key)

    CERTIFICATE=$(cat terraform.tfstate \
        | jq .resources[2].instances[0].attributes.certificate_pem)

    sed -i -r "s|// Device.*||g" include/secrets.h
    sed -i -r "s/static const char AWS_CERT_CRT.*//g" include/secrets.h
    sed -i -r "s/static const char AWS_CERT_PRIVATE.*//g" include/secrets.h
    sed -i "s/\n\n$//" include/secrets.h
    echo "// Device certificate" >> include/secrets.h
    echo "static const char AWS_CERT_CRT[] PROGMEM = $CERTIFICATE;" >> include/secrets.h
    echo "// Device Private Key" >> include/secrets.h
    echo "static const char AWS_CERT_PRIVATE[] PROGMEM = $PRIVATE_KEY;" >> include/secrets.h
}

main "$@"
