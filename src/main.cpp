#include <Arduino.h>
#include <WiFi.h>
#include "arduino_secrets.h"

const char SSID[] = SECRET_SSID;
const char PASSWORD[] = SECRET_PASS;

//HAN Notes - what does this object do?
WiFiServer server(80);

const byte LEDPIN = 13;
const byte SENSORPIN = A5;

// Function prototype for flashLED
void flashLED(int interval_ms, int repeatCount);

void initWiFi() {
  WiFi.mode(WIFI_STA);
  WiFi.begin(SSID, PASSWORD);
  Serial.print("Connecting to WiFi");
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print('.');
    delay(500);
  }
  Serial.println();
  Serial.print("Connected: ");
  Serial.println(SSID);
  Serial.print("IP Address: http://");
  Serial.println(WiFi.localIP());
}

void setup() {
  pinMode(LEDPIN, OUTPUT);
  pinMode(SENSORPIN, INPUT);
  Serial.begin(115200);
  delay(1000);
  initWiFi();
  server.begin();
}

void loop() {
  // Flash LED 5 times with 500 ms interval
  flashLED(500, 5);

  WiFiClient client = server.available();
  if (client) {
    Serial.println("New client connected");
    String currentLine = "";

    while (client.connected()) {
      if (client.available()) {
        char c = client.read();
        Serial.write(c);

        if (c == '\n') {
          // Blank line = end of HTTP headers
          if (currentLine.length() == 0) {
            client.println("HTTP/1.1 200 OK");
            client.println("Content-Type: text/html");
            client.println("Connection: close");
            client.println("Refresh: 5");
            client.println();
            client.println("<!DOCTYPE HTML><html>");
            client.println("<style>html{font-family:Arial;}</style>");
            client.println("<h1>Sensor Status</h1>");

            int sensorReading = analogRead(SENSORPIN);
            client.print("Raw value: ");
            client.println(sensorReading);

            bool ledState = digitalRead(LEDPIN);
            client.print("Red LED is ");
            client.println(ledState ? "ON" : "OFF");

            client.println("<br><a href=\"/H\">Turn LED ON</a><br>");
            client.println("<a href=\"/L\">Turn LED OFF</a><br>");
            client.println("</html>");
            break;
          } else {
            // Analyze request line
            if (currentLine.endsWith("GET /H")) {
              digitalWrite(LEDPIN, HIGH);
            }
            if (currentLine.endsWith("GET /L")) {
              digitalWrite(LEDPIN, LOW);
            }
            currentLine = "";
          }
        } else if (c != '\r') {
          currentLine += c;
        }
      }
    }

    client.stop();
    Serial.println("Client disconnected");
  }
}

// Definition of flashLED
void flashLED(int interval_ms, int repeatCount) {
  for (int i = 0; i < repeatCount; ++i) {
    digitalWrite(LEDPIN, HIGH);
    delay(interval_ms);
    digitalWrite(LEDPIN, LOW);
    delay(interval_ms);
  }
}
