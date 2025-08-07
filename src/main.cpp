#include <Arduino.h>
#include <WiFi.h>
#include "arduino_secrets.h" // Contains SSID and PASSWORD

const char SSID[] = SECRET_SSID;
const char PASSWORD[] = SECRET_PASS;

WiFiServer server(80); // Web server runs on port 80

const int LED_PIN = 13;      // Built-in LED pin
const int SENSOR_PIN = 34;   // Valid analog pin for ESP32

// Flashes LED on and off for a given number of times
void flashLED(int interval_ms, int repeatCount) {
  for (int i = 0; i < repeatCount; i++) {
    digitalWrite(LED_PIN, HIGH);
    delay(interval_ms);
    digitalWrite(LED_PIN, LOW);
    delay(interval_ms);
  }
}

// Connect to WiFi network
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
  Serial.begin(115200);  // Start serial monitor
  delay(1000);

  pinMode(LED_PIN, OUTPUT);
  pinMode(SENSOR_PIN, INPUT);

  connectToWiFi(); // Start WiFi connection
  server.begin();  // Start web server
}

// Handle a connected web client
void handleClient(WiFiClient client) {
  String request = client.readStringUntil('\r'); // Read HTTP request line
  Serial.println(request);

  // LED control via URL
  if (request.indexOf("GET /H") >= 0) {
    digitalWrite(LED_PIN, HIGH);  // Turn LED on
  } else if (request.indexOf("GET /L") >= 0) {
    digitalWrite(LED_PIN, LOW);   // Turn LED off
  }

  // Read sensor and LED state
  int sensorValue = analogRead(SENSOR_PIN);
  bool ledState = digitalRead(LED_PIN);

  // Send basic HTML response
  client.println("HTTP/1.1 200 OK");
  client.println("Content-Type: text/html");
  client.println("Connection: close");
  client.println();
  client.println("<!DOCTYPE html><html>");
  client.println("<head><meta http-equiv='refresh' content='5'><style>body{font-family:Arial;}</style></head>");
  client.println("<body>");
  client.println("<h1>Sensor Status</h1>");
  client.print("<p>Raw Value: ");
  client.print(sensorValue);
  client.println("</p>");
  client.print("<p>Red LED is ");
  client.print(ledState ? "ON" : "OFF");
  client.println("</p>");
  client.println("<a href=\"/H\">Turn LED ON</a><br>");
  client.println("<a href=\"/L\">Turn LED OFF</a><br>");
  client.println("</body></html>");
  client.flush();
  delay(10);
  client.stop(); // End connection
  Serial.println("Client disconnected");
}

void loop() {
  flashLED(500, 5); // Blink LED for feedback

  WiFiClient client = server.available(); // Check for incoming connection
  if (client) {
    Serial.println("New client connected");
    handleClient(client);
  }
}