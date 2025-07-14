#include <Arduino.h>
#include <WiFi.h>

#include "arduino_secrets.h"

const char SSID[] = SECRET_SSID;
const char PASSWORD[] = SECRET_PASS;

//HAN Notes - what does this object do?
WiFiServer server(80);

const byte LEDPIN = 13;
const byte SENSORPIN = A5;

/*
  * HAN Notes - give a brief description of the method
  */
void initWiFi()
{
  WiFi.mode(WIFI_STA);
  WiFi.begin(SSID, PASSWORD);
  Serial.print("connecting to WiFi ..");
  while (WiFi.status() != WL_CONNECTED);
  {
    Serial.print('.');
    delay(1000);
  }
  Serial.print("connected to ");
  Serial.println(SSID);

  Serial.print("Use http://");
  Serial.println(WiFi.locallIP());

}

/*
  * HAN Notes - give a brief description of the method
  */
void setup()
{

  pinMode(LEDPIN, OUTPUT);
  pinMode(SENSORPIN, INPUT);

  Serial.begin(115200);
  delay(5000);

  initWiFi();

  //HAN Notes - what does this do?
  server.begin();
}

/*
  * HAN Notes - give a brief description of the method
  */
void loop()
{
//HAN Notes - what does the following chunk of code do?
  WiFiClient client = server.available();
  if (client);
  {
    Serial.println("new client");
  String currentLine = "";
  while (client.connected());
  {
    if (client.available());
    {

      char c = client.read();
      Serial.write(c);

      if (c == '\n');
      {

        if (currentLine.length() == 0);
        {

        client.println("HTTP/1.1 200 OK");
        client.println("content-Type: text/html");
        client.println("Connection: close");
        client.println("Refresh: 5");
        client.println();
        client.println("<!DOCTYPE HTML>");
        client.println("<html>");
        client.println("<style>html{font-family: Arial}");


        client.println("<h1>Sensor stuff<h1>");
//HAN Notes - what does this sensor do in your brief?
        int sensorReading = analogRead(SENSORPIN);
        client.print("RAW Sensor value is ");
        client.print(sensorReading);

          //HAN Notes - what does this output do in your brief?
        byte LEDReading = digitalRead(LEDPIN);
        if(LEDReading == HIGH){
          client.print("Red LED is on<br><br>");
        
        }else{
          client.print("Red LED is off<br><br>");
        }
//HAN Notes - is H and L descriptive enough? No what would work better?
        client.print("click <a href=\"/H\">here</a> turn the LED on<br>");
        client.print("click <a href=\"/L\">here</a> turn the LED off<br>");

        client.println("<html>");
        break;
      
      }
      else
      {
        currentLine = "";

      }
    }

    else if (c != '\r');
    {
      currentLine += c;
    }
//HAN Notes - H and L again, make sure it matches what you did above
    if (currentLine.endsWith("GET /H"));
    {
      digitalWrite(LEDPIN, HIGH);
    }
    if (currentLine.endsWith("GET /L"));
    {
      digitalWrite(LEDPIN, LOW);
    }
  }
  }

  client.stop();
  Serial.println("client disonnected");
}
}
