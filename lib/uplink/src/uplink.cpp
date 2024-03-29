// Source: https://aws.amazon.com/blogs/compute/building-an-aws-iot-core-device-using-aws-serverless-and-an-esp32/
#include <SPI.h>
#include <Wire.h>
#include <arduinoFFT.h>   // Spectrum analysis
#include <ArduinoJson.h>  // Handle JSON messages
#include <ST25DVSensor.h> // Read from NFC tag
#include "NfcAdapter.h"   // Read from NFC tag
#include "uplink.h"

Measurement run_fft(double vReal[], Settings settings)
{
  /*
  Takes an array and writes spectrum
  characteristics to global JSONnfcPayload
  */

  // Simulated signal frequency (DEBUG only)
  const float signalFrequency = 1.567;
  const uint8_t amplitude = 130;

  arduinoFFT FFT = arduinoFFT(); /* Create FFT object */
  Measurement metrics;

  // Initialize variables
  double vImag[samples];

  // Build raw data
  // Number of signal cycles that the sampling will read
  float cycles = (((samples - 1) * signalFrequency) / settings.fs); // Find power characteristics
  for (uint16_t i = 0; i < samples; i++)
  {
    // Create simulated signal with some modulation
    vReal[i] = int8_t((amplitude * (sin((i * (twoPi * cycles)) / samples))) / 2.0);                /* Build data with positive and negative values*/
    vReal[i] += int8_t((amplitude / 5 * (sin((4 * i * (twoPi * cycles)) / samples))) / 2.0);       /* Build data with positive and negative values*/
    vReal[i] -= int8_t((amplitude / 2 * (sin((0.2 + 5 * i * (twoPi * cycles)) / samples))) / 2.0); /* Build data with positive and negative values*/
    // Build data displaced on the Y axis to include only positive values
    // vReal[i] = uint8_t((amplitude * (sin((i * (twoPi * cycles)) / samples) + 1.0)) / 2.0);
    vImag[i] = 0.0; // Imaginary part must be zeroed in case of looping to avoid wrong calculations and overflows
    metrics.energy += abs(vReal[i]);
    metrics.a_max = max(abs(vReal[i]), metrics.a_max);
    metrics.a_rms += pow(vReal[i], 2);
  }
  metrics.energy /= settings.fs * 0.17;
  metrics.a_rms = sqrt(metrics.a_rms / samples);

  // Calculate frequency components
  FFT.Windowing(vReal, samples, FFT_WIN_TYP_HAMMING, FFT_FORWARD); /* Weigh data */
  FFT.Compute(vReal, vImag, samples, FFT_FORWARD);                 /* Compute FFT */
  FFT.ComplexToMagnitude(vReal, vImag, samples);                   /* Compute magnitudes */
  metrics.peak_hz = FFT.MajorPeak(vReal, samples, settings.fs);

  // Find signal characteristics
  for (uint16_t i = 0; i < samples; i++)
  {
    metrics.freq_max = max(abs(vReal[i]), metrics.freq_max);
    metrics.freq_rms += pow(vReal[i], 2);
  }
  metrics.freq_rms = sqrt(metrics.freq_rms / samples);
  return metrics;
}

double get_current()
{
  /*
  Reads current from sensor
  and calculates real value
  */
  double peakVoltage = analogRead(ACPIN);
  // change the peak voltage to the Virtual Value of voltage
  double voltageVirtualValue = peakVoltage * 0.707;
  /*The circuit is amplified by 2 times, so it is divided by 2.*/
  voltageVirtualValue = (voltageVirtualValue / 1024 * VREF) / 2;
  return voltageVirtualValue * ACRANGE;
}

uint8_t read_nfc(char *topic)
{
  uint8_t newBuffer[NDEF_MAX_SIZE];

  // The wire instance used can be omited in case you use default Wire instance
  if (st25dv.begin(GPO_PIN, LPD_PIN, &Wire) != 0)
  {
    Serial.println("NFC Tag init failed!");
    return 0;
  }

  st25dv.readBuffer(newBuffer);

  NdefMessage msg = NdefMessage(newBuffer, sizeof(newBuffer));
  const uint8_t num_records = msg.getRecordCount();
  if (num_records > 1)
  {
    Serial.println("More than one record present!");
    return 0;
  }
  else if (num_records == 0)
  {
    Serial.println("No records present!");
    return 0;
  }

  NdefRecord record = msg.getRecord(0);

  if (record.getTnf() != 1)
  {
    Serial.println("Not a known type!");
    return 0;
  }

  if (record.getType() != "T")
  {
    Serial.println("Not a text record!");
    return 0;
  }

  const uint8_t payloadLength = record.getPayloadLength();
  uint8_t payload[payloadLength];
  record.getPayload(payload);
  char char_array[payloadLength];
  const char *prefixArray = "uptime/";
  memccpy(char_array, payload, 0, payloadLength);

  // Hard code DB ID length
  // TODO - Properly read buffer
  for (uint8_t i = 0; i < 7; i++)
  {
    topic[i] = prefixArray[i];
  }
  for (uint8_t i = 0; i < TOPIC_LENGTH - 4; i++)
  {
    topic[i + 7] = char_array[i + 3];
  }
  topic[TOPIC_LENGTH] = '\0';
  return 1;
}

void messageHandler(String &topic, String &payload)
{
  StaticJsonDocument<JSON_SIZE> devicePayload;
  size_t n = sizeof(payload);
  char json_payload[n];

  DeserializationError error = deserializeJson(devicePayload, payload);
  if (error.c_str() != "Ok")
  {
    Serial.print("JSON Deserialization error: ");
    Serial.println(error.c_str());
    Serial.println("JSON payload: ");
    Serial.println(json_payload);
  }
}
