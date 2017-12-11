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


#define LED_ON digitalWrite(D4, LOW)
#define LED_OFF digitalWrite(D4, HIGH)


#include <ESP8266WiFi.h>
#include <WiFiClient.h>

#include "i2c_cmds.h"
#include "settings.h"
#include "softap.h"


os_timer_t myTimer;

//Default values
struct cell_module {
  // 7 bit slave I2C address
  uint8_t address = DEFAULT_SLAVE_ADDR;
  uint16_t voltage;
  uint16_t temperature;
};

//Allow up to 24 modules
cell_module cell_array[24];
int cell_array_index = -1;
int cell_array_max = 0;




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

  yield();
  delay(250);
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

