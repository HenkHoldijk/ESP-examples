#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ESP8266WebServer.h>
#include <ESP8266mDNS.h>

// HW related defines
#define IO14_PIN      ( 14 )
#define LED_PIN       ( 13 )
#define RELAY_PIN     ( 12 )
#define BUTTON_PIN    ( 0 )
#define LED_ON        ( 0 )
#define LED_OFF       ( 1 )
#define RELAY_ON      ( 1 )
#define RELAY_OFF     ( 0 )
#define IO_LOW        ( 0 )
#define IO_HIGH       ( 1 )

// Web page password related defines
#define MY_PWD        ( "qwerty" )  // PWD before any LED, Relay or output-pin control is allowed
#define PWD_EXPIRE_S  ( 60 )        // PWD expiration time, see also refresh time inside CONTROL_HTML

// WiFi network info
const char* ssid     = "<YOUR SSID>";
const char* password = "<YOUR SSID PASSWORD>";

// Storage for the MAC address
byte mac[6];

// Password status
boolean       pwd_valid = false;
unsigned long pwd_time  = ((unsigned long) -1) - (PWD_EXPIRE_S * 1000);

// Server at port 80 (default HTTP)
ESP8266WebServer server(80);

// Web page, asking for the password
const char ASK_PWD_HTML[] =
"<!DOCTYPE HTML>"
"<html>"
"<head>"
"<meta name = \"viewport\" content = \"width = device-width, initial-scale = 1.0, maximum-scale = 1.0, user-scalable = 0\">"
"<title>Sonoff Password Demo</title>"
"<style>"
"\"body { background-color: #808080; font-family: Arial, Helvetica, Sans-Serif; Color: #000000; }\""
"</style>"
"</head>"
"<body>"
"<h1>Sonoff Password Demo</h1>"
"<form action = \"/\" method = \"post\">"
"<p>"
"Password: <input type = \"password\" name = \"PWD\" value = \"\">&emsp;"
"<input type = \"submit\" value = \"Send\">"
"</p>"
"</form>"
"</body>"
"</html>";

// Web page, reporting password is OK
const char PWD_OK_HTML[] =
"<!DOCTYPE HTML>"
"<html>"
"<head>"
"<meta name = \"viewport\" content = \"width = device-width, initial-scale = 1.0, maximum-scale = 1.0, user-scalable = 0\">"
"<meta http-equiv = \"refresh\" content = \"1\"/>"
"<title>Sonoff Password Demo</title>"
"<style>"
"\"body { background-color: #808080; font-family: Arial, Helvetica, Sans-Serif; Color: #000000; }\""
"</style>"
"</head>"
"<body>"
"<h1>Sonoff Password Demo</h1>"
"Password OK !"
"</body>"
"</html>";

// Web page, reporting password is wrong
const char PWD_FAIL_HTML[] =
"<!DOCTYPE HTML>"
"<html>"
"<head>"
"<meta name = \"viewport\" content = \"width = device-width, initial-scale = 1.0, maximum-scale = 1.0, user-scalable = 0\">"
"<meta http-equiv = \"refresh\" content = \"5\"/>"
"<title>Sonoff Password Demo</title>"
"<style>"
"\"body { background-color: #808080; font-family: Arial, Helvetica, Sans-Serif; Color: #000000; }\""
"</style>"
"</head>"
"<body>"
"<h1>Sonoff Password Demo</h1>"
"Wrong Password !!!"
"</body>"
"</html>";

// Web page, allowing LED, Relay and IO control
const char CONTROL_HTML[] =
"<!DOCTYPE HTML>"
"<html>"
"<head>"
"<meta name = \"viewport\" content = \"width = device-width, initial-scale = 1.0, maximum-scale = 1.0, user-scalable = 0\">"
"<meta http-equiv=\"refresh\" content=\"60\"/>"
"<title>Sonoff Password Demo</title>"
"<style>"
"\"body { background-color: #808080; font-family: Arial, Helvetica, Sans-Serif; Color: #000000; }\""
"</style>"
"</head>"
"<body>"
"<h1>Sonoff Password Demo</h1>"
"<table><tr>"
"<td>&emsp;On-Board LED&emsp;</td><td align=\"center\"><a href = \"LedOn\"><button>ON</button></a>&nbsp;</td><td align=\"center\"><a href = \"LedOff\"><button>OFF</button></a></td>"
"</tr><tr>"
"<td>&emsp;On-Board Relay&emsp;</td><td align=\"center\"><a href = \"RelayOn\"><button>ON</button></a>&nbsp;</td><td align=\"center\"><a href = \"RelayOff\"><button>OFF</button></a></td>"
"</tr><tr>"
"<td>&emsp;IO-14 pin&emsp;</td><td align=\"center\"><a href = \"IO14High\"><button>High</button></a>&nbsp;</td><td align=\"center\"><a href = \"IO14Low\"><button>Low</button></a></td>"
"</table></tr>"
"</body>"
"</html>";



