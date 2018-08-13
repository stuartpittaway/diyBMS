/* ____  ____  _  _  ____  __  __  ___
  (  _ \(_  _)( \/ )(  _ \(  \/  )/ __)
   )(_) )_)(_  \  /  ) _ < )    ( \__ \
  (____/(____) (__) (____/(_/\/\_)(___/

  (c) 2017 Stuart Pittaway
  Modifications for InfluxDB and additions made by Colin Hickey
  
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
#include <ESP8266HTTPClient.h>


#include "bms_values.h"
#include "i2c_cmds.h"
#include "settings.h"
#include "softap.h"
#include "WebServiceSubmit.h"
#include <Wire.h>

const int sensorIn = A0;
int mVperAmp = 66; // use 100 for 20A Module and 66 for 30A Module
int ACS712shift = 22;
bool InverterMon = false;

double Voltage = 0;
double VRMS = 0;
double AmpsRMS = 0;


//Allow up to 24 modules
cell_module cell_array[24];
int cell_array_index = -1;
int cell_array_max = 0;
unsigned long next_submit;
bool runProvisioning;

extern bool manual_balance;
bool max_enabled = false;
// 0=No balancing 1=Manual balancing started 2=Auto Balancing enabled 3=Auto Balancing enabled and bypass happening 4=A module is over max voltage
int balance_status = 0;
  
//Configuration for thermistor conversion
//use the datasheet to get this data.
//https://www.instructables.com/id/NTC-Temperature-Sensor-With-Arduino/
float Vin=3.3;     // [V]        
float Rt=10000;    // Resistor t [ohm]
float R0=20000;    // value of rct in T0 [ohm]
float T1=273.15;   // [K] in datasheet 0º C
float T2=373.15;   // [K] in datasheet 100° C
float RT1=35563;   // [ohms]  resistence in T1
float RT2=549.4;     // [ohms]   resistence in T2
float beta=0.0;    // initial parameters [K]
float Rinf=0.0;    // initial parameters [ohm]   
float T0=298.15;   // use T0 in Kelvin [K]
float Vout=0.0;    // Vout in A0 
float Rout=0.0;    // Rout in A0
float TempK=0.0;   // variable output
float TempC=0.0;   // variable output

EmonCMS emoncms;
Influxdb influxdb;

os_timer_t myTimer;

float getVPP() {
  float result;
  
  int readValue;             //value read from the sensor
  int maxValue = 0;          // store max value here
  int minValue = 1024;          // store min value here

   uint32_t start_time = millis();
   while((millis()-start_time) < 1000) //sample for 1 Sec
   {
       readValue = analogRead(sensorIn-ACS712shift);

       // see if you have a new maxValue
       if (readValue > maxValue) 
       {
           /*record the maximum sensor value*/
           maxValue = readValue;
       }
       if (readValue < minValue) 
       {
           /*record the maximum sensor value*/
           minValue = readValue;
       }
   }
   //Serial.println(readValue);
   // Subtract min from max
   result = ((maxValue - minValue) * 5.0)/1024.0;
      
   return result;
 }

void avg_balance() {
  uint16_t avgint = 0;
  float avgintf = 0.0;
       
  if ((myConfig.autobalance_enabled == true) && (manual_balance == false)) {
    
    if (cell_array_max > 0) {
    //Work out the average 
    float avg = 0;
    for (int a = 0; a < cell_array_max; a++) {
      avgintf = cell_array[a].voltage/1000.0;
      avg += 1.0 * avgintf;
    }
    avg = avg / cell_array_max;

    avgint = avg;

    balance_status = 2;
    
    //Serial.println("Average cell voltage is currently : " + String(avg*1000));
    //Serial.println("Configured balance voltage : " + String(myConfig.balance_voltage*1000));

    if ( avg >= myConfig.balance_voltage )  {
      for ( int a = 0; a < cell_array_max; a++) {
        if (cell_array[a].voltage > avg*1000) {
          cell_array[a].balance_target = avg*1000;
          balance_status = 3;
        }
      } 
    }  else {
      for (int a = 0; a < cell_array_max; a++) {
        command_set_bypass_voltage(cell_array[a].address,0); }}
        } 
  } else  balance_status = 0;    
}

