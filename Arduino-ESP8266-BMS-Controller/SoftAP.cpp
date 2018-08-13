#include "Arduino.h"

#include "bms_values.h"
#include "softap.h"
#include "settings.h"
#include "bms_values.h"
#include "i2c_cmds.h"

#include <ESP8266mDNS.h>
#include <ESP8266WebServer.h>

ESP8266WebServer server(80);

const char* ssid = "DIY_BMS_CONTROLLER";

String networks;

bool manual_balance = false;
extern int balance_status;
extern bool InverterMon;

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


String htmlManagementHeader() {
  return String(F("<!DOCTYPE html><html><head><meta charset=\"utf-8\"><meta name=\"viewport\" content=\"width=device-width, initial-scale=1\"><title>DIY BMS Management Console</title><script type=\"text/javascript\" src=\"https://chickey.github.io/diyBMS/loader.js\"></script></head><body></body></html>\r\n\r\n"));
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

void handleRedirect() {
  sendHeaders();
  server.send(200, "text/html", htmlManagementHeader());
}

void handleProvision() {
  runProvisioning = true;
  server.send(200, "application/json", "[1]\r\n\r\n");
}


void handleResetESP() {
  Serial.println("Restarting Controller");

  ESP.restart();
  server.send(200, "application/json", "[1]\r\n\r\n");
}

void handleCancelAverageBalance() {
  if (cell_array_max > 0) {
    for (int a = 0; a < cell_array_max; a++) {
      command_set_bypass_voltage(cell_array[a].address,0);
    }
  }
  manual_balance = false;
  balance_status = 0;
  Serial.println("Cancelling balancing");
  server.send(200, "application/json", "[1]\r\n\r\n");
}


void handleAboveAverageBalance() {
  uint16_t avgint = 0;
  if (cell_array_max > 0) {
    //Work out the average
    float avg = 0;
    for (int a = 0; a < cell_array_max; a++) {
      avg += 1.0 * cell_array[a].voltage;
    }
    avg = avg / cell_array_max;

    avgint = avg;

    for ( int a = 0; a < cell_array_max; a++) {
      if (cell_array[a].voltage > avgint) {
        cell_array[a].balance_target = avgint;
      }
    }
  }

  manual_balance = true;
  balance_status = 1;
  server.send(200, "application/json", "[" + String(avgint) + "]\r\n\r\n");
}

void handleFactoryReset() {
  uint8_t module =  server.arg("module").toInt();
  float newValue = server.arg("value").toFloat();

  for ( int a = 0; a < cell_array_max; a++) {
    if (cell_array[a].address == module) {
      Serial.print("FactoryReset ");
      Serial.print(module);
      cell_array[a].factoryReset = true;
      cell_array[a].update_calibration = true;
      server.send(200, "text/plain", "");
      return;
    }
  }
  server.send(500, "text/plain", "");  
}

void handleSetLoadResistance() {
  uint8_t module =  server.arg("module").toInt();
  float newValue = server.arg("value").toFloat();

  Serial.print("SetLoadResistance ");
  Serial.print(module);
  Serial.print(" = ");
  Serial.println(newValue, 6);

  for ( int a = 0; a < cell_array_max; a++) {
    if (cell_array[a].address == module) {

      if (cell_array[a].loadResistance != newValue) {
        cell_array[a].loadResistance = newValue;
        cell_array[a].update_calibration = true;
      }
      server.send(200, "text/plain", "");
      return;
    }
  }
  server.send(500, "text/plain", "");
}

void handleSetInfluxDB() {

  myConfig.influxdb_enabled = (server.arg("influxdb_enabled").toInt() == 1) ? true : false;
  myConfig.influxdb_httpPort = server.arg("influxdb_httpPort").toInt();

  server.arg("influxdb_host").toCharArray(myConfig.influxdb_host, sizeof(myConfig.influxdb_host));
  server.arg("influxdb_database").toCharArray(myConfig.influxdb_database, sizeof(myConfig.influxdb_database));
  server.arg("influxdb_user").toCharArray(myConfig.influxdb_user, sizeof(myConfig.influxdb_user));
  server.arg("influxdb_password").toCharArray(myConfig.influxdb_password, sizeof(myConfig.influxdb_password));
  
  WriteConfigToEEPROM();

  server.send(200, "text/plain", "");
}

void handleSetEmonCMS() {

  myConfig.autobalance_enabled = (server.arg("autobalance_enabled").toInt() == 1) ? true : false;
  myConfig.invertermon_enabled = (server.arg("invertermon_enabled").toInt() == 1) ? true : false;
  myConfig.max_voltage = server.arg("max_voltage").toFloat();
  myConfig.balance_voltage = server.arg("balance_voltage").toFloat();
  myConfig.balance_dev = server.arg("balance_dev").toFloat();
      
  myConfig.influxdb_enabled = (server.arg("influxdb_enabled").toInt() == 1) ? true : false;
  myConfig.influxdb_httpPort = server.arg("influxdb_httpPort").toInt();

  server.arg("influxdb_host").toCharArray(myConfig.influxdb_host, sizeof(myConfig.influxdb_host));
  server.arg("influxdb_database").toCharArray(myConfig.influxdb_database, sizeof(myConfig.influxdb_database));
  server.arg("influxdb_user").toCharArray(myConfig.influxdb_user, sizeof(myConfig.influxdb_user));
  server.arg("influxdb_password").toCharArray(myConfig.influxdb_password, sizeof(myConfig.influxdb_password));

  myConfig.emoncms_enabled = (server.arg("emoncms_enabled").toInt() == 1) ? true : false;
  myConfig.emoncms_node_offset = server.arg("emoncms_node_offset").toInt();
  myConfig.emoncms_httpPort = server.arg("emoncms_httpPort").toInt();

  server.arg("emoncms_host").toCharArray(myConfig.emoncms_host, sizeof(myConfig.emoncms_host));
  server.arg("emoncms_url").toCharArray(myConfig.emoncms_url, sizeof(myConfig.emoncms_url));
  server.arg("emoncms_apikey").toCharArray(myConfig.emoncms_apikey, sizeof(myConfig.emoncms_apikey));

  WriteConfigToEEPROM();

  server.send(200, "text/plain", "");
}

void handleSetVoltCalib() {
  uint8_t module =  server.arg("module").toInt();
  float newValue = server.arg("value").toFloat();

  Serial.print("SetVoltCalib ");
  Serial.print(module);
  Serial.print(" = ");
  Serial.println(newValue, 6);

  for ( int a = 0; a < cell_array_max; a++) {
    if (cell_array[a].address == module) {

      if (cell_array[a].voltage_calib != newValue) {
        cell_array[a].voltage_calib = newValue;
        cell_array[a].update_calibration = true;
      }
      server.send(200, "text/plain", "");
      return;
    }
  }
  server.send(500, "text/plain", "");
}

void handleSetTempCalib() {
  uint8_t module =  server.arg("module").toInt();
  float newValue = server.arg("value").toFloat();

  Serial.print("SetTempCalib ");
  Serial.print(module);
  Serial.print(" = ");
  Serial.println(newValue, 6);

  for ( int a = 0; a < cell_array_max; a++) {
    if (cell_array[a].address == module) {
      if (cell_array[a].temperature_calib != newValue) {
        cell_array[a].temperature_calib = newValue;
        cell_array[a].update_calibration = true;
      }
      server.send(200, "text/plain", "");
      return;
    }
  }
  server.send(500, "text/plain", "");
}

void handleCellConfigurationJSON() {
  String json1 = "";
  
  if (cell_array_max > 0) {
    for ( int a = 0; a < cell_array_max; a++) {
      json1 += "{\"address\":" + String(cell_array[a].address)
        +",\"volt\":" + String(cell_array[a].voltage)
        +",\"voltc\":" + String(cell_array[a].voltage_calib, 6)
        +",\"temp\":" + String(cell_array[a].temperature)
        +",\"bypass\":" + String(cell_array[a].bypass_status)
        +",\"tempc\":" + String(cell_array[a].temperature_calib, 6)
        +",\"resistance\":" + String( isnan(cell_array[a].loadResistance) ? 0:cell_array[a].loadResistance, 6) 
        + "}";
      if (a < cell_array_max - 1) {
        json1 += ",";
      }
    }
  }
  server.send(200, "application/json", "[" + json1 + "]\r\n\r\n");
}

void handleSettingsJSON() {
  String json1 =   "{\"emoncms_enabled\":" + (myConfig.emoncms_enabled ? String("true") : String("false"))
                   + ",\"emoncms_node_offset\":" + String(myConfig.emoncms_node_offset)
                   + ",\"emoncms_httpPort\":" + String(myConfig.emoncms_httpPort)
                   + ",\"emoncms_host\":\"" + String(myConfig.emoncms_host) + "\""
                   + ",\"emoncms_apikey\":\"" + String(myConfig.emoncms_apikey) + "\""
                   + ",\"emoncms_url\":\"" + String(myConfig.emoncms_url) + "\""
                   + ",\"influxdb_enabled\":" + (myConfig.influxdb_enabled ? String("true") : String("false"))
                   + ",\"influxdb_host\":\"" + String(myConfig.influxdb_host) + "\""
                   + ",\"influxdb_httpPort\":" + String(myConfig.influxdb_httpPort)
                   + ",\"influxdb_database\":\"" + String(myConfig.influxdb_database) + "\""
                   + ",\"influxdb_user\":\"" + String(myConfig.influxdb_user) + "\""
                   + ",\"influxdb_password\":\"" + String(myConfig.influxdb_password) + "\""
                   + ",\"invertermon_enabled\":" + (myConfig.invertermon_enabled ? String("true") : String("false"))
                   + ",\"autobalance_enabled\":" + (myConfig.autobalance_enabled ? String("true") : String("false"))
                   + ",\"max_voltage\":\"" + String(myConfig.max_voltage) + "\""
                   + ",\"balance_voltage\":\"" + String(myConfig.balance_voltage) + "\""
                   + ",\"balance_dev\":\"" + String(myConfig.balance_dev) + "\""
                   + "}\r\n\r\n";
  server.send(200, "application/json", json1 );
}

void handleCellJSONData() {
  //Voltage
  String json1 = "[";
  //Temperature
  String json2 = "[";
  //Min voltage (since reset)
  String json3 = "[";
  //Max voltage (since reset)
  String json4 = "[";

  //Out of balance from average
  String json5 = "[";

  if (cell_array_max > 0) {

    //Work out the average
    float avg = 0;
    for (int a = 0; a < cell_array_max; a++) {
      avg += 1.0 * cell_array[a].voltage;
    }
    avg = avg / cell_array_max;

    for ( int a = 0; a < cell_array_max; a++) {
      json1 += String(cell_array[a].voltage);
      json2 += String(cell_array[a].temperature);
      json3 += String(cell_array[a].min_voltage);
      json4 += String(cell_array[a].max_voltage);

      json5 += String(cell_array[a].voltage - avg);

      if (a < cell_array_max - 1) {
        json1 += ",";
        json2 += ",";
        json3 += ",";
        json4 += ",";
        json5 += ",";
      }
    }
  }

  json1 += "]";
  json2 += "]";
  json3 += "]";
  json4 += "]";
  json5 += "]";


  server.send(200, "application/json", "[" + json1 + "," + json2 + "," + json3 + "," + json4 + "," + json5 + "]\r\n\r\n");
}

void handleSave() {
  String s;
  String ssid = server.arg("ssid");
  String password = server.arg("pass");

  if ((ssid.length() <= sizeof(myConfig_WIFI.wifi_ssid)) && (password.length() <= sizeof(myConfig_WIFI.wifi_passphrase))) {

    memset(&myConfig_WIFI, 0, sizeof(wifi_eeprom_settings));

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
    HandleWifiClient();
  }
}

void SetupManagementRedirect() {
  server.on("/", HTTP_GET, handleRedirect);
  server.on("/celljson", HTTP_GET, handleCellJSONData);
  server.on("/provision", HTTP_GET, handleProvision);
  server.on("/aboveavgbalance", HTTP_GET, handleAboveAverageBalance);
  server.on("/cancelavgbalance", HTTP_GET, handleCancelAverageBalance);
  server.on("/ResetESP", HTTP_GET, handleResetESP);
  server.on("/getmoduleconfig", HTTP_GET, handleCellConfigurationJSON);
  server.on("/getsettings", HTTP_GET, handleSettingsJSON);
  server.on("/factoryreset", HTTP_POST, handleFactoryReset);
  server.on("/setloadresistance", HTTP_POST, handleSetLoadResistance);
  server.on("/setvoltcalib", HTTP_POST, handleSetVoltCalib);
  server.on("/settempcalib", HTTP_POST, handleSetTempCalib);
  server.on("/setemoncms", HTTP_POST, handleSetEmonCMS);
  //server.on("/handleSetInfluxDB", HTTP_POST, handleSetInfluxDB);

  server.onNotFound(handleNotFound);

  server.begin();
  MDNS.addService("http", "tcp", 80);

  Serial.println("Management Redirect Ready");
}

void HandleWifiClient() {
  server.handleClient();
}