// #################################################
// # Report the WiFi connection info to a terminal #
// #################################################
void printWifiInfo()
{
  Serial.println("");

  Serial.print(F("SSID: "));
  Serial.println(WiFi.SSID());

  Serial.print(F("IP Address: "));
  Serial.println(WiFi.localIP());

  WiFi.macAddress(mac);
  Serial.print(F("Device MAC Address: "));
  for(int i = 5; i >= 1; i--) {
    Serial.print(mac[i], HEX);
    Serial.print(":");
  }
  Serial.println(mac[0]);

  Serial.print(F("Signal strength (RSSI): "));
  Serial.print(WiFi.RSSI());
  Serial.println(" dBm");

  Serial.println("");

  return;
}



// ################################
// # Basic HTML WEB page handlers #
// ################################

// Handle a Client HTTP request at Root level
void handleRoot()
{
  if (server.hasArg("PWD"))
  {
    handleSubmit();
  }
  else
  {
    if (pwd_valid)
    {
	    server.send(200, "text/html", CONTROL_HTML);
	  }
	  else
	  {
      server.send(200, "text/html", ASK_PWD_HTML);
    }
  }

  return;
}

// Close the connection with an OK
void returnOK()
{
  Serial.println(F("Connection closed with OK!"));

  server.sendHeader("Connection", "close");
  server.sendHeader("Access-Control-Allow-Origin", "*");
  server.send(200, "text/plain", "OK\r\n");

  return;
}

// Close the connection due to a failure
void returnFail(String msg)
{
  Serial.println(F("Connection closed with FAIL!"));

  server.sendHeader("Connection", "close");
  server.sendHeader("Access-Control-Allow-Origin", "*");
  server.send(500, "text/plain", msg + "\r\n");

  return;
}

// Handle a Client HTTP request at Root level
void handleSubmit()
{
  String Password;

  if (!server.hasArg("PWD")) return returnFail("Stop hacking !");
  
  Password = server.arg("PWD");
  Password.toLowerCase();

  if (Password.length() == 0)
  {
    server.send(200, "text/html", ASK_PWD_HTML);
  }
  else if (Password == MY_PWD)
  {
  	pwd_valid = true;
    pwd_time  = millis();

    Serial.println(F("Valid Password!"));

    server.send(200, "text/html", PWD_OK_HTML);
    delay(500);
    server.send(200, "text/html", CONTROL_HTML);
  }
  else
  {
  	pwd_valid = false;
    pwd_time  = ((unsigned long) -1) - (PWD_EXPIRE_S * 1000);

    Serial.println(F("Wrong Password!"));

    server.send(200, "text/html", PWD_FAIL_HTML);
    delay(4000);
    server.send(200, "text/html", ASK_PWD_HTML);
  }

  return;
}

// Handle not found
void handleNotFound()
{
  String message = "File Not Found\n\n";
  message += "URI: ";
  message += server.uri();
  message += "\nMethod: ";
  message += (server.method() == HTTP_GET)?"GET":"POST";
  message += "\nArguments: ";
  message += server.args();
  message += "\n";
  for (uint8_t i=0; i<server.args(); i++){
    message += " " + server.argName(i) + ": " + server.arg(i) + "\n";
  }
  server.send(404, "text/plain", message);

  return;
}



// #############
// # handle IO #
// #############
void handleLedOn()
{
  if (pwd_valid)
  {
    pwd_time = millis();

    server.send(200, "text/html", CONTROL_HTML);
    digitalWrite(LED_PIN, LED_ON);
    delay(1000);
  }
  else
  {
    server.send(200, "text/html", ASK_PWD_HTML);
  }

  return;
}

void handleLedOff()
{
  if (pwd_valid)
  {
    pwd_time = millis();

    server.send(200, "text/html", CONTROL_HTML);
    digitalWrite(LED_PIN, LED_OFF);
    delay(1000);
  }
  else
  {
    server.send(200, "text/html", ASK_PWD_HTML);
  }

  return;
}

void handleRelayOn()
{
  if (pwd_valid)
  {
    pwd_time = millis();

    server.send(200, "text/html", CONTROL_HTML);
    digitalWrite(RELAY_PIN, RELAY_ON);
    delay(1000);
  }
  else
  {
    server.send(200, "text/html", ASK_PWD_HTML);
  }

  return;
}

