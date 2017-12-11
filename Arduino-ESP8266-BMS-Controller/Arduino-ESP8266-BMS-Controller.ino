/* ____  ____  _  _  ____  __  __  ___
  (  _ \(_  _)( \/ )(  _ \(  \/  )/ __)
   )(_) )_)(_  \  /  ) _ < )    ( \__ \
  (____/(____) (__) (____/(_/\/\_)(___/

  (c) 2017 Stuart Pittaway

  This is the code for the controller - it talks to the cell modules over isolated i2c bus

  This code runs on ESP-8266-12E (NODE MCU 1.0) and compiles with Arduino 1.8.5 environment

  Arduino settings
  NodeMCU 1.0 (ESP-8266-12E module), Flash 4M (3MSPIFF), CPU 80MHZ

  Setting up ESP-8266-12E (NODE MCU 1.0) on Arduino
  http://www.instructables.com/id/Programming-a-HTTP-Server-on-ESP-8266-12E/

  c:\Program Files (x86)\PuTTY\putty.exe -serial COM6 -sercfg 115200,8,n,1,N

  i2c CLOCK STRETCH
  https://github.com/esp8266/Arduino/issues/698
*/

extern "C"
{
#include "user_interface.h"
}

#include <EEPROM.h>
#include <CRC32.h>
#include <ESP8266WiFi.h>
#include <ESP8266mDNS.h>
#include <WiFiClient.h>
#include <ESP8266WebServer.h>

#include "i2c_cmds.h"

ESP8266WebServer server(80);

//Default i2c SLAVE address (used for auto provision of address)
#define DEFAULT_SLAVE_ADDR 21

//Configured cell modules use i2c addresses 24 to 48 (24S)
//See http://www.i2c-bus.org/addressing/
#define DEFAULT_SLAVE_ADDR_START_RANGE 24
#define DEFAULT_SLAVE_ADDR_END_RANGE DEFAULT_SLAVE_ADDR_START_RANGE + 24

const char* ssid = "DIY_BMS_CONTROLLER";

#define LED_ON digitalWrite(D4, LOW)
#define LED_OFF digitalWrite(D4, HIGH)

os_timer_t myTimer;

//Default values
struct cell_module {
  // 7 bit slave I2C address
  uint8_t address = DEFAULT_SLAVE_ADDR;
  uint16_t voltage;
  uint16_t temperature;
};

//Allow up to 20 modules
cell_module cell_array[DEFAULT_SLAVE_ADDR_END_RANGE - DEFAULT_SLAVE_ADDR_START_RANGE];
int cell_array_index = -1;
int cell_array_max = 0;

struct wifi_eeprom_settings {
  char wifi_ssid[32 + 1];
  char wifi_passphrase[63 + 1];
};

wifi_eeprom_settings myConfig_WIFI;

struct eeprom_settings {
  uint8_t address_list[DEFAULT_SLAVE_ADDR_END_RANGE - DEFAULT_SLAVE_ADDR_START_RANGE];
};

eeprom_settings myConfig;

//Where in EEPROM do we store the configuration
#define EEPROM_storageSize 1024
#define EEPROM_WIFI_CHECKSUM_ADDRESS 0
#define EEPROM_WIFI_CONFIG_ADDRESS EEPROM_WIFI_CHECKSUM_ADDRESS+sizeof(uint32_t)

#define EEPROM_CHECKSUM_ADDRESS 256
#define EEPROM_CONFIG_ADDRESS EEPROM_CHECKSUM_ADDRESS+sizeof(uint32_t)


void WriteConfigToEEPROM() {
  uint32_t checksum = CRC32::calculate(&myConfig, sizeof(eeprom_settings));
  EEPROM.begin(EEPROM_storageSize);
  EEPROM.put(EEPROM_CONFIG_ADDRESS, myConfig);
  EEPROM.put(EEPROM_CHECKSUM_ADDRESS, checksum);
  EEPROM.end();
}

