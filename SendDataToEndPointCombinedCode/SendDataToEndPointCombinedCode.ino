#include <WiFi.h>
#include <HTTPClient.h>
#include <base64.h> // Include the Base64 library
#include "EmonLib.h"

#include <Arduino.h>
#include <SPIFFS.h>
#include <ESPAsyncWebServer.h>

const char* apSSID = "ESP32-AP";
const char* apPassword = "password";
const char* filePath = "/configNew.txt";

//const char* ssid = "Dialog 4G 580";
//const char* password = "c06610F1";
AsyncWebServer server(80);
const char* endpoint ="https://pcabackendapi.azurewebsites.net/api/v1/PowerConsumption";
const String deviceSerialKey = "12056988"; // Replace this with your actual device serial key
//double power = 0.0054; // kwh Replace this with the actual power value
String timestamp = "2023-11-17T04:47:28.8721234Z"; // Replace this with the actual timestamp
const char* auth = "teamdigitalfortress:DigitalFortress@2023"; // Replace with your username and password
String payload="";
EnergyMonitor emon1; 
#define vCalibration 154.8
#define currCalibration 1.40

void setup() {
  Serial.begin(115200);

if (!SPIFFS.begin(true)) {
    Serial.println("An error occurred while mounting SPIFFS");
    return;
  }

  if (loadCredentials()) {
    connectWiFi();
  }
    setupAccessPoint();
  

  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(200, "text/html", "<html><body><h1>Enter WiFi Credentials</h1><p>SSID: <input type='text' id='ssid'></p><br><p>Password: <input type='password' id='password'></p><p><button onclick='saveCredentials()'>Save</button></p><script>function saveCredentials(){var ssid=document.getElementById('ssid').value;var password=document.getElementById('password').value;var xhr=new XMLHttpRequest();xhr.open('POST','/save',true);xhr.setRequestHeader('Content-type','application/x-www-form-urlencoded');xhr.send('ssid='+ssid+'&password='+password);xhr.onload=function(){if(xhr.status==200){alert('Credentials saved successfully!');}else{alert('Error saving credentials!');}};}</script></body></html>");
  });

  server.on("/save", HTTP_POST, [](AsyncWebServerRequest *request){
     Serial.println("Save called");
    if (request->hasParam("ssid", true) && request->hasParam("password", true)) {
      AsyncWebParameter* p_ssid = request->getParam("ssid", true);
      AsyncWebParameter* p_password = request->getParam("password", true);
      String ssid = p_ssid->value();
       Serial.println("ssid");
       Serial.println(ssid);
      String password = p_password->value();
       Serial.println("password");
       Serial.println(password);
      saveCredentials(ssid, password);
      request->send(200, "text/html", "<html><body><h1>Credentials saved successfully!</h1></body></html>");
      connectWiFi();
    } else {
      request->send(400, "text/html", "<html><body><h1>Invalid credentials!</h1></body></html>");
    }
  });

  server.begin();





  
//  connectToWiFi();
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

//void connectToWiFi() {
  //WiFi.begin(ssid, password);
 // while (WiFi.status() != WL_CONNECTED) {
  //  delay(1000);
   // Serial.println("Connecting to WiFi...");
  //}
  //Serial.println("Connected to WiFi");
//}

void connectWiFi() {
  String ssid = getSSID();
  String password = getPassword();
  WiFi.begin(ssid.c_str(), password.c_str());
  Serial.println("Connecting to WiFi...");
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Connecting to WiFi...");
  }
  Serial.println("\nConnected to WiFi");
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

void setupAccessPoint() {
  WiFi.softAP(apSSID, apPassword);
  Serial.println("Access Point started");
  Serial.print("IP Address: ");
  Serial.println(WiFi.softAPIP());
}

void saveCredentials(String ssid, String password) {
  File file = SPIFFS.open(filePath, "w");
  if (!file) {
    Serial.println("Failed to open config file for writing");
    return;
  }
  file.println(ssid);
  file.println(password);
  file.close();
}

bool loadCredentials() {
  if (!SPIFFS.exists(filePath)) {
    Serial.println("Config file does not exist");
    return false;
  }
  String ssid = getSSID();
  String password = getPassword();
  return (ssid.length() > 0 && password.length() > 0);
}

String getSSID() {
  File file = SPIFFS.open(filePath, "r");
  if (!file) {
    return "";
  }
  String ssid = file.readStringUntil('\n');
  ssid.trim();
  file.close();
  return ssid;
}

String getPassword() {
  File file = SPIFFS.open(filePath, "r");
  if (!file) {
    return "";
  }
  String password = file.readStringUntil('\n');
  password.trim();
  password = file.readStringUntil('\n');
  password.trim();
  file.close();
  return password;
}
