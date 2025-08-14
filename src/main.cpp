#include <Arduino.h>
#include <WiFi.h>
#include "arduino_secrets.h" // Contains SECRET_SSID and SECRET_PASS

const char SSID[] = SECRET_SSID;
const char PASSWORD[] = SECRET_PASS;

WiFiServer server(80);

const int LED_PIN = 13;
const int SENSOR_PIN = 34; n 

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

  pinMode(LED_PIN, OUTPUT);
  pinMode(SENSOR_PIN, INPUT);

  connectToWiFi();
  server.begin();
}

void handleClient(WiFiClient client) {
  String request = client.readStringUntil('\r');
  Serial.println(request);

  if (request.indexOf("GET /H") >= 0) {
    digitalWrite(LED_PIN, HIGH);
  } else if (request.indexOf("GET /L") >= 0) {
    digitalWrite(LED_PIN, LOW);
  }

  int sensorValue = analogRead(SENSOR_PIN);
  bool ledState = digitalRead(LED_PIN);

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
  client.stop();
  Serial.println("Client disconnected");
}

void loop() {
  WiFiClient client = server.available();
  if (client) {
    Serial.println("New client connected");
    handleClient(client);
  }
}
