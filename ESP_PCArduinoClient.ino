#include <Arduino.h>
#include <SPIFFS.h>
#include <HTTPClient.h>
#include <NTPClient.h>
#include <WiFi.h>
#include <ESPAsyncWebServer.h>
#include <ArduinoJson.h>
#include "base64.h"

const char* apSSID = "ESP32-AP";
const char* apPassword = "password";
const char* filePath = "/configNew.txt";

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

AsyncWebServer server(80);

void setup() {
  Serial.begin(115200);

  setupAccessPoint();
  
  if (!SPIFFS.begin(true)) {
    Serial.println("An error occurred while mounting SPIFFS");
    return;
  }

  connectionInitiation();

   server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
  const char* htmlContent = R"html(
    <!DOCTYPE html>
    <html lang="en">
    <head>
      <meta charset="UTF-8">
      <meta name="viewport" content="width=device-width, initial-scale=1.0">
      <title>WiFi Configuration</title>
        <style>
      body {
        padding: 40px;
        font-family: Arial, sans-serif;
        background-color: #f4f4f4;
        margin: 0;
      }
      .container {
        width: 100%;
        max-width: 600px;
        margin: 0 auto;
      }
      .row {
        display: flex;
        justify-content: center;
      }
      .col-lg-6 {
        width: 100%;
        max-width: 400px;
      }
      .form-container {
        background-color: #fff;
        padding: 20px;
        border-radius: 5px;
        box-shadow: 0 0 10px rgba(0, 0, 0, 0.1);
      }
      .text-center {
        text-align: center;
      }
      .mb-4 {
        margin-bottom: 20px;
        font-size: 28px;
      }
      .form-group {
        margin-bottom: 15px;
      }
      .form-control {
        width: 100%;
        padding: 8px;
        font-size: 16px;
        border: 1px solid #ccc;
        border-radius: 4px;
        box-sizing: border-box;
      }
      .btn {
        display: inline-block;
        padding: 10px 20px;
        font-size: 16px;
        border: none;
        border-radius: 4px;
        background-color: #007bff;
        color: white;
        cursor: pointer;
        transition: background-color 0.3s ease;
      }
      .btn:hover {
        background-color: #0056b3;
      }
    </style>
    </head>
    <body>
      <div class="container">
        <div class="row">
          <div class="col-lg-6">
            <div class="form-container">
              <h1 class="text-center mb-4">PCS WiFi Configuration</h1>
              <form id="wifiForm">
                <div class="form-group">
                  <label for="ssid">SSID:</label>
                  <input type="text" class="form-control" id="ssid" required>
                </div>
                <div class="form-group">
                  <label for="password">Password:</label>
                  <input type="password" class="form-control" id="password" required>
                </div>
                <div class="text-center">
                  <button type="button" class="btn" onclick="saveCredentials()">Configure WiFi</button>
                </div>
              </form>
            </div>
          </div>
        </div>
      </div>

      <script>
        function saveCredentials() {
          var ssid = document.getElementById('ssid').value;
          var password = document.getElementById('password').value;
          var xhr = new XMLHttpRequest();
          xhr.open('POST', '/save', true);
          xhr.setRequestHeader('Content-type', 'application/x-www-form-urlencoded');
          xhr.send('ssid=' + ssid + '&password=' + password);
          xhr.onload = function () {
            if (xhr.status == 200) {
              alert('WiFi credentials configured successfully!');
              location.reload(); // Reload the page
            } else {
              alert('Error while configuring WiFi credentials!');
            }
          };
        }
      </script>
    </body>
    </html>
  )html";

  request->send(200, "text/html", htmlContent);
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
      request->send(200, "text/html", "<html><body><h1>WiFi credentials configured successfully!</h1></body></html>");
      connectWiFi();
    } else {
      request->send(400, "text/html", "<html><body><h1>Invalid credentials!</h1></body></html>");
    }
  });

  server.begin();
}

void connectionInitiation(){

  if (loadCredentials()) {
     connectWiFi();
     }
     else {
     Serial.println("Unable to retrieve WiFi connection details. Please reconfigure it!");
     }
}

void loop() {
  // put your main code here, to run repeatedly:
  if (WiFi.status() == WL_CONNECTED) {
    sendDataToAPI();
    delay(sendInterval);
  } else {
     if (loadCredentials()) {
     connectWiFi();
     }
     else {
     Serial.println("Unable to retrieve WiFi connection details. Please reconfigure it!");
   }
  }
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