void print_module_details(struct  cell_module *module) {
  Serial.print("Mod: ");
  Serial.print(module->address);
  Serial.print(" V:");
  Serial.print(module->voltage);
  Serial.print(" VC:");
  Serial.print(module->voltage_calib);
  Serial.print(" T:");
  Serial.print(module->temperature);
  Serial.print(" TC:");
  Serial.print(module->temperature_calib);
  Serial.print(" R:");
  Serial.print(module->loadResistance);
  Serial.println("");
}

void check_module_quick(struct  cell_module *module) {
  module->voltage = cell_read_voltage(module->address);
  module->temperature = tempconvert(cell_read_board_temp(module->address));   
  module->bypass_status = cell_read_bypass_enabled_state(module->address);
 if (module->voltage >= 0 && module->voltage <= 5000) {
    if ( module->voltage > module->max_voltage || module->valid_values == false) {
      module->max_voltage = module->voltage;
    }
    if ( module->voltage < module->min_voltage || module->valid_values == false) {
      module->min_voltage = module->voltage;
    }
    module->valid_values = true;
  } else {
    module->valid_values = false;
  }
}

void check_module_full(struct  cell_module *module) {
  check_module_quick(module);
  module->voltage_calib = cell_read_voltage_calibration(module->address);
  module->temperature_calib = cell_read_temperature_calibration(module->address);
  module->loadResistance = cell_read_load_resistance(module->address);
}

void timerCallback(void *pArg) {
  LED_ON;

  if (runProvisioning) {
    Serial.println("runProvisioning");
    uint8_t newCellI2CAddress = provision();

    if (newCellI2CAddress > 0) {
      Serial.print("Found ");
      Serial.println(newCellI2CAddress);

      cell_module m2;
      m2.address = newCellI2CAddress;
      cell_array[cell_array_max] = m2;
      cell_array[cell_array_max].min_voltage = 0xFFFF;
      cell_array[cell_array_max].max_voltage = 0;
      cell_array[cell_array_max].balance_target = 0;
      cell_array[cell_array_max].valid_values = false;

      //Dont attempt to read here as the module will be rebooting
      //check_module_quick( &cell_array[cell_array_max] );
      cell_array_max++;
    }

    runProvisioning = false;
    return;
  }


  //Ensure we have some cell modules to check
  if (cell_array_max > 0 && cell_array_index >= 0) {


    if (cell_array[cell_array_index].update_calibration) {

      if (cell_array[cell_array_index].factoryReset) {
        command_factory_reset(cell_array[cell_array_index].address);
      } else {

        //Check to see if we need to configure the calibration data for this module
        command_set_voltage_calibration(cell_array[cell_array_index].address, cell_array[cell_array_index].voltage_calib);
        command_set_temperature_calibration(cell_array[cell_array_index].address, cell_array[cell_array_index].temperature_calib);

        command_set_load_resistance(cell_array[cell_array_index].address, cell_array[cell_array_index].loadResistance);
      }
      cell_array[cell_array_index].update_calibration = false;
    }

    check_module_quick( &cell_array[cell_array_index] );

    if (cell_array[cell_array_index].balance_target > 0) {
      command_set_bypass_voltage(cell_array[cell_array_index].address, cell_array[cell_array_index].balance_target);
      cell_array[cell_array_index].balance_target = 0;
    }

    cell_array_index++;
    if (cell_array_index >= cell_array_max) {
      cell_array_index = 0;
    }
  }

  LED_OFF;
} // End of timerCallback

void scani2cBus() {

  cell_array_index = 0;

  //We have 1 module
  cell_array_max = 0;

  //Scan the i2c bus looking for modules on start up
  for (uint8_t address = DEFAULT_SLAVE_ADDR_START_RANGE; address <= DEFAULT_SLAVE_ADDR_END_RANGE; address++ )
  {
    if (testModuleExists(address) == true) {
      //We have found a module
      cell_module m1;
      m1.address = address;
      //Default values
      m1.valid_values = false;
      m1.min_voltage = 0xFFFF;
      m1.max_voltage = 0;
      cell_array[cell_array_max] = m1;

      check_module_full( &cell_array[cell_array_max] );

      //Switch off bypass if its on
      command_set_bypass_voltage(address, 0);

      print_module_details( &cell_array[cell_array_max] );

      cell_array_max++;
    }
  }
}

