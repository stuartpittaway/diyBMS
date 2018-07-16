#ifndef bms_values_H_
#define bms_values_H_

#include "Arduino.h"

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
#define COMMAND_set_load_resistance 9


#define read_voltage 10
#define read_temperature 11
#define read_voltage_calibration 12
#define read_temperature_calibration 13
#define read_raw_voltage 14
#define read_error_counter 15
#define read_bypass_enabled_state 16
#define read_bypass_voltage_measurement 17
#define read_load_resistance 18

//Default values
struct cell_module {
  // 7 bit slave I2C address
  uint8_t address;
  uint16_t voltage;
//  uint16_t temperature;
  float temperature;

  uint16_t balance_target;
  
  float voltage_calib;
  float temperature_calib;
  float loadResistance;

  bool factoryReset;

  //Record min/max volts over time (between cpu resets)
  uint16_t min_voltage;
  uint16_t max_voltage;

  bool valid_values;

  bool update_calibration;

  bool bypass_status;
};




#endif

