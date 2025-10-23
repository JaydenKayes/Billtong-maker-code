#include <Arduino.h>
#include <WiFi.h>
#include "arduino_secrets.h"   // Contains Wi-Fi credentials (SSID & password)
#include "DHT.h"               // DHT11 temperature/humidity sensor
#include <Adafruit_GFX.h>      // Core graphics library
#include <Adafruit_ST7789.h>   // Driver for ST7789 TFT display

// ---------- PIN DEFINITIONS ----------
#define DHT_PIN 5               // GPIO5 → DHT11 data pin
#define DHTTYPE DHT11           // Using DHT11 sensor
#define LED_PIN 13              // Optional indicator LED
#define FAN_PIN 26              // Relay controlling fan
#define LAMP_PIN 27             // Relay controlling lamp

// TFT pins for ESP32-S3 Reverse TFT
#define TFT_CS 42
#define TFT_DC 40
#define TFT_RST 41
#define TFT_BL 45               // Backlight

// ---------- WIFI SERVER ----------
WiFiServer server(80);           // HTTP server on port 80

// ---------- SENSOR & DISPLAY OBJECTS ----------
DHT dht(DHT_PIN, DHTTYPE);       // Initialize DHT11
Adafruit_ST7789 tft = Adafruit_ST7789(TFT_CS, TFT_DC, TFT_RST);  // Initialize TFT

// ---------- THRESHOLDS & STATES ----------
const float TEMP_THRESHOLD = 30.0;   // Fan triggers above 30°C
const float HUMID_THRESHOLD = 70.0;  // Fan triggers above 70% humidity
bool fanState = false;                // Track fan status
bool lampState = false;               // Track lamp status

// ---------- WIFI CONNECT FUNCTION ----------
void connectToWiFi() {
  WiFi.mode(WIFI_STA);                  // Set ESP32 to station mode
  WiFi.begin(SECRET_SSID, SECRET_PASS); // Connect to your network
  Serial.print("📡 Connecting to Wi-Fi");
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    delay(500);
  }
  Serial.println("\n✅ Wi-Fi connected!");
  Serial.print("📶 IP Address: ");
  Serial.println(WiFi.localIP());
}

// ---------- DHT SENSOR CHECK ----------
bool checkDHTSensor() {
  float t = dht.readTemperature();
  float h = dht.readHumidity();
  if (isnan(t) || isnan(h)) {
    Serial.println("❌ ERROR: DHT11 sensor not responding!");
    return false;
  }
  Serial.println("✅ DHT11 sensor OK");
  Serial.print("Initial Temp: "); Serial.print(t); Serial.print("°C, ");
  Serial.print("Humidity: "); Serial.print(h); Serial.println("%");
  return true;
}

// ---------- CLIENT HANDLER ----------
void handleClient(WiFiClient client) {
  String request = client.readStringUntil('\r');
  Serial.println("🌐 New Client Request: " + request);

  // Manual control via URL
  if (request.indexOf("GET /fanOn") >= 0) fanState = true;
  if (request.indexOf("GET /fanOff") >= 0) fanState = false;
  if (request.indexOf("GET /lampOn") >= 0) lampState = true;
  if (request.indexOf("GET /lampOff") >= 0) lampState = false;

  // Read DHT11
  float t = dht.readTemperature();
  float h = dht.readHumidity();
  if (isnan(t) || isnan(h)) {
    Serial.println("⚠️ ERROR: DHT11 read failed during client request");
    t = -99; h = -99;
  }

  // Automatic control
  if (t > TEMP_THRESHOLD) { fanState = true; lampState = false; }
  if (h > HUMID_THRESHOLD) { fanState = true; }

  // Update pins
  digitalWrite(FAN_PIN, fanState ? HIGH : LOW);
  digitalWrite(LAMP_PIN, lampState ? HIGH : LOW);

  // Send simple webpage
  client.println("HTTP/1.1 200 OK");
  client.println("Content-Type: text/html");
  client.println("Connection: close");
  client.println();
  client.println("<!DOCTYPE html><html><head><title>Biltong Maker</title></head><body>");
  client.println("<h1>Biltong Maker Control</h1>");
  client.println("<p>Temperature: " + String(t) + " °C</p>");
  client.println("<p>Humidity: " + String(h) + " %</p>");
  client.println("<p>Fan: " + String(fanState ? "ON" : "OFF") + "</p>");
  client.println("<p>Lamp: " + String(lampState ? "ON" : "OFF") + "</p>");
  client.println("</body></html>");
  client.flush();
  delay(10);
  client.stop();
}

