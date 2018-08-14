#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ESP8266WebServer.h>
#include <ESP8266mDNS.h>
#include "secrets.h"

#define RED_LED D10
#define ORANGE_LED D11
#define YELLOW_LED D12
#define GREEN_LED D13
#define SERIAL_BAUD_RATE 115200
#define DELAY_TIME 500
#define ON 1
#define OFF 0

const char *ssid = SSID;
const char *password = WPA_KEY;

const char *authHeaderName = "AuthKey";
const char *authHeaderValue = AUTH_SECRET;

const char *loginUserName = LOGIN_USER_NAME;
const char *loginPassword = LOGIN_PASSWORD;

const char *APP_TITLE = "Work Status Board";

enum State
{
  state_allOff,
  state_allOn,
  state_coding,
  state_onACall,
  state_workingHard,
  state_hardlyWorking
} state;

ESP8266WebServer server(80);

void setup()
{
  // put your setup code here, to run once:
  Serial.begin(SERIAL_BAUD_RATE);

  pinMode(RED_LED, OUTPUT);
  pinMode(ORANGE_LED, OUTPUT);
  pinMode(YELLOW_LED, OUTPUT);
  pinMode(GREEN_LED, OUTPUT);

  allOn();
  delay(DELAY_TIME * 2);
  allOff();
  delay(DELAY_TIME * 2);

  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  Serial.println("");

  while (WiFi.status() != WL_CONNECTED)
  {
    cylon();
    Serial.print(".");
  }

  Serial.println("");
  Serial.print("Connected to ");
  Serial.println(ssid);
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());

  if (MDNS.begin("esp8266"))
  {
    Serial.println("MDNS responder started");
  }

  server.on("/", handleRoot);
  server.on("/login", handleLogin);
  server.on("/onACall", handleOnACall);
  server.on("/coding", handleCoding);
  server.on("/workingHard", handleWorkingHard);
  server.on("/hardlyWorking", handleHardlyWorking);
  server.on("/allOff", handleAllOff);

  server.onNotFound(handleNotFound);

  //here the list of headers to be recorded
  const char *headerkeys[] = {authHeaderName, "Cookie"};
  size_t headerkeyssize = sizeof(headerkeys) / sizeof(char *);
  //ask server to track these headers
  server.collectHeaders(headerkeys, headerkeyssize);

  server.begin();
  Serial.println("HTTP server started");

  allOff();
}

void handleRoot()
{
  String message = "<HTML><HEAD><TITLE>";
  message += APP_TITLE;
  message += "</TITLE></HEAD><BODY><H1>";
  message += APP_TITLE;
  message += "</H1>";
  message += "<H2>Current Status</H2>";
  message += currentStateHTML();
  message += "<HR />";
  if (checkAuth())
  {
    message += validEndpoints();
  }
  else
  {
    message += loginMessage();
  }
  message += "<HR />";
  message += "</BODY></HTML>";
  server.sendHeader("Cache-Control", "no-cache");
  server.send(200, "text/html", message);
  Serial.println("/");
}

String loginMessage()
{
  String message = "";
  message += "<p>Click <a href=\"/login\">here</a> to login and update current status</p>";
  return message;
}

String validEndpoints()
{
  String message = "";
  message += "<H2>Valid Endpoints</H2><UL>";
  message += "<LI><A href=\"onACall\">On a call</LI>";
  message += "<LI><A href=\"coding\">Coding</LI>";
  message += "<LI><A href=\"workingHard\">Working Hard</LI>";
  message += "<LI><A href=\"hardlyWorking\">Hardly Working</LI>";
  message += "<LI><A href=\"allOff\">All Off</LI>";
  message += "</UL><HR />";
  message += "<p><a href=\"/login?DISCONNECT=TRUE\">Logout</a></p>";

  return message;
}

bool checkAuth()
{
  bool isAuthenticated = false;
  // can be authenticated in 3 different ways
  // Header set with valid key
  if (server.hasHeader(authHeaderName) && server.header(authHeaderName) == authHeaderValue)
  {
    isAuthenticated = true;
  }

  // Key passed in url as arg
  if (server.hasArg(authHeaderName) && server.arg(authHeaderName) == authHeaderValue)
  {
    isAuthenticated = true;
  }

  // Logged in and auth key set as cookie
  if (!isAuthenticated && server.hasHeader("Cookie"))
  {
    String cookie = server.header("Cookie");
    String lookFor = authHeaderName;
    lookFor += "=";
    lookFor += authHeaderValue;
    if (cookie.indexOf(lookFor) != -1)
    {
      isAuthenticated = true;
    }
  }

  return isAuthenticated;
}

//login page, also called for disconnect
void handleLogin()
{
  String msg;

  if (server.hasArg("DISCONNECT"))
  {
    server.sendHeader("Location", "/");
    server.sendHeader("Cache-Control", "no-cache");
    String notLoggedInCookie = authHeaderName; 
    notLoggedInCookie += "=";    
    server.sendHeader("Set-Cookie", notLoggedInCookie);
    server.send(301);
    return;
  }

  if (server.hasArg("USERNAME") && server.hasArg("PASSWORD"))
  {
    if (server.arg("USERNAME") == loginUserName && server.arg("PASSWORD") == loginPassword)
    {
      server.sendHeader("Location", "/");
      server.sendHeader("Cache-Control", "no-cache");
      String loggedInCookie = authHeaderName; 
      loggedInCookie += "=";
      loggedInCookie += authHeaderValue;
      server.sendHeader("Set-Cookie", loggedInCookie);
      server.send(301);
      Serial.println("Log in Successful");
      return;
    }

    msg = "Wrong username/password! try again.";
    Serial.println("Log in Failed");
  }
  String content = "<html><body><h1>Login to ";
  content += APP_TITLE;
  content += "</h1><form action='/login' method='POST'>";
  content += "User:<input type='text' name='USERNAME' placeholder='user name'><br>";
  content += "Password:<input type='password' name='PASSWORD' placeholder='password'><br>";
  content += "<input type='submit' name='SUBMIT' value='Submit'></form>" + msg + "<br>";
  content += "You also can go <a href='/'>home</a> to see status without logging in.</body></html>";
  server.send(200, "text/html", content);
}

