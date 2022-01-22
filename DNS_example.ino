#include <ESP8266WiFi.h>
#include <ESP8266mDNS.h>

#ifndef STASSID
#define STASSID "<YOUR SSID>"
#define STAPSK  "<YOUR SSID PASSWORD>"
#endif

char URL2FIND[] = "www.google.nl";
IPAddress ipURL2FIND; 

const char* ssid = STASSID;
const char* password = STAPSK;

// Report the WiFi connection info to a terminal
void print_WiFi_info()
{
  byte bMac[6];

  Serial.println("");

  Serial.print(F("SSID: "));
  Serial.println(WiFi.SSID());

  Serial.print(F("IP Address: "));
  Serial.println(WiFi.localIP());

  WiFi.macAddress(bMac);
  Serial.print(F("Device MAC Address: "));
  for(int i = 5; i >= 1; i--)
  {
    Serial.print(bMac[i], HEX);
    Serial.print(":");
  }
  Serial.println(bMac[0]);

  Serial.print(F("Signal strength (RSSI): "));
  Serial.print(WiFi.RSSI());
  Serial.println(" dBm");

  Serial.println("");

  return;
}

void setup()
{
  Serial.begin(115200);
  Serial.println("Booting");

  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  while (WiFi.waitForConnectResult() != WL_CONNECTED)
  {
    Serial.println("Connection Failed! Rebooting...");
    delay(5000);
    ESP.restart();
  }

  print_WiFi_info();
}

void loop()
{
  Serial.print("The IP of ");
  Serial.print(URL2FIND);
  Serial.print(" is ");
  if (WiFi.hostByName(URL2FIND, ipURL2FIND))
  {
    Serial.println(ipURL2FIND);
  }
  else
  {
    Serial.println("not found!");
  }

  delay(5000);
}