bool LoadConfigFromEEPROM() {
  eeprom_settings restoredConfig;
  uint32_t existingChecksum;

  EEPROM.begin(EEPROM_storageSize);
  EEPROM.get(EEPROM_CONFIG_ADDRESS, restoredConfig);
  EEPROM.get(EEPROM_CHECKSUM_ADDRESS, existingChecksum);
  EEPROM.end();

  // Calculate the checksum of an entire buffer at once.
  uint32_t checksum = CRC32::calculate(&restoredConfig, sizeof(eeprom_settings));

  Serial.println(checksum);
  Serial.println(existingChecksum);

  if (checksum == existingChecksum) {
    //Clone the config into our global variable and return all OK
    memcpy(&myConfig, &restoredConfig, sizeof(eeprom_settings));
    return true;
  }

  //Config is not configured or gone bad, return FALSE
  return false;
}

void WriteWIFIConfigToEEPROM() {
  uint32_t checksum = CRC32::calculate(&myConfig_WIFI, sizeof(wifi_eeprom_settings));

  EEPROM.begin(EEPROM_storageSize);
  EEPROM.put(EEPROM_WIFI_CONFIG_ADDRESS, myConfig_WIFI);
  EEPROM.put(EEPROM_WIFI_CHECKSUM_ADDRESS, checksum);
  EEPROM.end();

  Serial.println(checksum);
}

bool LoadWIFIConfigFromEEPROM() {
  wifi_eeprom_settings restoredConfig;
  uint32_t existingChecksum;

  EEPROM.begin(EEPROM_storageSize);
  EEPROM.get(EEPROM_WIFI_CONFIG_ADDRESS, restoredConfig);
  EEPROM.get(EEPROM_WIFI_CHECKSUM_ADDRESS, existingChecksum);
  EEPROM.end();

  // Calculate the checksum of an entire buffer at once.
  uint32_t checksum = CRC32::calculate(&restoredConfig, sizeof(wifi_eeprom_settings));

  Serial.println(checksum);
  Serial.println(existingChecksum);

  if (checksum == existingChecksum) {
    //Clone the config into our global variable and return all OK
    memcpy(&myConfig_WIFI, &restoredConfig, sizeof(wifi_eeprom_settings));
    return true;
  }

  //Config is not configured or gone bad, return FALSE
  return false;
}


void handleNotFound()
{
  String message = "File Not Found\n\n";

  server.send(404, "text/plain", message);
}

String networks;

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


void setup() {
  Serial.begin(19200);           // start serial for output

  //D4 is LED
  pinMode(D4, OUTPUT);
  LED_OFF;


  Serial.println("DIY BMS Startup");

  initWire();

  if (LoadConfigFromEEPROM()) {
    Serial.println("Settings loaded from EEPROM");
  } else {
    //We are in initial power on mode (factory reset)
  }

  if (LoadWIFIConfigFromEEPROM()) {
    Serial.println("WIFI Settings loaded from EEPROM");
  } else {
    //We are in initial power on mode (factory reset)
    setupAccessPoint();
  }

  //while (!Serial);             // Leonardo: wait for serial monitor
  //Serial.println("\n\nDIYBMS Test Controller\n\n");

  cell_module m1;
  m1.address = DEFAULT_SLAVE_ADDR_START_RANGE;

  cell_array[0] = m1;
  cell_array_index = 0;
  //We have 1 module
  cell_array_max = 1;

  os_timer_setfn(&myTimer, timerCallback, NULL);
  os_timer_arm(&myTimer, 1000, true);
}


void loop() {
  yield();
  delay(250);
  yield();
  delay(250);
  yield();
  delay(250);

  if (cell_array_max > 0) {
    for ( int a = 0; a < cell_array_max; a++) {
      Serial.print(cell_array[a].address);
      Serial.print(':');
      Serial.print(cell_array[a].voltage);
      Serial.print(':');
      Serial.print(cell_array[a].temperature);
      Serial.print(' ');
    }

    Serial.println();
  }
}//end of loop


void check_module_quick(struct  cell_module *module) {
  module->voltage = cell_read_voltage(module->address);
  module->temperature = cell_read_board_temp(module->address);
}

void timerCallback(void *pArg) {
  LED_ON;

  //Ensure we have some cell modules to check
  if (cell_array_max > 0 && cell_array_index >= 0) {

    check_module_quick( &cell_array[cell_array_index] );

    cell_array_index++;
    if (cell_array_index >= cell_array_max) {
      cell_array_index = 0;
    }
  }

  LED_OFF;
} // End of timerCallback

