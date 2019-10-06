// Example testing sketch for various DHT humidity/temperature sensors
// Written by ladyada, public domain

// REQUIRES the following Arduino libraries:
// - DHT Sensor Library: https://github.com/adafruit/DHT-sensor-library
// - Adafruit Unified Sensor Lib: https://github.com/adafruit/Adafruit_Sensor

#include <ESP8266WiFi.h>
#include <WiFiClientSecure.h>
#include <WiFiUdp.h>

#include <AzureIoTHub.h>
#include <AzureIoTProtocol_MQTT.h>
#include <AzureIoTUtility.h>

#include "DHT.h"

#define MESSAGE_MAX_LEN 256
#define DHTPIN 2     // Digital pin connected to the DHT sensor
#define INTERVAL 2000
#define HIGH_TEMP 70

// Uncomment whatever type you're using!
//#define DHTTYPE DHT11   // DHT 11
#define DHTTYPE DHT22   // DHT 22  (AM2302), AM2321
//#define DHTTYPE DHT21   // DHT 21 (AM2301)

const char* connectionString = "";
const char* ssid = "wwcode";
const char* pass = "Hopper Lovelae Borg";
const char *onSuccess = "\"Successfully invoke device method\"";
const char *notFound = "\"No method found\"";

// Feather HUZZAH ESP8266 note: use pins 3, 4, 5, 12, 13 or 14 --
// Pin 15 can work but DHT must be disconnected during program upload.

// Initialize DHT sensor.
// Note that older versions of this library took an optional third parameter to
// tweak the timings for faster processors.  This parameter is no longer needed
// as the current DHT reading algorithm adjusts itself to work on faster procs.
DHT dht(DHTPIN, DHTTYPE);

static WiFiClientSecure sslClient; // for ESP8266

static int interval = INTERVAL;
static bool messagePending = false;
static bool messageSending = true;

void initWifi()
{
    // Attempt to connect to Wifi network:
    Serial.printf("Attempting to connect to SSID: %s\n", ssid);

    // Connect to WPA/WPA2 network. Change this line if using open or WEP network:
    WiFi.begin(ssid, pass);
    while (WiFi.status() != WL_CONNECTED) {
        // Get Mac Address and show it.
        // WiFi.macAddress(mac) save the mac address into a six length array, but the endian may be different. The huzzah board should
        // start from mac[0] to mac[5], but some other kinds of board run in the oppsite direction.
        uint8_t mac[6];
        WiFi.macAddress(mac);
        Serial.printf("You device with MAC address %02x:%02x:%02x:%02x:%02x:%02x connects to %s failed! Waiting 10 seconds to retry.\r\n",
                mac[0], mac[1], mac[2], mac[3], mac[4], mac[5], ssid);
        WiFi.begin(ssid, pass);
        delay(10000);
    }
    Serial.printf("Connected to wifi %s.\n", ssid);
}

static IOTHUB_CLIENT_LL_HANDLE iotHubClientHandle;

void setup() {
  Serial.begin(74880);
  Serial.println(F("DHTxx test!"));

  initWifi();
  dht.begin();
  Serial.println("initiated dht");
    /*
     * AzureIotHub library remove AzureIoTHubClient class in 1.0.34, so we remove the code below to avoid
     *    compile error
    */

    // initIoThubClient();
    iotHubClientHandle = IoTHubClient_LL_CreateFromConnectionString(connectionString, MQTT_Protocol);
    if (iotHubClientHandle == NULL)
    {
        Serial.println("Failed on IoTHubClient_CreateFromConnectionString.");
        while (1);
    }

    Serial.println("Setting iotHub handlers...");
    IoTHubClient_LL_SetOption(iotHubClientHandle, "product_info", "HappyPath_AdafruitFeatherHuzzah-C");
    IoTHubClient_LL_SetMessageCallback(iotHubClientHandle, receiveMessageCallback, NULL);
    IoTHubClient_LL_SetDeviceMethodCallback(iotHubClientHandle, deviceMethodCallback, NULL);
    IoTHubClient_LL_SetDeviceTwinCallback(iotHubClientHandle, twinCallback, NULL);
    Serial.println("done connection to iotHub");
}

char messagePayload[MESSAGE_MAX_LEN];
char DEVICE_ID[] = "WWCODE-HUZZAH";

void loop() {
    if (!messagePending && messageSending)
    {
      float h = dht.readHumidity();
//      // Read temperature as Celsius (the default)
//      float t = dht.readTemperature();
      // Read temperature as Fahrenheit (isFahrenheit = true)
      float f = dht.readTemperature(true);
      temperatureAlert = (f > HIGH_TEMP);
      // Check if any reads failed and exit early (to try again).
      if (isnan(h) || isnan(f)) {
          Serial.println(F("Failed to read from DHT sensor!"));
      } els {
        sprintf(messagePayload, "{'deviceId':%s,'temperature':%f,'humidity':%f}\n", DEVICE_ID, h, f);
        Serial.print(messagePayload);

        sendMessage(iotHubClientHandle, messagePayload, temperatureAlert);
        messageCount++;
        delay(interval);
      }
      IoTHubClient_LL_DoWork(iotHubClientHandle);
      delay(10);
    }
}