float tempconvert(float rawtemp) {
  Vout=Vin*((float)(rawtemp)/1024.0); // calc for ntc
  Rout=(Rt*Vout*(Vin-Vout));

  TempK=(beta/log(Rout/Rinf)); // calc for temperature
  TempC=TempK-273.15;

  return TempC;
}

void setup() {
  Serial.begin(19200);           // start serial for output
  Serial.println();
  Serial.println();
  Serial.println();
  Serial.println();
  Serial.println();

  //D4 is LED
  pinMode(D4, OUTPUT);
  LED_OFF;

  //Thermistor setup
  beta=(log(RT1/RT2))/((1/T1)-(1/T2));
  Rinf=R0*exp(-beta/T0);
  
  Serial.println(F("DIY BMS Controller Startup"));

  initWire();

  if (LoadConfigFromEEPROM()) {
    Serial.println(F("Settings loaded from EEPROM"));
  } else {
    Serial.println(F("We are in initial power on mode (factory reset)"));
    FactoryResetSettings();
  }

  if (LoadWIFIConfigFromEEPROM()) {
    Serial.println(F("Connect to WIFI AP"));
    /* Explicitly set the ESP8266 to be a WiFi-client, otherwise, it by default,
      would try to act as both a client and an access-point and could cause
      network-issues with your other WiFi-devices on your WiFi-network. */
    WiFi.mode(WIFI_STA);
    WiFi.begin(myConfig_WIFI.wifi_ssid, myConfig_WIFI.wifi_passphrase);
  } else {
    //We are in initial power on mode (factory reset)
    setupAccessPoint();
  }

  scani2cBus();

  //Ensure we service the cell modules every 0.5 seconds
  os_timer_setfn(&myTimer, timerCallback, NULL);
  os_timer_arm(&myTimer, 1000, true);

  //Check WIFI is working and connected
  Serial.print(F("WIFI Connecting"));

  //TODO: We need a timeout here in case the AP is dead!
  while (WiFi.status() != WL_CONNECTED)
  {
    delay(250);
    Serial.print( WiFi.status() );
  }
  Serial.print(F(". Connected IP:"));
  Serial.println(WiFi.localIP());

  SetupManagementRedirect();
}

void loop() {
  HandleWifiClient();
  yield();
  delay(250);
  HandleWifiClient();
  yield();
  delay(250);
  HandleWifiClient();
  yield();
  delay(250);
  HandleWifiClient();
  yield();
  delay(250);

  if (cell_array_max > 0) {  
       /* for ( int a = 0; a < cell_array_max; a++) {
          Serial.print(cell_array[a].address);
          Serial.print(':');
          Serial.print(cell_array[a].voltage);
          Serial.print(':');
          Serial.print(cell_array[a].temperature);
          Serial.print(':');
          Serial.print(cell_array[a].bypass_status);
          Serial.print(' ');
        }
        Serial.println();  */
    
    if ((millis() > next_submit) && (WiFi.status() == WL_CONNECTED)) {
        if (myConfig.invertermon_enabled == true ) {
          Voltage = getVPP();
          VRMS = (Voltage/2.0) *0.707; 
          AmpsRMS = (VRMS * 1000)/mVperAmp;
        }
      
      emoncms.postData(myConfig, cell_array, cell_array_max);
      influxdb.postData(myConfig, cell_array, cell_array_max);
 
      for (int a = 0; a < cell_array_max; a++) {
        //Serial.println(" Cell Voltage = " + String(cell_array[a].voltage));
        //Serial.println(" Max Cell Voltage = " + String(myConfig.max_voltage*1000));
        if (cell_array[a].voltage >= myConfig.max_voltage*1000) {
          cell_array[a].balance_target = myConfig.max_voltage*1000; 
           max_enabled = true;
           balance_status = 4;
        } else if (manual_balance!= true) cell_array[a].balance_target = 0;
      }
      if (max_enabled!=true) avg_balance(); 
      max_enabled = false;

      //Update Influxdb/emoncms every 20 seconds
      next_submit = millis() + 20000;
    }
  }
}//end of loop

