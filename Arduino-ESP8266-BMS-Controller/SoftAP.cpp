#include "Arduino.h"

#include "softap.h"
#include "settings.h"
#include <ESP8266mDNS.h>

#include <ESP8266WebServer.h>

ESP8266WebServer server(80);

const char* ssid = "DIY_BMS_CONTROLLER";


String networks;

void handleNotFound()
{
  String message = "File Not Found\n\n";
  server.send(404, "text/plain", message);
}
void sendHeaders()
{
  server.sendHeader("Connection", "close");
  server.sendHeader("Cache-Control", "private");
}
String htmlHeader() {
  return String(F("<!DOCTYPE HTML>\r\n<html><head><style>.page {width:300px;margin:0 auto 0 auto;background-color:cornsilk;font-family:sans-serif;padding:22px;} label {min-width:120px;display:inline-block;padding: 22px 0 22px 0;}</style></head><body><div class=\"page\"><h1>DIY BMS</h1>"));
}
String htmlFooter() {
  return String(F("</div></body></html>\r\n\r\n"));
}
void handleRoot()
{
  String s;
  s = htmlHeader();
  //F Macro - http://arduino-esp8266.readthedocs.io/en/latest/PROGMEM.html
  s += F("<h2>WiFi Setup</h2><p>Select local WIFI to connect to:</p><form autocomplete=\"off\" method=\"post\" enctype=\"application/x-www-form-urlencoded\" action=\"\\save\"><label for=\"ssid\">SSID:</label><select id=\"ssid\" name=\"ssid\">");
  s += networks;
  s += F("</select><label for=\"pass\">Password:</label><input type=\"password\" id=\"id\" name=\"pass\"><br/><input minlength=\"8\" maxlength=\"32\" type=\"submit\" value=\"Submit\"></form>");
  s += htmlFooter();

  sendHeaders();
  server.send(200, "text/html", s);
}

void handleSave() {
  String s;
  String ssid = server.arg("ssid");
  String password = server.arg("pass");

  if ((ssid.length() <= sizeof(myConfig_WIFI.wifi_ssid)) && (password.length() <= sizeof(myConfig_WIFI.wifi_passphrase))) {

    memset(&myConfig_WIFI,0,sizeof(wifi_eeprom_settings));

    ssid.toCharArray(myConfig_WIFI.wifi_ssid, sizeof(myConfig_WIFI.wifi_ssid));
    password.toCharArray(myConfig_WIFI.wifi_passphrase, sizeof(myConfig_WIFI.wifi_passphrase));

    WriteWIFIConfigToEEPROM();

    s = htmlHeader() + F("<p>WIFI settings saved, will reboot in a few seconds.</p>") + htmlFooter();
    sendHeaders();
    server.send(200, "text/html", s);

    for (int i = 0; i < 20; i++) {
      delay(250);
      yield();
    }
    ESP.restart();

  } else {
    s = htmlHeader() + F("<p>WIFI settings too long.</p>") + htmlFooter();
    sendHeaders();
    server.send(200, "text/html", s);
  }
}

void setupAccessPoint(void) {
  WiFi.mode(WIFI_STA);
  WiFi.disconnect();

  delay(100);
  int n = WiFi.scanNetworks();

  if (n == 0)
    networks = "no networks found";
  else
  {
    for (int i = 0; i < n; ++i)
    {
      if (WiFi.encryptionType(i) != ENC_TYPE_NONE) {
        // Only show encrypted networks
        networks += "<option>";
        networks += WiFi.SSID(i);
        networks += "</option>";
      }
      delay(10);
    }
  }

  WiFi.mode(WIFI_AP);
  WiFi.softAP(ssid);

  if (!MDNS.begin("diybms")) {
    Serial.println("Error setting up MDNS responder!");
    //This will force a reboot of the ESP module by hanging the loop
    while (1) {
      delay(1000);
    }
  }

  server.on("/", HTTP_GET, handleRoot);
  server.on("/save", HTTP_POST, handleSave);
  server.onNotFound(handleNotFound);

  server.begin();
  MDNS.addService("http", "tcp", 80);

  Serial.println("Soft AP ready");
  while (1) {
    server.handleClient();
  }
}

