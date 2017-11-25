/*
   ____  ____  _  _  ____  __  __  ___
  (  _ \(_  _)( \/ )(  _ \(  \/  )/ __)
   )(_) )_)(_  \  /  ) _ < )    ( \__ \
  (____/(____) (__) (____/(_/\/\_)(___/

  (c) 2017 Stuart Pittaway

  This is the code for the controller - it talks to the cell modules over isolated i2c bus

  This code runs on ESP-8266-12E (NODE MCU 1.0) and compiles with Arduino 1.8.5 environment


  Arduino settings
  NodeMCU 1.0 (ESP-12module), Flash 4M (3MSPIFF), CPU 80MHZ

  Setting up ESP-8266-12E (NODE MCU 1.0) on Arduino
  http://www.instructables.com/id/Programming-a-HTTP-Server-on-ESP-8266-12E/


  c:\Program Files (x86)\PuTTY\putty.exe -serial COM6 -sercfg 115200,8,n,1,N

  i2c CLOCK STRETCH
  https://github.com/esp8266/Arduino/issues/698
*/

#include <Wire.h>

//Inter-packet/request delay on i2c bus
#define delay_ms 20

//If we send a cmdByte with BIT 5 set its a command byte which instructs the cell to do something (not for reading)
#define COMMAND_BIT 5

#define COMMAND_green_led_pattern   1
#define COMMAND_led_off   2
#define COMMAND_factory_default 3
#define COMMAND_set_slave_address 4
#define COMMAND_green_led_default 5
#define COMMAND_set_voltage_calibration 6
#define COMMAND_set_temperature_calibration 7

#define read_voltage 10
#define read_temperature 11
#define read_voltage_calibration 12
#define read_temperature_calibration 13

//Default i2c SLAVE address (used for auto provision of address)
#define DEFAULT_SLAVE_ADDR 0x15
#define DEFAULT_SLAVE_ADDR_START_RANGE 0x20
#define DEFAULT_SLAVE_ADDR_END_RANGE DEFAULT_SLAVE_ADDR_START_RANGE + 20

union {
  float val;
  uint8_t buffer[4];
} float_to_bytes;

uint8_t  send_command(uint8_t cell_id, uint8_t cmd) {
  Wire.beginTransmission(cell_id); // transmit to device
  Wire.write(cmd);  //Command configure device address
  uint8_t ret = Wire.endTransmission();  // stop transmitting
  delay(delay_ms);
  return ret;
}

uint8_t  send_command(uint8_t cell_id, uint8_t cmd, uint8_t byteValue) {
  Wire.beginTransmission(cell_id); // transmit to device
  Wire.write(cmd);  //Command configure device address
  Wire.write(byteValue);  //Value
  uint8_t ret = Wire.endTransmission();  // stop transmitting
  delay(delay_ms);
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
  delay(delay_ms);
  return ret;
}


uint8_t cmdByte(uint8_t cmd) {
  bitSet(cmd, COMMAND_BIT);
  return cmd;
}


uint16_t read_uint16_from_cell(uint8_t cell_id, uint8_t cmd) {
  send_command(cell_id, cmd);

  delay(2);
  uint8_t status = Wire.requestFrom((uint8_t)cell_id, (uint8_t)2);

  uint8_t buffer[4];
  buffer[0] = (uint8_t)Wire.read();
  buffer[1] = (uint8_t)Wire.read();

  delay(delay_ms);
  return word(buffer[0], buffer[1]);
}


float read_float_from_cell(uint8_t cell_id, uint8_t cmd) {
  send_command(cell_id, cmd);

  delay(2);
  uint8_t status = Wire.requestFrom((uint8_t)cell_id, (uint8_t)4);

  float_to_bytes.buffer[0] = (uint8_t)Wire.read();
  float_to_bytes.buffer[1] = (uint8_t)Wire.read();
  float_to_bytes.buffer[2] = (uint8_t)Wire.read();
  float_to_bytes.buffer[3] = (uint8_t)Wire.read();

  delay(delay_ms);
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
  return send_command(cell_id, cmdByte( COMMAND_set_voltage_calibration ), value);

}
uint8_t command_set_temperature_calibration(uint8_t cell_id, float value) {
  return send_command(cell_id, cmdByte( COMMAND_set_temperature_calibration ), value);

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

uint16_t cell_read_board_temp(uint8_t cell_id) {
  return read_uint16_from_cell(cell_id, read_temperature);
}

void setup() {
  Serial.begin(19200);           // start serial for output

  //while (!Serial);             // Leonardo: wait for serial monitor
  Serial.println("\n\nDIYBMS Controller");

  Wire.setTimeout(1000);  //1000ms timeout
  Wire.setClock(100000);  //100khz
  Wire.setClockStretchLimit(1000);

  // join i2c bus
  // DATA=GPIO4/D2, CLOCK=GPIO5/D1
  Wire.begin(4, 5); //SDA/SCL

  //D4 is LED
  pinMode(D4, OUTPUT);
  digitalWrite(D4, HIGH); //OFF
}

void print_status(uint8_t status) {
  Serial.print(' ');
  Serial.print(status);
  switch (status) {
    case 0: Serial.print(" * "); break;
    default: Serial.print(" ERR ");    break;
  }

}

void loop() {
  digitalWrite(D4, LOW);

  Serial.print("Loop... ");

  uint8_t status;
  //uint8_t cell_id = DEFAULT_SLAVE_ADDR;
  uint8_t cell_id = DEFAULT_SLAVE_ADDR_START_RANGE;
  uint16_t data16;
  uint32_t data32;

  data16 = cell_read_voltage(cell_id);
  Serial.print("V=");
  Serial.print(data16, HEX);
  Serial.print('=');
  Serial.print(data16);

  data16 = cell_read_board_temp(cell_id);
  Serial.print(" T=");
  Serial.print(data16, HEX);
  Serial.print('=');
  Serial.print(data16);

  float f = cell_read_voltage_calibration(cell_id);

  char buffer[16];  
  dtostrf(f,5,2,buffer);
      
  Serial.print(" VC=");
  Serial.print(buffer);

  f = cell_read_temperature_calibration(cell_id);
  dtostrf(f,5,2,buffer);
  Serial.print(" TC=");
  Serial.print(buffer);

  Serial.println("");

  digitalWrite(D4, HIGH);

  for (int i = 0; i < 10; i++) {
    //ESP8266 function
    yield();
    delay(250);
  }

  //Serial.println("FACTORY RESET:");
  //command_factory_reset(cell_id);

  //command_set_slave_address(cell_id,DEFAULT_SLAVE_ADDR_START_RANGE);
  command_set_voltage_calibration(cell_id, 1.635F);
  command_set_temperature_calibration(cell_id, 1.00F);
}


