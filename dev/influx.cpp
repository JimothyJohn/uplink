#if defined(ESP32)
#include <WiFiMulti.h>
WiFiMulti wifiMulti;
#define DEVICE "ESP32"
#elif defined(ESP8266)
#include <ESP8266WiFiMulti.h>
ESP8266WiFiMulti wifiMulti;
#define DEVICE "ESP8266"
#endif

#include <InfluxDbClient.h>
#include <InfluxDbCloud.h>

// WiFi AP SSID
#define WIFI_SSID "TheCage"
// WiFi password
#define WIFI_PASSWORD "2143001929"
// InfluxDB v2 server url, e.g. https://eu-central-1-1.aws.cloud2.influxdata.com (Use: InfluxDB UI -> Load Data -> Client Libraries)
#define INFLUXDB_URL "https://us-east-1-1.aws.cloud2.influxdata.com"
// InfluxDB v2 server or cloud API token (Use: InfluxDB UI -> Data -> API Tokens -> Generate API Token)
#define INFLUXDB_TOKEN "7PvL-pxjQa1lc5d6KtsYRQqb4Dd5BTvUQlXBD6HtDLO4rWcq_mQFhbuWSDig98AE6rkcmc2P38It5SXUJ_yoxg=="
// InfluxDB v2 organization id (Use: InfluxDB UI -> User -> About -> Common Ids )
#define INFLUXDB_ORG "nick@advin.io"
// InfluxDB v2 bucket name (Use: InfluxDB UI ->  Data -> Buckets)
#define INFLUXDB_BUCKET "Home"
#define WRITE_PRECISION WritePrecision::MS
#define MAX_BATCH_SIZE 100
#define WRITE_BUFFER_SIZE 300
#define FLUSH_RATE 1000

// Set timezone string according to https://www.gnu.org/software/libc/manual/html_node/TZ-Variable.html
// Examples:
//  Pacific Time: "PST8PDT"
//  Eastern: "EST5EDT"
//  Japanesse: "JST-9"
//  Central Europe: "CET-1CEST,M3.5.0,M10.5.0/3"
#define TZ_INFO "CST6CDT"

// InfluxDB client instance with preconfigured InfluxCloud certificate
InfluxDBClient client(INFLUXDB_URL, INFLUXDB_ORG, INFLUXDB_BUCKET, INFLUXDB_TOKEN, InfluxDbCloud2CACert);

Point sensor("wifi_status");

const uint8_t ACPin = 34;  // set arduino signal read pin
#define ACTectionRange 20; // set Non-invasive AC Current Sensor tection range

// VREF: Analog reference
// For Arduino UNO, Leonardo and mega2560, etc. change VREF to 5
// For Arduino Zero, Due, MKR Family, ESP32, etc. 3V3 controllers, change VREF to 3.3
#define VREF 3.3
float readACCurrentValue()
{
    float ACCurrtntValue = 0;
    float peakVoltage = 0;
    float voltageVirtualValue = 0; // Vrms
    for (int i = 0; i < 5; i++)
    {
        peakVoltage += analogRead(ACPin); // read peak voltage
        delay(1);
    }
    peakVoltage = peakVoltage / 5;
    voltageVirtualValue = peakVoltage * 0.707; // change the peak voltage to the Virtual Value of voltage
    /*The circuit is amplified by 2 times, so it is divided by 2.*/
    voltageVirtualValue = (voltageVirtualValue / 1024 * VREF) / 2;
    ACCurrtntValue = voltageVirtualValue * ACTectionRange;
    return ACCurrtntValue;
}

float ACCurrentValue = readACCurrentValue(); // read AC Current Value

void WriteToInflux(void *pvParameters)
{
    for (;;)
    {
        for (uint8_t i = 0; i < MAX_BATCH_SIZE; i++)
        {
            // Clear fields for reusing the point. Tags will remain untouched
            sensor.clearFields();

            // Store measured value into point
            // Report RSSI of currently connected network
            sensor.addField("rssi", WiFi.RSSI());
            sensor.addField("current", ACCurrentValue);

            // Print what are we exactly writing
            // Serial.print("Writing: ");
            // Serial.println(sensor.toLineProtocol());

            // Write point
            if (!client.writePoint(sensor))
            {
                Serial.print("InfluxDB write failed: ");
                Serial.println(client.getLastErrorMessage());
            }
            uint16_t sampleTime = 1000 / MAX_BATCH_SIZE;
            delay(sampleTime);
        }

        // Check WiFi connection and reconnect if needed
        if (wifiMulti.run() != WL_CONNECTED)
        {
            Serial.println("Wifi connection lost");
        }

        // End of the iteration - force write of all the values into InfluxDB as single transaction
        // Serial.println("Flushing data into InfluxDB");
        if (!client.flushBuffer())
        {
            Serial.print("InfluxDB flush failed: ");
            Serial.println(client.getLastErrorMessage());
            Serial.print("Full buffer: ");
            Serial.println(client.isBufferFull() ? "Yes" : "No");
        }
        delay(100);
    }
}

void MonitorCurrent(void *pvParameters)
{

    for (;;)
    {
        ACCurrentValue = readACCurrentValue(); // read AC Current Value
        Serial.print(ACCurrentValue);
        Serial.print(",");
    }
}

void setup()
{
    Serial.begin(115200);
    while (!Serial)
        ;

    // Setup wifi
    WiFi.mode(WIFI_STA);
    wifiMulti.addAP(WIFI_SSID, WIFI_PASSWORD);

    Serial.print("Connecting to wifi");
    while (wifiMulti.run() != WL_CONNECTED)
    {
        Serial.print(".");
        delay(100);
    }
    Serial.println();

    client.setWriteOptions(WriteOptions().writePrecision(WRITE_PRECISION).batchSize(MAX_BATCH_SIZE).bufferSize(WRITE_BUFFER_SIZE));

    // Add tags
    sensor.addTag("device", DEVICE);
    sensor.addTag("SSID", WiFi.SSID());

    // Accurate time is necessary for certificate validation and writing in batches
    // For the fastest time sync find NTP servers in your area: https://www.pool.ntp.org/zone/
    // Syncing progress and the time will be printed to Serial.
    timeSync(TZ_INFO, "pool.ntp.org", "time.nis.gov");

    // Check server connection
    if (client.validateConnection())
    {
        Serial.print("Connected to InfluxDB: ");
        Serial.println(client.getServerUrl());
    }
    else
    {
        Serial.print("InfluxDB connection failed: ");
        Serial.println(client.getLastErrorMessage());
    }

    // create a task that will be executed in the AuxTaskcode() function, with priority 1 and executed on core 0
    xTaskCreatePinnedToCore(
        WriteToInflux, /* Task function. */
        "InfluxDB",    /* name of task. */
        10000,         /* Stack size of task */
        NULL,          /* parameter of the task */
        1,             /* priority of the task */
        NULL,          /* Task handle to keep track of created task */
        0);            /* pin task to core 0 */

    // create a task that will be executed in the AuxTaskcode() function, with priority 1 and executed on core 0
    xTaskCreatePinnedToCore(
        MonitorCurrent, /* Task function. */
        "Current",      /* name of task. */
        10000,          /* Stack size of task */
        NULL,           /* parameter of the task */
        1,              /* priority of the task */
        NULL,           /* Task handle to keep track of created task */
        1);             /* pin task to core 0 */
}

void loop() {}