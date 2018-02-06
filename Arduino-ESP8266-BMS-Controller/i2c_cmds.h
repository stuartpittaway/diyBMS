#ifndef i2c_cmds_H_
#define i2c_cmds_H_

extern uint8_t DEFAULT_SLAVE_ADDR;
extern uint8_t DEFAULT_SLAVE_ADDR_START_RANGE;
extern uint8_t DEFAULT_SLAVE_ADDR_END_RANGE;

uint8_t send_command(uint8_t cell_id, uint8_t cmd);
uint8_t send_command(uint8_t cell_id, uint8_t cmd, uint8_t byteValue);
uint8_t send_command(uint8_t cell_id, uint8_t cmd, float floatValue);
uint8_t send_command(uint8_t cell_id, uint8_t cmd, uint16_t Value);

uint8_t cmdByte(uint8_t cmd);

uint16_t read_uint16_from_cell(uint8_t cell_id, uint8_t cmd);
uint8_t  read_uint8_t_from_cell(uint8_t cell_id, uint8_t cmd) ;
float    read_float_from_cell(uint8_t cell_id, uint8_t cmd) ;
void     clear_buffer();
uint8_t cell_green_led_default(uint8_t cell_id);
uint8_t cell_green_led_pattern(uint8_t cell_id);
uint8_t cell_led_off(uint8_t cell_id);
uint8_t command_factory_reset(uint8_t cell_id) ;
uint8_t command_set_slave_address(uint8_t cell_id, uint8_t newAddress);
uint8_t command_set_voltage_calibration(uint8_t cell_id, float value);
uint8_t command_set_temperature_calibration(uint8_t cell_id, float value);
uint8_t command_set_bypass_voltage(uint8_t cell_id, uint16_t  value);
uint8_t command_set_load_resistance(uint8_t cell_id, float value);

float cell_read_voltage_calibration(uint8_t cell_id);
float cell_read_temperature_calibration(uint8_t cell_id);
float cell_read_load_resistance(uint8_t cell_id);

uint16_t cell_read_voltage(uint8_t cell_id) ;
uint16_t cell_read_bypass_enabled_state(uint8_t cell_id);
uint16_t cell_read_raw_voltage(uint8_t cell_id);
uint16_t cell_read_error_counter(uint8_t cell_id);
uint16_t cell_read_board_temp(uint8_t cell_id) ;
uint16_t cell_read_bypass_voltage_measurement(uint8_t cell_id) ;


void initWire();
uint8_t provision();
bool testModuleExists(uint8_t address);

#endif

