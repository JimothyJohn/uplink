#!/usr/bin/env bash
# Source: https://raw.githubusercontent.com/norbertg1/pubsubclient/master/examples/mqtt_ESP32_TLS/certificates.hertificate_generator.sh

#################################################################################################################################################################################
#Certificate generator for TLS encryption																	#
#################################################################################################################################################################################
openssl req -new -x509 -days 365 -extensions v3_ca -keyout include/ca.key -out include/ca.crt -passout pass:1234 -subj '/CN=TrustedCA.net'
#If you generating self-signed certificates the CN can be anything

openssl genrsa -out include/mosquitto.key 2048
openssl req -out include/mosquitto.csr -key include/mosquitto.key -new -subj '/CN=locsol.dynamic-dns.net'
openssl x509 -req -in include/mosquitto.csr -CA include/ca.crt -CAkey include/ca.key -CAcreateserial -out include/mosquitto.crt -days 365 -passin file:PEMpassphrase.pass
#Mostly the client verifies the adress of the mosquitto server, so its necessary to set the CN to the correct adress (eg. yourserver.com)!!!


#################################################################################################################################################################################
#These certificates are only needed if the mosquitto broker requires a certificate for client autentithication (require_certificate is set to true in mosquitto config).      	#
#################################################################################################################################################################################
openssl genrsa -out include/esp.key 2048
openssl req -out include/esp.csr -key include/esp.key -new -subj '/CN=localhost'
openssl x509 -req -in include/esp.csr -CA include/ca.crt -CAkey include/ca.key -CAcreateserial -out include/esp.crt -days 365 -passin pass:1234
#If the server (mosquitto) identifies the clients based on CN key, its necessary to set it to the correct value, else it can be blank.

#################################################################################################################################################################################
#For ESP32. Formatting for source code. The output is the esp_certificates.h                                                                                                    #
#################################################################################################################################################################################
echo -en "const char CA_cert[] = \\\\\n" > include/esp_certificates.h;
sed -z '0,/-/s//"-/;s/\n/\\n" \\\n"/g;s/\\n" \\\n"$/";\n\n/g' include/ca.crt >> include/esp_certificates.h     #replace first occurance of - with "-  ;  all newlnies with \n " \ \n", except last newline where just add ;"
echo "const char ESP_CA_cert[] = \\" >> include/esp_certificates.h;
sed -z '0,/-/s//"-/;s/\n/\\n" \\\n"/g;s/\\n" \\\n"$/";\n\n/g' include/esp.crt >> include/esp_certificates.h     #replace first occurance of - with "-  ;  all newlnies with \n " \ \n", except last newline where just add ;"
echo "const char ESP_RSA_key[] = \\" >> include/esp_certificates.h;
sed -z '0,/-/s//"-/;s/\n/\\n" \\\n"/g;s/\\n" \\\n"$/";/g' include/esp.key >> include/esp_certificates.h     #replace first occurance of - with "-  ;  all newlnies with \n " \ \n", except last newline where just add ;"
