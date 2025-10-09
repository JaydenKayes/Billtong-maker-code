#include <Arduino.h> 
#include <WiFi.h>
#include "arduino_secrets.h" // Contains WiFi credentials (SSID and password)
#include "DHT.h" // Library for temperature and humidity sensor

// --- WiFi Setup ---
const char SSID[] = SECRET_SSID;      // Network name
const char PASSWORD[] = SECRET_PASS;  // Network password
WiFiServer server(80);                // Creates a web server on port 80

// --- Pin Assignments ---
const int LED_PIN = 13;       // Indicator LED (optional)
const int FAN_PIN = 26;       // Fan relay output
const int LAMP_PIN = 27;      // Heat lamp relay output
const int DHT_PIN = 4;        // DHT sensor data pin

// --- DHT Setup ---
#define DHTTYPE DHT11         // Choose between DHT11 or DHT22 sensor
DHT dht(DHT_PIN, DHTTYPE);    // Initialize DHT sensor

// --- Thresholds for automatic control ---
const float TEMP_THRESHOLD = 30.0;   // Max temperature before fan turns on
const float HUMID_THRESHOLD = 70.0;  // Max humidity before fan turns on

// --- Device States ---
bool fanState = false;  // Stores fan ON/OFF status
bool lampState = false; // Stores lamp ON/OFF status

// --- Connects to WiFi network ---
void connectToWiFi() {
  WiFi.mode(WIFI_STA);                // Set WiFi mode to station
  WiFi.begin(SSID, PASSWORD);         // Connect to WiFi
  Serial.print("Connecting to WiFi");
  while (WiFi.status() != WL_CONNECTED) { // Wait until connected
    Serial.print(".");
    delay(500);
  }
  Serial.println("\nConnected to WiFi!");
  Serial.print("IP address: http://");
  Serial.println(WiFi.localIP());     // Print device IP address
}

// --- Function to check if the DHT sensor is working properly ---
bool checkDHTSensor() {
  // Try to read both temperature and humidity values
  float temp = dht.readTemperature();
  float hum = dht.readHumidity();

  // If either reading is NaN (not a number), the sensor isn’t responding correctly
  if (isnan(temp) || isnan(hum)) {
    Serial.println("⚠️ ERROR: DHT sensor not working! Check wiring, power, or sensor connection.");
    return false; // Return false meaning the sensor has failed
  }

  // If both readings are valid, return true meaning the sensor is working
  return true;
}

void setup() {
  Serial.begin(115200);
  delay(1000);

  // Setup pin modes
  pinMode(LED_PIN, OUTPUT);
  pinMode(FAN_PIN, OUTPUT);
  pinMode(LAMP_PIN, OUTPUT);

  dht.begin();        // Start DHT sensor
  connectToWiFi();    // Connect to WiFi network
  server.begin();     // Start web server

  // --- Initial sensor check at startup ---
  // This checks if the DHT sensor is connected and giving valid readings.
  // If not, a warning will appear in the Serial Monitor before anything else runs.
  if (!checkDHTSensor()) {
    Serial.println("⚠️ DHT sensor failed to initialize properly! Readings may not be accurate.");
  } else {
    Serial.println("✅ DHT sensor initialized successfully and working correctly.");
  }
}

// --- Handles webpage requests and manual button commands ---
void handleClient(WiFiClient client) {
  String request = client.readStringUntil('\r'); // Read incoming request
  Serial.println(request);

  // --- Manual control commands via URL ---
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

  // --- Handle sensor read errors ---
  // If the sensor doesn’t respond, both temp and hum will be NaN (not a number).
  // When this happens, it will print a clear error message to the Serial Monitor.
  if (isnan(temp) || isnan(hum)) {
    Serial.println("⚠️ ERROR: Failed to read from DHT sensor during client request! Possible disconnection or malfunction.");
    temp = -99; // Placeholder value so the program keeps running safely
    hum = -99;  // Placeholder for humidity
  }

  // --- Automatic fan/lamp control rules ---
  if (temp > TEMP_THRESHOLD) {
    fanState = true;   // Turn fan on if too hot
    lampState = false; // Turn off lamp to prevent extra heat
  }
  if (hum > HUMID_THRESHOLD) {
    fanState = true;   // Turn on fan if humidity too high
  }

  // --- Update output pins ---
  digitalWrite(FAN_PIN, fanState ? HIGH : LOW);
  digitalWrite(LAMP_PIN, lampState ? HIGH : LOW);

  // --- Send HTML webpage back to client ---
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
  client.println("  let r=await fetch('/data');");
  client.println("  let j=await r.json();");
  client.println("  document.getElementById('temp').innerText=j.temp+' °C';");
  client.println("  document.getElementById('hum').innerText=j.hum+' %';");
  client.println("  document.getElementById('fan').innerText=j.fan?'ON':'OFF';");
  client.println("  document.getElementById('lamp').innerText=j.lamp?'ON':'OFF';");
  client.println("}");
  client.println("setInterval(refreshData,2000);"); // Auto refresh every 2 seconds
  client.println("</script></head>");
  
  // --- Simple HTML layout for display and control ---
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
  
  client.flush();  // Ensure data is sent
  delay(10);
  client.stop();   // Close connection
}

// --- Sends live temperature/humidity data in JSON format ---
void handleData(WiFiClient client) {
  float temp = dht.readTemperature();
  float hum = dht.readHumidity();

  // --- Default fallback if sensor fails ---
  // If readings fail, print an error message and send a -99 value
  // to the webpage to indicate an issue with the sensor.
  if (isnan(temp) || isnan(hum)) {
    Serial.println("⚠️ ERROR: DHT sensor not working during data request! Sending fallback values (-99).");
    temp = -99;
    hum = -99;
  }

  // --- Send JSON data to client ---
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
    String reqLine = client.readStringUntil('\n'); // Read request line
    Serial.println(reqLine);
    
    // --- Route request to correct handler ---
    // If the client requests “/data”, send JSON.
    // Otherwise, load the main webpage.
    if (reqLine.indexOf("GET /data") >= 0) {
      handleData(client);     // Return JSON data
    } else {
      handleClient(client);   // Return webpage
    }
  }
}