void handleRelayOff()
{
  if (pwd_valid)
  {
    pwd_time = millis();

    server.send(200, "text/html", CONTROL_HTML);
    digitalWrite(RELAY_PIN, RELAY_OFF);
    delay(1000);
  }
  else
  {
    server.send(200, "text/html", ASK_PWD_HTML);
  }

  return;
}

void handleIO14High()
{
  if (pwd_valid)
  {
    pwd_time = millis();

    server.send(200, "text/html", CONTROL_HTML);
    digitalWrite(IO14_PIN, IO_HIGH);
    delay(1000);
  }
  else
  {
    server.send(200, "text/html", ASK_PWD_HTML);
  }

  return;
}

void handleIO14Low()
{
  if (pwd_valid)
  {
    pwd_time = millis();

    server.send(200, "text/html", CONTROL_HTML);
    digitalWrite(IO14_PIN, IO_LOW);
    delay(1000);
  }
  else
  {
    server.send(200, "text/html", ASK_PWD_HTML);
  }

  return;
}



// #########
// # Setup #
// #########
void setup(void)
{
  // Switch On-Board LED OFF
  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, LED_OFF);

  // Switch On-Board Relay OFF
  pinMode(RELAY_PIN, OUTPUT);
  digitalWrite(RELAY_PIN, RELAY_OFF);

  // Set IO pin 14 to output and LOW
  pinMode(IO14_PIN, OUTPUT);
  digitalWrite(IO14_PIN, IO_LOW);

  // Set the Serial port to 115200 baud
  Serial.begin(115200);
  while (!Serial)
  {
    delay(100);
  }

  // Init the WiFi connection
  WiFi.begin(ssid, password);
  Serial.println("");

  // Wait for the WiFi connection
  Serial.println("");
  Serial.print(F("Connecting."));
  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.println(F("Connection Successful!"));

  // Connected, so report the netwok info
  printWifiInfo();

  // Start Multicast DNS responder
  if (MDNS.begin("esp8266", WiFi.localIP()))
  {
    Serial.println(F("MDNS responder started"));
  }

  // Set the web page handlers
  server.on("/", handleRoot);
  server.on("/submit", handleSubmit);
  server.on("/LedOn", handleLedOn);
  server.on("/LedOff", handleLedOff);
  server.on("/RelayOn", handleRelayOn);
  server.on("/RelayOff", handleRelayOff);
  server.on("/IO14High", handleIO14High);
  server.on("/IO14Low", handleIO14Low);
  server.onNotFound(handleNotFound);

  // Start the HTTP-server
  server.begin();
  Serial.println(F("HTTP server started"));
} 



// #############
// # Main loop #
// #############
void loop(void)
{
  // Handle the client requests
  server.handleClient();

  // Check if PWD entry has expired
  if (millis() > (pwd_time + (PWD_EXPIRE_S * 1000)))
  {
    pwd_valid = false;
    pwd_time  = ((unsigned long) -1) - (PWD_EXPIRE_S * 1000);

    Serial.println(F("Password expired!"));
  }

  // handle the serial commands
  HandleUserCommand();
}



// ######################
// # software reset     #
// # ------------------ #
// # reset cause msg:   #
// #  0:                #
// #  1: normal boot    #
// #  2: reset pin      #
// #  3: software reset #
// #  4: watchdog reset #
// ######################
void SoftReset()
{
  ESP.restart();

  return;
}



// #######################
// # handle user command #
// #######################
void HandleUserCommand()
{
  static String sSerialInput;

  // quit now if we don't have to handle a serial command
  if (Serial.available() < 1)
  {
    return;
  }

  // read the serial command and make it upper case
  sSerialInput = Serial.readStringUntil('\n');
  sSerialInput.replace("\r", "");
  sSerialInput.replace("\n", "");
  sSerialInput.replace(" ", "");
  sSerialInput.toUpperCase();

  // quit now if it's an empty serial command (quick response)
  if (sSerialInput.length() < 1)
  {
    return;
  }

  // parse the serial command
  if (sSerialInput == "?")
  {
    printWifiInfo();
  }
  else if (sSerialInput == "RESET")
  {
    // Note; it remembers the previous GPIO0 level (boot mode)
    Serial.println();
    Serial.println(F("Reset (in same mode as last power-up) requested !!!"));
    delay(5000);
    SoftReset();
  }
  else
  {
    Serial.println();
    Serial.println(F("Duhhh?"));
  }

  return;
}
