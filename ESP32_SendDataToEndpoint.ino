#include <WiFi.h>
#include <HTTPClient.h>
#include <NTPClient.h>
#include <WiFiUdp.h>
#include <ArduinoJson.h>
#include "base64.h"

const char* ssid = "Inivos CMB";
const char* password = "inivos@2020";
const char* apiUrl = "https://pcabackendapi.azurewebsites.net/api/v1/PowerConsumption"; // Replace with your API endpoint

String authUsername = "teamdigitalfortress";
String authPassword = "DigitalFortress@2023";
String auth = base64::encode(authUsername + ":" + authPassword);

const char *ntpServer = "pool.ntp.org";

const char* deviceSerialKey = "ESP32_001";
const float consumedUnits = 25.5; 
const int sendInterval = 60000; // Interval in milliseconds (1 minute)

WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, ntpServer);

void setup() {
  // put your setup code here, to run once:
  Serial.begin(115200);
  connectToWiFi();
  // Initialize NTP client
  timeClient.begin();
  timeClient.update();

}

void loop() {
  // put your main code here, to run repeatedly:
  if (WiFi.status() == WL_CONNECTED) {
    sendDataToAPI();
    delay(sendInterval);
  } else {
    connectToWiFi();
  }

}

void connectToWiFi() {
  Serial.println("Connecting to WiFi...");
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Connecting...");
  }
  Serial.println("");
  Serial.print("Connected to WiFi network with IP Address: ");
  Serial.println(WiFi.localIP());
  
}

void sendDataToAPI(){

    // Get current time from NTP server
    timeClient.update();
    unsigned long epochTime = timeClient.getEpochTime();

    // Convert epoch time to time_t (seconds since Jan 1, 1970)
    time_t timeSinceEpoch = static_cast<time_t>(epochTime);

    // Convert time_t to a struct tm
    struct tm timeinfo;
    gmtime_r(&timeSinceEpoch, &timeinfo);

    // Format date and time
    char formattedTime[30]; // Adjust buffer size if necessary
    strftime(formattedTime, sizeof(formattedTime), "%Y-%m-%dT%H:%M:%S", &timeinfo);

    // Get milliseconds
    int milliseconds = (millis() % 1000);

    // Append milliseconds to the formatted time
    char timeWithMilliseconds[40]; // Adjust buffer size if necessary
    snprintf(timeWithMilliseconds, sizeof(timeWithMilliseconds), "%s.%03dZ", formattedTime, milliseconds);

    DynamicJsonDocument payload(200);
    payload["deviceSerialKey"] = deviceSerialKey;
    payload["consumedUnits"] = consumedUnits;
    payload["logTimestamp"] = timeWithMilliseconds;


    // Serialize JSON document
    String payloadString;
    serializeJson(payload, payloadString);
    Serial.println(payloadString);
    Serial.println(String(payloadString.length()));

    HTTPClient http;
    http.begin(apiUrl);
    http.addHeader("Authorization", "Basic " + auth);
    // Specify content-type header
    http.addHeader("Content-Type", "application/json");
    //http.addHeader("Content-Length", String(strlen(json)));
    http.addHeader("User-Agent", "HTTPTool/1.0");

    http.addHeader("Content-Length", String(payloadString.length()));

    int httpResponseCode = http.PUT(payloadString);

    if (httpResponseCode > 0) {
          Serial.print("HTTP Response code: ");
          Serial.println(httpResponseCode);
        } else {
          Serial.println("Error sending PUT request");
        }

    http.end();
}