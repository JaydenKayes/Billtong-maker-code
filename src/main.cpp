#include <Arduino.h>
#include <WiFi.h>
#include "arduino_secrets.h" // Contains SECRET_SSID and SECRET_PASS
#include "DHT.h"

const char SSID[] = SECRET_SSID;
const char PASSWORD[] = SECRET_PASS;

WiFiServer server(80);

// --- Pin Assignments ---
const int LED_PIN = 13;       // Indicator LED (optional)
const int FAN_PIN = 26;       // Fan relay output
const int LAMP_PIN = 27;      // Heat lamp relay output
const int DHT_PIN = 4;        // DHT22 sensor input pin

// --- DHT Setup ---
//HAN NOTES - What sensor are you using? I had the AHT20 and BMP280 available in class?
#define DHTTYPE DHT22         // Change to DHT11 if using that sensor
DHT dht(DHT_PIN, DHTTYPE);

// --- Thresholds for auto control ---
const float TEMP_THRESHOLD = 30.0;   // Max temp before fan turns on
const float HUMID_THRESHOLD = 70.0;  // Max humidity before fan turns on

// --- Device States ---
bool fanState = false;
bool lampState = false;

void connectToWiFi() {
  WiFi.mode(WIFI_STA);
  WiFi.begin(SSID, PASSWORD);
  Serial.print("Connecting to WiFi");
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    delay(500);
  }
  Serial.println("\nConnected to WiFi!");
  Serial.print("IP address: http://");
  Serial.println(WiFi.localIP());
}

void setup() {
  Serial.begin(115200);
  delay(1000);

  // Setup pins
  pinMode(LED_PIN, OUTPUT);
  pinMode(FAN_PIN, OUTPUT);
  pinMode(LAMP_PIN, OUTPUT);

  // Start DHT sensor and WiFi
  dht.begin();
  connectToWiFi();
  server.begin();
}

// --- Handles normal webpage requests (UI + buttons) ---
void handleClient(WiFiClient client) {
  String request = client.readStringUntil('\r');
  Serial.println(request);

  // --- Handle button commands ---
  if (request.indexOf("GET /fanOn") >= 0) {
    fanState = true;
  } else if (request.indexOf("GET /fanOff") >= 0) {
    fanState = false;
  } else if (request.indexOf("GET /lampOn") >= 0) {
    lampState = true;
  } else if (request.indexOf("GET /lampOff") >= 0) {
    lampState = false;
  }

  // --- Read sensor data ---
  float temp = dht.readTemperature();
  float hum = dht.readHumidity();

  //HAN NOTES - I haven't talked to you about logic commands like isnan and ||
  //can you explain it further?
  // If sensor failed, set fallback values
  if (isnan(temp) || isnan(hum)) {
    Serial.println("Failed to read from DHT sensor!");
    temp = -99;
    hum = -99;
  }

  // --- Automatic control rules ---
  if (temp > TEMP_THRESHOLD) {
    fanState = true;   // Too hot → fan ON
    lampState = false; // Don’t heat if hot
  }
  if (hum > HUMID_THRESHOLD) {
    fanState = true;   // Too humid → fan ON
  }

  // Apply control states to relays
  digitalWrite(FAN_PIN, fanState ? HIGH : LOW);
  digitalWrite(LAMP_PIN, lampState ? HIGH : LOW);

  // --- Send webpage to client ---
  client.println("HTTP/1.1 200 OK");
  client.println("Content-Type: text/html");
  client.println("Connection: close");
  client.println();
  client.println("<!DOCTYPE html><html>");
  client.println("<head><meta charset='utf-8'>");
  client.println("<title>Biltong Maker</title>");
  
  // JavaScript to fetch live data
  client.println("<script>");
  client.println("async function refreshData(){");
  client.println("  let r=await fetch('/data');");
  client.println("  let j=await r.json();");
  client.println("  document.getElementById('temp').innerText=j.temp+' °C';");
  client.println("  document.getElementById('hum').innerText=j.hum+' %';");
  client.println("  document.getElementById('fan').innerText=j.fan?'ON':'OFF';");
  client.println("  document.getElementById('lamp').innerText=j.lamp?'ON':'OFF';");
  client.println("}");
  client.println("setInterval(refreshData,2000);"); // Auto-update every 2 sec
  client.println("</script></head>");
  
  // HTML layout
  client.println("<body onload='refreshData()'>");
  client.println("<h1>Biltong Maker Control</h1>");
  client.println("<p>Temperature: <span id='temp'>--</span></p>");
  client.println("<p>Humidity: <span id='hum'>--</span></p>");
  client.println("<p>Fan: <span id='fan'>--</span></p>");
  client.println("<p>Lamp: <span id='lamp'>--</span></p>");
  client.println("<button onclick=\"fetch('/fanOn')\">Fan ON</button>");
  client.println("<button onclick=\"fetch('/fanOff')\">Fan OFF</button><br><br>");
  client.println("<button onclick=\"fetch('/lampOn')\">Lamp ON</button>");
  client.println("<button onclick=\"fetch('/lampOff')\">Lamp OFF</button>");
  client.println("</body></html>");
  
  client.flush();
  delay(10);
  client.stop();
}

// --- Handles JSON data requests for live updates ---
void handleData(WiFiClient client) {
  float temp = dht.readTemperature();
  float hum = dht.readHumidity();

  // Fallback if sensor fails
  if (isnan(temp) || isnan(hum)) {
    temp = -99;
    hum = -99;
  }

  // Return JSON response
  client.println("HTTP/1.1 200 OK");
  client.println("Content-Type: application/json");
  client.println("Connection: close");
  client.println();
  client.print("{\"temp\":");
  client.print(temp);
  client.print(",\"hum\":");
  client.print(hum);
  client.print(",\"fan\":");
  client.print(fanState ? "true" : "false");
  client.print(",\"lamp\":");
  client.print(lampState ? "true" : "false");
  client.println("}");
  
  client.flush();
  delay(10);
  client.stop();
}

void loop() {
  WiFiClient client = server.available();
  if (client) {
    Serial.println("New client connected");
    String reqLine = client.readStringUntil('\n');
    Serial.println(reqLine);
    
    // Direct to JSON data or HTML handler
    if (reqLine.indexOf("GET /data") >= 0) {
      handleData(client);
    } else {
      handleClient(client);
    }
  }
}