void send401()
{
  server.send(401, "text/plain", "Authenticate to update status");
}

void gotoRoot()
{
  server.sendHeader("Location", "/");
  server.sendHeader("Cache-Control", "no-cache");
  server.send(302);
}

void handleAllOff()
{
  if (!checkAuth())
  {
    send401();
  }
  else
  {
    allOff();
    Serial.println("All Off");
    gotoRoot();
  }
}

void handleOnACall()
{
  if (!checkAuth())
  {
    send401();
  }
  else
  {
    onACall();
    Serial.println("On a Call");
    gotoRoot();
  }
}

void handleCoding()
{
  if (!checkAuth())
  {
    send401();
  }
  else
  {
    coding();
    gotoRoot();
  }
}

void handleWorkingHard()
{
  if (!checkAuth())
  {
    send401();
  }
  else
  {
    workingHard();
    Serial.println("Working Hard");
    gotoRoot();
  }
}

void handleHardlyWorking()
{
  if (!checkAuth())
  {
    send401();
  }
  else
  {
    hardlyWorking();
    Serial.println("Hardly Working");
    gotoRoot();
  }
}

void handleNotFound()
{
  String message = "File Not Found\n\n";
  message += "URI: ";
  message += server.uri();
  message += "\nMethod: ";
  message += (server.method() == HTTP_GET) ? "GET" : "POST";
  message += "\nArguments: ";
  message += server.args();
  message += "\n";
  for (uint8_t i = 0; i < server.args(); i++)
  {
    message += " " + server.argName(i) + ": " + server.arg(i) + "\n";
  }
  Serial.println("File not Found");
  server.send(404, "text/plain", message);
}

void loop()
{
  // put your main code here, to run repeatedly:
  server.handleClient();
}

void onACall()
{
  allOff();
  setLed(ORANGE_LED, ON);
  state = state_onACall;
}

void coding()
{
  allOff();
  setLed(RED_LED, ON);
  state = state_coding;
}

void workingHard()
{
  allOff();
  setLed(YELLOW_LED, ON);
  state = state_workingHard;
}

void hardlyWorking()
{
  allOff();
  setLed(GREEN_LED, ON);
  state = state_hardlyWorking;
}

void setLed(int led, int state)
{
  digitalWrite(led, state);
}

void allOff()
{
  digitalWrite(RED_LED, OFF);
  digitalWrite(ORANGE_LED, OFF);
  digitalWrite(YELLOW_LED, OFF);
  digitalWrite(GREEN_LED, OFF);
  state = state_allOff;
}

void allOn()
{
  digitalWrite(RED_LED, ON);
  digitalWrite(ORANGE_LED, ON);
  digitalWrite(YELLOW_LED, ON);
  digitalWrite(GREEN_LED, ON);
  state = state_allOn;
}

void cycleLights()
{
  digitalWrite(GREEN_LED, OFF);
  digitalWrite(RED_LED, ON);
  delay(DELAY_TIME);
  digitalWrite(RED_LED, OFF);
  digitalWrite(ORANGE_LED, ON);
  delay(DELAY_TIME);
  digitalWrite(ORANGE_LED, OFF);
  digitalWrite(YELLOW_LED, ON);
  delay(DELAY_TIME);
  digitalWrite(YELLOW_LED, OFF);
  digitalWrite(GREEN_LED, ON);
  delay(DELAY_TIME);
}

void cylon()
{
  setLed(GREEN_LED, ON);
  setLed(YELLOW_LED, OFF);
  delay(DELAY_TIME);
  setLed(YELLOW_LED, ON);
  setLed(GREEN_LED, OFF);
  delay(DELAY_TIME);
  setLed(ORANGE_LED, ON);
  setLed(YELLOW_LED, OFF);
  delay(DELAY_TIME);
  setLed(RED_LED, ON);
  setLed(ORANGE_LED, OFF);
  delay(DELAY_TIME);
  setLed(ORANGE_LED, ON);
  setLed(RED_LED, OFF);
  delay(DELAY_TIME);
  setLed(YELLOW_LED, ON);
  setLed(ORANGE_LED, OFF);
  delay(DELAY_TIME);
}

String currentStateHTML()
{
  String html = "<P>";

  switch (state)
  {
  case state_allOff:
    html += "All lights off";
    break;
  case state_allOn:
    html += "All lights on";
    break;
  case state_coding:
    html += "<font color=\"red\">CODING</font>";
    break;
  case state_onACall:
    html += "<font color=\"orange\">On a call</font>";
    break;
  case state_workingHard:
    html += "<font color=\"yellow\">Working hard</font>";
    break;
  case state_hardlyWorking:
    html += "<font color=\"green\">Hardly working</font>";
    break;
  default:
    html += "Unknown";
  }

  html += "</P>";

  return html;
}
