#include <ESP8266WebServer.h>

/*
   ____  ____  _  _  ____  __  __  ___
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

#include <Wire.h>

#include <ESP8266WiFi.h>
#include <ESP8266mDNS.h>
#include <WiFiClient.h>
#include <EEPROM.h>

ESP8266WebServer server(80);

//If we send a cmdByte with BIT 6 set its a command byte which instructs the cell to do something (not for reading)
#define COMMAND_BIT 6

#define COMMAND_green_led_pattern   1
#define COMMAND_led_off   2
#define COMMAND_factory_default 3
#define COMMAND_set_slave_address 4
#define COMMAND_green_led_default 5
#define COMMAND_set_voltage_calibration 6
#define COMMAND_set_temperature_calibration 7
#define COMMAND_set_bypass_voltage 8

#define read_voltage 10
#define read_temperature 11
#define read_voltage_calibration 12
#define read_temperature_calibration 13
#define read_raw_voltage 14
#define read_error_counter 15
#define read_bypass_enabled_state 16
#define read_bypass_voltage_measurement 17

//Default i2c SLAVE address (used for auto provision of address)
#define DEFAULT_SLAVE_ADDR 21

//Configured cell modules use i2c addresses 24 to 48 (24S)
//See http://www.i2c-bus.org/addressing/
#define DEFAULT_SLAVE_ADDR_START_RANGE 24
#define DEFAULT_SLAVE_ADDR_END_RANGE DEFAULT_SLAVE_ADDR_START_RANGE + 24


#define LED_ON digitalWrite(D4, LOW)
#define LED_OFF digitalWrite(D4, HIGH)

os_timer_t myTimer;

uint8_t i2cstatus;

union {
  float val;
  uint8_t buffer[4];
} float_to_bytes;

union {
  uint16_t val;
  uint8_t buffer[2];
} uint16_t_to_bytes;


uint8_t  send_command(uint8_t cell_id, uint8_t cmd) {
  Wire.beginTransmission(cell_id); // transmit to device
  Wire.write(cmd);  //Command configure device address
  uint8_t ret = Wire.endTransmission();  // stop transmitting
  return ret;
}

uint8_t  send_command(uint8_t cell_id, uint8_t cmd, uint8_t byteValue) {
  Wire.beginTransmission(cell_id); // transmit to device
  Wire.write(cmd);  //Command configure device address
  Wire.write(byteValue);  //Value
  uint8_t ret = Wire.endTransmission();  // stop transmitting
  return ret;
}

uint8_t  send_command(uint8_t cell_id, uint8_t cmd, float floatValue) {
  float_to_bytes.val = floatValue;
  Wire.beginTransmission(cell_id); // transmit to device
  Wire.write(cmd);  //Command configure device address
  Wire.write(float_to_bytes.buffer[0]);
  Wire.write(float_to_bytes.buffer[1]);
  Wire.write(float_to_bytes.buffer[2]);
  Wire.write(float_to_bytes.buffer[3]);
  uint8_t ret = Wire.endTransmission();  // stop transmitting
  return ret;
}

uint8_t send_command(uint8_t cell_id, uint8_t cmd, uint16_t Value) {
  uint16_t_to_bytes.val = Value;
  Wire.beginTransmission(cell_id); // transmit to device
  Wire.write(cmd);  //Command configure device address
  Wire.write(uint16_t_to_bytes.buffer[0]);
  Wire.write(uint16_t_to_bytes.buffer[1]);
  uint8_t ret = Wire.endTransmission();  // stop transmitting
  return ret;
}

uint8_t cmdByte(uint8_t cmd) {
  bitSet(cmd, COMMAND_BIT);
  return cmd;
}


uint16_t read_uint16_from_cell(uint8_t cell_id, uint8_t cmd) {
  send_command(cell_id, cmd);
  i2cstatus = Wire.requestFrom((uint8_t)cell_id, (uint8_t)2);
  return (word((uint8_t)Wire.read(), (uint8_t)Wire.read()));
}

uint8_t read_uint8_t_from_cell(uint8_t cell_id, uint8_t cmd) {
  send_command(cell_id, cmd);
  i2cstatus = Wire.requestFrom((uint8_t)cell_id, (uint8_t)1);
  return (uint8_t)Wire.read();
}

float read_float_from_cell(uint8_t cell_id, uint8_t cmd) {
  send_command(cell_id, cmd);
  i2cstatus = Wire.requestFrom((uint8_t)cell_id, (uint8_t)4);
  float_to_bytes.buffer[0] = (uint8_t)Wire.read();
  float_to_bytes.buffer[1] = (uint8_t)Wire.read();
  float_to_bytes.buffer[2] = (uint8_t)Wire.read();
  float_to_bytes.buffer[3] = (uint8_t)Wire.read();
  return float_to_bytes.val;
}

void clear_buffer() {
  while (Wire.available())  {
    Wire.read();
  }
}

uint8_t cell_green_led_default(uint8_t cell_id) {
  return send_command(cell_id, cmdByte( COMMAND_green_led_default ));
}

uint8_t cell_green_led_pattern(uint8_t cell_id) {
  return send_command(cell_id, cmdByte( COMMAND_green_led_pattern ), (uint8_t)B11001010);
}

uint8_t cell_led_off(uint8_t cell_id) {
  return send_command(cell_id, cmdByte( COMMAND_led_off ));
}

uint8_t command_factory_reset(uint8_t cell_id) {
  return send_command(cell_id, cmdByte( COMMAND_factory_default ));
}

uint8_t command_set_slave_address(uint8_t cell_id, uint8_t newAddress) {
  return send_command(cell_id, cmdByte( COMMAND_set_slave_address ), newAddress);
}

uint8_t command_set_voltage_calibration(uint8_t cell_id, float value) {
  return send_command(cell_id, cmdByte(COMMAND_set_voltage_calibration ), value);

}
uint8_t command_set_temperature_calibration(uint8_t cell_id, float value) {
  return send_command(cell_id, cmdByte(COMMAND_set_temperature_calibration ), value);

}

float cell_read_voltage_calibration(uint8_t cell_id) {
  return read_float_from_cell(cell_id, read_voltage_calibration);
}

float cell_read_temperature_calibration(uint8_t cell_id) {
  return read_float_from_cell(cell_id, read_temperature_calibration);
}

uint16_t cell_read_voltage(uint8_t cell_id) {
  return read_uint16_from_cell(cell_id, read_voltage);
}

uint16_t cell_read_bypass_enabled_state(uint8_t cell_id) {
  return read_uint8_t_from_cell(cell_id, read_bypass_enabled_state);
}

uint16_t cell_read_raw_voltage(uint8_t cell_id) {
  return read_uint16_from_cell(cell_id, read_raw_voltage);
}

uint16_t cell_read_error_counter(uint8_t cell_id) {
  return read_uint16_from_cell(cell_id, read_error_counter);
}

uint16_t cell_read_board_temp(uint8_t cell_id) {
  return read_uint16_from_cell(cell_id, read_temperature);
}

uint16_t cell_read_bypass_voltage_measurement(uint8_t cell_id) {
  return read_uint16_from_cell(cell_id, read_bypass_voltage_measurement);
}

uint8_t command_set_bypass_voltage(uint8_t cell_id, uint16_t  value) {
  return send_command(cell_id, cmdByte(COMMAND_set_bypass_voltage), value);
}


//Default values
struct cell_module {
  // 7 bit slave I2C address
  uint8_t address = DEFAULT_SLAVE_ADDR;

  uint16_t voltage;
  uint16_t temperature;

};

//Allow up to 20 modules
cell_module cell_array[20];
int cell_array_index = -1;
int cell_array_max = 0;

struct wifi_eeprom_settings {
  char wifi_ssid[32 + 1];
  char wifi_passphrase[63 + 1];
};

wifi_eeprom_settings myConfig_WIFI;

uint32_t calculateCRC32(const uint8_t *data, size_t length)
{
  uint32_t crc = 0xffffffff;
  while (length--) {
    uint8_t c = *data++;
    for (uint32_t i = 0x80; i > 0; i >>= 1) {
      bool bit = crc & 0x80000000;
      if (c & i) {
        bit = !bit;
      }
      crc <<= 1;
      if (bit) {
        crc ^= 0x04c11db7;
      }
    }
  }
  return crc;
}


struct eeprom_settings {
  int a;
  int b;
};

eeprom_settings myConfig;

//Where in EEPROM do we store the configuration
#define EEPROM_WIFI_CHECKSUM_ADDRESS 0
#define EEPROM_WIFI_CONFIG_ADDRESS EEPROM_WIFI_CHECKSUM_ADDRESS+sizeof(uint32_t)

#define EEPROM_CHECKSUM_ADDRESS EEPROM_WIFI_CONFIG_ADDRESS+sizeof(wifi_eeprom_settings)
#define EEPROM_CONFIG_ADDRESS EEPROM_CHECKSUM_ADDRESS+sizeof(uint32_t)


void WriteConfigToEEPROM() {
  EEPROM.put(EEPROM_CONFIG_ADDRESS, myConfig);
  EEPROM.put(EEPROM_CHECKSUM_ADDRESS, calculateCRC32((uint8_t*)&myConfig, sizeof(eeprom_settings)));
}

bool LoadConfigFromEEPROM() {
  eeprom_settings restoredConfig;
  uint32_t existingChecksum;

  EEPROM.get(EEPROM_CONFIG_ADDRESS, restoredConfig);
  EEPROM.get(EEPROM_CHECKSUM_ADDRESS, existingChecksum);

  // Calculate the checksum of an entire buffer at once.
  uint32_t checksum = calculateCRC32((uint8_t*)&restoredConfig, sizeof(eeprom_settings));

  if (checksum == existingChecksum) {
    //Clone the config into our global variable and return all OK
    memcpy(&myConfig, &restoredConfig, sizeof(eeprom_settings));
    return true;
  }

  //Config is not configured or gone bad, return FALSE
  return false;
}

void WriteWIFIConfigToEEPROM() {
  EEPROM.put(EEPROM_WIFI_CONFIG_ADDRESS, myConfig_WIFI);
  EEPROM.put(EEPROM_WIFI_CHECKSUM_ADDRESS, calculateCRC32((uint8_t*)&myConfig_WIFI, sizeof(wifi_eeprom_settings)));
}

bool LoadWIFIConfigFromEEPROM() {
  wifi_eeprom_settings restoredConfig;
  uint32_t existingChecksum;

  EEPROM.get(EEPROM_WIFI_CONFIG_ADDRESS, restoredConfig);
  EEPROM.get(EEPROM_WIFI_CHECKSUM_ADDRESS, existingChecksum);

  // Calculate the checksum of an entire buffer at once.
  uint32_t checksum = calculateCRC32((uint8_t*)&restoredConfig, sizeof(wifi_eeprom_settings));

  if (checksum == existingChecksum) {
    //Clone the config into our global variable and return all OK
    memcpy(&myConfig_WIFI, &restoredConfig, sizeof(wifi_eeprom_settings));
    return true;
  }

  //Config is not configured or gone bad, return FALSE
  return false;
}

const char* ssid = "DIY_BMS_CONTROLLER";

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
  return String("<!DOCTYPE HTML>\r\n<html><head><style>.page {width:300px;margin:0 auto 0 auto;background-color:cornsilk;font-family:sans-serif;padding:22px;} label {min-width:120px;display:inline-block;padding: 22px 0 22px 0;}</style></head><body><div class=\"page\"><h1>DIY BMS</h1>");
}

String htmlFooter() {
  return String("</div></body></html>\r\n\r\n");
}

void handleRoot()
{ 
  String s;
  s = htmlHeader()+"<h2>WiFi Setup</h2><p>Select local WIFI to connect to:</p>";
  s += "<form autocomplete=\"off\" method=\"post\" enctype=\"application/x-www-form-urlencoded\" action=\"\\save\">";
  s += "<label for=\"ssid\">SSID:</label><select id=\"ssid\" name=\"ssid\">";
  s += networks;
  s += "</select>";
  s += "<label for=\"pass\">Password:</label><input type=\"password\" id=\"id\" name=\"pass\"><br/>";
  s += "<input minlength=\"8\" maxlength=\"32\" type=\"submit\" value=\"Submit\"></form>";
  s += htmlFooter();

  sendHeaders();
  server.send(200, "text/html", s);
}

void handleSave() {

  String ssid = server.arg("ssid");
  String password = server.arg("pass");
  
  String s;
  s = htmlHeader()+"<p>WIFI settings saved, will reboot in a few seconds.</p><p>"+ssid+"</p><p>"+password+"</p>"+htmlFooter();  
  sendHeaders();
  server.send(200, "text/html", s);

  //ESP.restart()
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
  Serial.println("mDNS responder started");

  server.on("/", HTTP_GET, handleRoot);
  server.on("/save", HTTP_POST, handleSave);
  server.onNotFound(handleNotFound);

  server.begin();
  MDNS.addService("http", "tcp", 80);

  while (1) {
    server.handleClient();
  }
}


void setup() {
  Serial.begin(19200);           // start serial for output

  //D4 is LED
  pinMode(D4, OUTPUT);
  LED_OFF;

  Wire.setTimeout(1000);  //1000ms timeout
  Wire.setClock(100000);  //100khz
  Wire.setClockStretchLimit(1000);

  // join i2c bus
  // DATA=GPIO4/D2, CLOCK=GPIO5/D1
  Wire.begin(4, 5); //SDA/SCL

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

