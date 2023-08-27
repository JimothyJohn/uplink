// #include "Arduino.h"
#include "freertos/FreeRTOS.h"
#include <WiFiClientSecure.h>
#include "secrets.h"

WiFiClientSecure net = WiFiClientSecure();

// Dual-core tasks
TaskHandle_t MQTTHandler;
TaskHandle_t MessageHandler;

// Auxiiliary Task
void MessageProcess(void *pvParameter)
{
  Serial.println("Message!");
  // Delete this task after it's done
  vTaskDelete(NULL);
}

// Auxiiliary Task
void MQTTProcess(void *pvParameters)
{
  for (;;)
  {
    // Create a FreeRTOS task for the serial output
    xTaskCreate(
        &MessageProcess, /* Task function. */
        "Messager",
        2048,
        NULL,
        2,
        &MessageHandler);
    // DELETING THIS DELAY WILL CRASH THE MCU
    delay(1000);
  }
}

uint8_t connectAWS()
{
  while (WiFi.status() != WL_CONNECTED)
  {
    Serial.print("\rConnecting to Wi-Fi...");
    delay(1000);
  }

  // Configure WiFiClientSecure to use the AWS IoT device credentials
  net.setCACert(AWS_CERT_CA);
  net.setCertificate(AWS_CERT_CRT);
  net.setPrivateKey(AWS_CERT_PRIVATE);

  Serial.print("\rConnected to AWS IoT with client ID \"");
  Serial.print(THING_NAME);
  Serial.print("\"!");
  return 0;
}

// Setup sequence
void setup()
{
  Serial.begin(115200);
  delay(500);
  while (!Serial)
    ;

  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  connectAWS();

  // Figure out a use for the second core
  xTaskCreatePinnedToCore(
      MQTTProcess,  /* Task function. */
      "MQTT",       /* name of task. */
      10000,        /* Stack size of task */
      NULL,         /* parameter of the task */
      1,            /* priority of the task */
      &MQTTHandler, /* Task handle to keep track of created task */
      0);           /* pin task to core 0 */
}

// Main loop
void loop() {}
