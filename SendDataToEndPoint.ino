#include <WiFi.h>
#include <HTTPClient.h>
#include <base64.h> // Include the Base64 library

const char* ssid = "Dialog 4G 580";
const char* password = "c06610F1";

const char* endpoint = "https://pcabackendapi.azurewebsites.net/api/v1/PowerConsumption";
const String deviceSerialKey = "12056988"; // Replace this with your actual device serial key
double power = 0.0054; // kwh Replace this with the actual power value
String timestamp = "2023-11-16T03:05:07.390Z"; // Replace this with the actual timestamp
const char* auth = "teamdigitalfortress:DigitalFortress@2023"; // Replace with your username and password
 String payload="";

void setup() {
  Serial.begin(115200);
  connectToWiFi();
  //sendDataAsync();
}

void loop() {
  sendDataAsync();
  Serial.println(payload);
   delay(50000);
}

void connectToWiFi() {
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Connecting to WiFi...");
  }
  Serial.println("Connected to WiFi");
}

void sendDataAsync() {
  HTTPClient http;

  http.begin(endpoint);

  // Add headers
  http.addHeader("Content-Type", "application/json");

  // Encode username and password in Base64 directly
  String base64Login = base64::encode("teamdigitalfortress:DigitalFortress@2023");

  // Add Basic Authorization header
  http.addHeader("Authorization", "Basic " + base64Login);

  // Create JSON payload
  String payload = "{\"deviceSerialKey\":\"" + deviceSerialKey + "\",\"consumedUnits\":" + String(power,10) + ",\"logTimestamp\":\"" + timestamp + "\"}";

  Serial.println("Sending payload: " + payload);

  // Start the asynchronous POST request
  int httpResponseCode = http.POST(payload);

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