// ---------- JSON DATA HANDLER ----------
void handleData(WiFiClient client) {
  float t = dht.readTemperature();
  float h = dht.readHumidity();
  if (isnan(t) || isnan(h)) { t = -99; h = -99; }

  client.println("HTTP/1.1 200 OK");
  client.println("Content-Type: application/json");
  client.println("Connection: close");
  client.println();
  client.print("{\"temp\":"); client.print(t);
  client.print(",\"hum\":"); client.print(h);
  client.print(",\"fan\":"); client.print(fanState ? "true" : "false");
  client.print(",\"lamp\":"); client.print(lampState ? "true" : "false");
  client.println("}");
  client.flush();
  delay(10);
  client.stop();
}

// ---------- SETUP FUNCTION ----------
void setup() {
  Serial.begin(115200);
  delay(1000);

  // Pin modes
  pinMode(LED_PIN, OUTPUT);
  pinMode(FAN_PIN, OUTPUT);
  pinMode(LAMP_PIN, OUTPUT);
  pinMode(TFT_BL, OUTPUT);
  digitalWrite(TFT_BL, HIGH);  // Turn on screen

  // Initialize DHT11
  dht.begin();
  if (!checkDHTSensor()) Serial.println("❌ DHT11 not working at startup!");

  // Initialize TFT display
  tft.init(240, 135);
  tft.setRotation(1);
  tft.fillScreen(ST77XX_BLACK);
  tft.setTextSize(2);
  tft.setTextColor(ST77XX_WHITE);
  tft.setCursor(10, 20);
  tft.println("🌡️ BILTONG MAKER 🌡️");

  // Connect Wi-Fi & start server
  connectToWiFi();
  server.begin();
}

// ---------- MAIN LOOP ----------
void loop() {
  // Handle incoming clients
  WiFiClient client = server.available();
  if (client) {
    String reqLine = client.readStringUntil('\n');
    Serial.println("🌐 New client connected: " + reqLine);
    if (reqLine.indexOf("GET /data") >= 0) handleData(client);
    else handleClient(client);
  }

  // Read DHT11
  float t = dht.readTemperature();
  float h = dht.readHumidity();
  if (isnan(t) || isnan(h)) {
    Serial.println("❌ Failed to read DHT11 sensor!");
    t = -99; h = -99;
  }

  // Serial Monitor: fancy output
  Serial.println("--------------------------------------------------------");
  Serial.println("🌡️  DHT11 SENSOR READINGS 🌡️");
  Serial.print("💧 Humidity: "); Serial.print(h); Serial.println(" %");
  Serial.print("🔥 Temperature: "); Serial.print(t); Serial.println(" °C");
  Serial.print("🌀 Fan: "); Serial.print(fanState ? "ON" : "OFF");
  Serial.print(", 💡 Lamp: "); Serial.println(lampState ? "ON" : "OFF");
  Serial.println("--------------------------------------------------------");

  // Update TFT display
  tft.fillScreen(ST77XX_BLACK);  // Clear old readings
  tft.setTextColor(ST77XX_GREEN);
  tft.setTextSize(2);
  tft.setCursor(10, 10); tft.println("🌡️ Temp & Humidity 🌡️");

  tft.setTextColor(ST77XX_CYAN);
  tft.setTextSize(2);
  tft.setCursor(20, 50); tft.print("Temp: "); tft.print(t); tft.println(" C");

  tft.setTextColor(ST77XX_YELLOW);
  tft.setCursor(20, 90); tft.print("Humidity: "); tft.print(h); tft.println(" %");

  // Automatic fan/lamp control
  if (t > TEMP_THRESHOLD) { fanState = true; lampState = false; }
  if (h > HUMID_THRESHOLD) fanState = true;
  digitalWrite(FAN_PIN, fanState ? HIGH : LOW);
  digitalWrite(LAMP_PIN, lampState ? HIGH : LOW);

  // LED heartbeat
  digitalWrite(LED_PIN, HIGH);
  delay(500);
  digitalWrite(LED_PIN, LOW);
  delay(500);
}
