#include <Wire.h>
#include "bms_values.h"
#include "i2c_cmds.h"
#include "Arduino.h"

//Default i2c SLAVE address (used for auto provision of address)
uint8_t DEFAULT_SLAVE_ADDR = 21;

//Configured cell modules use i2c addresses 24 to 48 (24S)
//See http://www.i2c-bus.org/addressing/
uint8_t DEFAULT_SLAVE_ADDR_START_RANGE = 24;
uint8_t DEFAULT_SLAVE_ADDR_END_RANGE = DEFAULT_SLAVE_ADDR_START_RANGE + 24;


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

uint8_t i2cstatus;

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
  return send_command(cell_id, cmdByte( COMMAND_green_led_pattern ), (uint8_t)202);
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
uint8_t command_set_load_resistance(uint8_t cell_id, float value) {
  return send_command(cell_id, cmdByte(COMMAND_set_load_resistance), value);
}


float cell_read_voltage_calibration(uint8_t cell_id) {
  return read_float_from_cell(cell_id, read_voltage_calibration);
}

float cell_read_temperature_calibration(uint8_t cell_id) {
  return read_float_from_cell(cell_id, read_temperature_calibration);
}

float cell_read_load_resistance(uint8_t cell_id) {
  return read_float_from_cell(cell_id, read_load_resistance);
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

void initWire() {
  Wire.setTimeout(1000);  //1000ms timeout
  Wire.setClock(100000);  //100khz
  Wire.setClockStretchLimit(1000);  

  // join i2c bus
  // DATA=GPIO4/D2, CLOCK=GPIO5/D1
  Wire.begin(4, 5); //SDA/SCL

  }

bool testModuleExists(uint8_t address) {
  Wire.beginTransmission(address);
  byte error2 = Wire.endTransmission();
  if (error2 == 0)
  {
      return true;
  }

  return false;
}

uint8_t provision() {

  if (testModuleExists(DEFAULT_SLAVE_ADDR))
  {
    for (uint8_t address = DEFAULT_SLAVE_ADDR_START_RANGE; address <= DEFAULT_SLAVE_ADDR_END_RANGE; address++ )
    {
      if (testModuleExists(address) == false) {
        //We have found a gap
        command_set_slave_address(DEFAULT_SLAVE_ADDR, (uint8_t)address);
        return address;
        break;
      }
    }
  } 

  return 0;
}

