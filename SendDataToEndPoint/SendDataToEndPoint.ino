#include <WiFi.h>
#include <HTTPClient.h>
#include <base64.h> // Include the Base64 library
#include "EmonLib.h"

const char* ssid = "Dialog 4G 580";
const char* password = "c06610F1";

const char* endpoint ="https://pcabackendapi.azurewebsites.net/api/v1/PowerConsumption";
const String deviceSerialKey = "12056988"; // Replace this with your actual device serial key
//double power = 0.0054; // kwh Replace this with the actual power value
String timestamp = "2023-11-17T04:47:28.8721234Z"; // Replace this with the actual timestamp
const char* auth = "teamdigitalfortress:DigitalFortress@2023"; // Replace with your username and password
String payload="";
EnergyMonitor emon1; 
#define vCalibration 106.8
#define currCalibration 0.52

void setup() {
  Serial.begin(115200);
  connectToWiFi();
  setLocalTime();
  emon1.voltage(35, vCalibration, 1.7); // Voltage: input pin, calibration, phase_shift
  emon1.current(34, currCalibration); // Current: input pin, calibration.
  //emon1.calcVI(20,2000); // delay till get correct values
  //delay(900000);
}

void loop() {
  sendDataAsync();
  Serial.println(payload);
   delay(60000);
}

void connectToWiFi() {
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Connecting to WiFi...");
  }
  Serial.println("Connected to WiFi");
}
void setLocalTime() {
   configTime(5.5 * 3600, 0, "pool.ntp.org", "time.nist.gov"); // Set timezone to Sri Lanka (5.5 hours ahead of UTC)

  time_t now;
  struct tm timeinfo;

  while (!getLocalTime(&timeinfo)) {
    Serial.println("Failed to obtain time");
    delay(1000);
  }
}
void sendDataAsync() {

// get time
  struct tm timeinfo;
  if (!getLocalTime(&timeinfo)) {
    Serial.println("Failed to obtain time");
    return;
  }
   char timestamp[30];
  strftime(timestamp, sizeof(timestamp), "%Y-%m-%dT%H:%M:%S", &timeinfo);
  String timeNow = String(timestamp) + "." + String(millis() % 1000) + "Z";
  Serial.println(String(timestamp) + "." + String(millis() % 1000) + "Z");
//-------------------
  //read current and voltage 
  emon1.calcVI(20,2000);
  float supplyVoltage   = emon1.Vrms;             //extract Vrms into Variable
  float Irms            = emon1.Irms;             //extract Irms into Variable
  float power =(supplyVoltage *Irms)/1000; //KwH

 Serial.println("V");
  Serial.println(supplyVoltage);

  Serial.println("I");
  Serial.println(Irms);
  
  HTTPClient http;

  http.begin(endpoint);

  // Add headers
  http.addHeader("Content-Type", "application/json");

  // Encode username and password in Base64 directly
  String base64Login = base64::encode("teamdigitalfortress:DigitalFortress@2023");

  // Add Basic Authorization header
  http.addHeader("Authorization", "Basic " + base64Login);

  // Create JSON payload
  String payload = "{\"deviceSerialKey\":\"" + deviceSerialKey + "\",\"consumedUnits\":" + String(power,4) + ",\"logTimestamp\":\"" + timeNow + "\"}";

  Serial.println("Sending payload: " + payload);

  // Start the asynchronous POST request
  int httpResponseCode = http.PUT(payload);

  if (httpResponseCode > 0) {
    Serial.print("HTTP Response code: ");
    Serial.println(httpResponseCode);
    String response = http.getString();
   Serial.println("Response: " + response);
  } else {
    Serial.print("Error code: ");
    Serial.println(httpResponseCode);
  }

  http.end();
}
