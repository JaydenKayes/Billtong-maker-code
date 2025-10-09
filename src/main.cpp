#include <Arduino.h> 
#include <WiFi.h>
#include "arduino_secrets.h" // Contains WiFi credentials (SSID and password)
#include "DHT.h" // Library for temperature and humidity sensor

// --- WiFi Setup ---
// These store your WiFi credentials from the secrets file.
const char SSID[] = SECRET_SSID;      
const char PASSWORD[] = SECRET_PASS;  
WiFiServer server(80); // Creates a web server on port 80 to serve the webpage

// --- Pin Assignments ---
// Assign physical pins on ESP32 for outputs and sensor input
const int LED_PIN = 13;       // Optional indicator LED
const int FAN_PIN = 26;       // Relay controlling fan
const int LAMP_PIN = 27;      // Relay controlling heat lamp
const int DHT_PIN = 4;        // Pin connected to DHT sensor data line

// --- DHT Sensor Setup ---
// DHT22 chosen because it has better accuracy than DHT11
#define DHTTYPE DHT22         
DHT dht(DHT_PIN, DHTTYPE);    // Initialize DHT sensor object

// --- Thresholds for automatic control ---
// If temperature or humidity exceeds these, automatic rules trigger
const float TEMP_THRESHOLD = 30.0;   
const float HUMID_THRESHOLD = 70.0;  

// --- Device States ---
// Keep track of whether the fan and lamp are ON or OFF
bool fanState = false;  
bool lampState = false; 

// --- Function: Connect to WiFi ---
// Handles connecting the ESP32 to your WiFi network
void connectToWiFi() {
  WiFi.mode(WIFI_STA);                // Set ESP32 to station mode
  WiFi.begin(SSID, PASSWORD);         // Start connection to network
  Serial.print("Connecting to WiFi");
  
  // Wait until ESP32 is connected
  while (WiFi.status() != WL_CONNECTED) { 
    Serial.print("."); // Print dots while connecting
    delay(500);
  }

  // Print confirmation and IP address
  Serial.println("\nConnected to WiFi!");
  Serial.print("IP address: http://");
  Serial.println(WiFi.localIP());     
}

// --- Function: Check if DHT sensor is working ---
// Returns true if sensor readings are valid, false otherwise
bool checkDHTSensor() {
  float temp = dht.readTemperature(); 
  float hum = dht.readHumidity();

  // If either reading is invalid (NaN), sensor has failed
  if (isnan(temp) || isnan(hum)) {
    Serial.println("⚠️ ERROR: DHT sensor not working! Check wiring, power, or sensor connection.");
    return false; 
  }

  // Sensor is responding correctly
  return true;
}

void setup() {
  Serial.begin(115200); // Start Serial monitor for debugging
  delay(1000);          // Wait 1 second to stabilize

  // --- Setup pin modes ---
  pinMode(LED_PIN, OUTPUT); // Optional indicator LED
  pinMode(FAN_PIN, OUTPUT); // Control fan relay
  pinMode(LAMP_PIN, OUTPUT);// Control lamp relay

  dht.begin();        // Start DHT sensor communication
  connectToWiFi();    // Connect ESP32 to WiFi
  server.begin();     // Start web server to serve webpage

  // --- Initial sensor check ---
  // Ensures DHT is working at startup
  if (!checkDHTSensor()) {
    Serial.println("⚠️ DHT sensor failed to initialize properly! Readings may not be accurate.");
  } else {
    Serial.println("✅ DHT sensor initialized successfully and working correctly.");
  }
}

// --- Function: Handle client requests for webpage ---
// Reads URL commands and updates fan/lamp or shows sensor values
void handleClient(WiFiClient client) {
  String request = client.readStringUntil('\r'); // Read HTTP request line
  Serial.println(request);

  // --- Manual control via URL ---
  // Example: /fanOn turns fan on manually
  if (request.indexOf("GET /fanOn") >= 0) {
    fanState = true;
  } else if (request.indexOf("GET /fanOff") >= 0) {
    fanState = false;
  } else if (request.indexOf("GET /lampOn") >= 0) {
    lampState = true;
  } else if (request.indexOf("GET /lampOff") >= 0) {
    lampState = false;
  }

  // --- Read sensor values ---
  float temp = dht.readTemperature();
  float hum = dht.readHumidity();

  // --- Check for sensor errors ---
  // isnan() checks if value is invalid; || means OR
  // If either temp or hum is NaN, use safe fallback values
  if (isnan(temp) || isnan(hum)) {
    Serial.println("⚠️ ERROR: Failed to read from DHT sensor during client request!");
    temp = -99; // Placeholder value to keep program running
    hum = -99;  
  }

  // --- Automatic control rules ---
  // Fan turns on if too hot or too humid
  // Lamp turns off if too hot to prevent extra heating
  if (temp > TEMP_THRESHOLD) {
    fanState = true;
    lampState = false;
  }
  if (hum > HUMID_THRESHOLD) {
    fanState = true;
  }

  // --- Update output pins ---
  digitalWrite(FAN_PIN, fanState ? HIGH : LOW);   // Turn fan on/off
  digitalWrite(LAMP_PIN, lampState ? HIGH : LOW); // Turn lamp on/off

  // --- Send webpage to client ---
  client.println("HTTP/1.1 200 OK");
  client.println("Content-Type: text/html");
  client.println("Connection: close");
  client.println();
  client.println("<!DOCTYPE html><html>");
  client.println("<head><meta charset='utf-8'>");
  client.println("<title>Biltong Maker</title>");
  
  // --- JavaScript for live sensor updates ---
  client.println("<script>");
  client.println("async function refreshData(){");
  client.println("  let r=await fetch('/data');"); // Get JSON from server
  client.println("  let j=await r.json();");       // Parse JSON
  client.println("  document.getElementById('temp').innerText=j.temp+' °C';");
  client.println("  document.getElementById('hum').innerText=j.hum+' %';");
  client.println("  document.getElementById('fan').innerText=j.fan?'ON':'OFF';");
  client.println("  document.getElementById('lamp').innerText=j.lamp?'ON':'OFF';");
  client.println("}");
  client.println("setInterval(refreshData,2000);"); // Update every 2 seconds
  client.println("</script></head>");
  
  // --- HTML layout for display and buttons ---
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
  
  client.flush();  // Make sure all data sent
  delay(10);
  client.stop();   // Close connection
}

// --- Function: Send JSON sensor data ---
// This is used by JavaScript on webpage to show live readings
void handleData(WiFiClient client) {
  float temp = dht.readTemperature();
  float hum = dht.readHumidity();

  // --- Handle sensor errors ---
  if (isnan(temp) || isnan(hum)) {
    Serial.println("⚠️ ERROR: DHT sensor not working during data request!");
    temp = -99;
    hum = -99;
  }

  // --- Send JSON response ---
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
  // --- Listen for incoming clients ---
  WiFiClient client = server.available();
  if (client) {
    Serial.println("New client connected");
    String reqLine = client.readStringUntil('\n'); // Read HTTP request line
    Serial.println(reqLine);
    
    // --- Route requests ---
    if (reqLine.indexOf("GET /data") >= 0) {
      handleData(client);     // Return live JSON data
    } else {
      handleClient(client);   // Return webpage
    }
  }
}
