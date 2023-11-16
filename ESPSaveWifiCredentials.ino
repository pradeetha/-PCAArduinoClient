#include <Arduino.h>
#include <SPIFFS.h>
#include <WiFi.h>
#include <ESPAsyncWebServer.h>

const char* apSSID = "ESP32-AP";
const char* apPassword = "password";
const char* filePath = "/configNew.txt";

AsyncWebServer server(80);

void setup() {
  Serial.begin(115200);
  
  if (!SPIFFS.begin(true)) {
    Serial.println("An error occurred while mounting SPIFFS");
    return;
  }

  if (loadCredentials()) {
    connectWiFi();
  } else {
    setupAccessPoint();
  }

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
}

void loop() {
  
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
