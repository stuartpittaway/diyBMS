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

extern "C"
{
  #include "user_interface.h"
}

#include <Wire.h>

//Inter-packet/request delay on i2c bus
#define delay_ms 20

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

uint8_t send_command(uint8_t cell_id, uint8_t cmd, uint16_t Value) {
  uint16_t_to_bytes.val = Value;
  Wire.beginTransmission(cell_id); // transmit to device
  Wire.write(cmd);  //Command configure device address
  Wire.write(uint16_t_to_bytes.buffer[0]);
  Wire.write(uint16_t_to_bytes.buffer[1]);
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
  uint8_t status = Wire.requestFrom((uint8_t)cell_id, (uint8_t)2);
  uint8_t buffer[4];
  buffer[0] = (uint8_t)Wire.read();
  buffer[1] = (uint8_t)Wire.read();
  delay(delay_ms);
  return word(buffer[0], buffer[1]);
}

uint8_t read_uint8_t_from_cell(uint8_t cell_id, uint8_t cmd) {
  send_command(cell_id, cmd);
  uint8_t status = Wire.requestFrom((uint8_t)cell_id, (uint8_t)1);
  uint8_t buffer = (uint8_t)Wire.read();
  delay(delay_ms);
  return buffer;
}


float read_float_from_cell(uint8_t cell_id, uint8_t cmd) {
  send_command(cell_id, cmd);
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

os_timer_t myTimer;

bool x=true;
void timerCallback(void *pArg) {
  if (x) {
  digitalWrite(D4, LOW);
  } else {digitalWrite(D4, HIGH); }

  x=!x;
  
} // End of timerCallback


void setup() {
  Serial.begin(19200);           // start serial for output

  Wire.setTimeout(1000);  //1000ms timeout
  Wire.setClock(100000);  //100khz
  Wire.setClockStretchLimit(1000);

  // join i2c bus
  // DATA=GPIO4/D2, CLOCK=GPIO5/D1
  Wire.begin(4, 5); //SDA/SCL

  //D4 is LED
  pinMode(D4, OUTPUT);
  digitalWrite(D4, HIGH); //OFF

  while (!Serial);             // Leonardo: wait for serial monitor
  Serial.println("\n\nDIYBMS Test Controller\n\n");

  Serial.println("Commands: 'S<ADDR>' select module i2c address, 'F' factory reset selected module");
  Serial.println("Commands: 'A<ADDR>' set module i2c address, 'D' Go dark (light out)");
  Serial.println("Commands: 'B<VOLT>' set bypass voltage to VOLT, 'Y' query bypass enabled status");
  Serial.println("Commands: 'Q' Raw ADC voltage reading, 'L' Set default LED pattern");
  Serial.println("Commands: 'M' Read bypass voltage measurement");
  Serial.println("Commands: 'X' Scan for modules, 'P' Provision new module (one at a time)");
  Serial.println("Commands: 'V<MULT>' voltage multiplier (float), 'T<MULT>' Temperature multiplier (float)");
  Serial.println("Commands: 'R' Take voltage and temp readings");

  os_timer_setfn(&myTimer, timerCallback, NULL);
  os_timer_arm(&myTimer, 1000, true);
}

void print_status(uint8_t status) {
  Serial.print(' ');
  Serial.print(status);
  switch (status) {
    case 0: Serial.print(" * "); break;
    default: Serial.print(" ERR ");    break;
  }

}

uint8_t cell_id = DEFAULT_SLAVE_ADDR;

void loop() {
  uint8_t status;
  uint8_t v2;
  uint16_t data16;
  uint32_t data32;

  yield();
  float v1;

  if (Serial.available()) {
    char c = Serial.read();  //gets one byte from serial buffer

    //Upper case
    switch (c) {
      case 'X':
        Serial.print("i2c SCAN: ");
        for (uint8_t address = DEFAULT_SLAVE_ADDR; address <= DEFAULT_SLAVE_ADDR_END_RANGE; address++ )
        {
          Wire.beginTransmission(address);
          byte error = Wire.endTransmission();
          if (error == 0)
          {
            Serial.print(address);
            Serial.print(' ');
          }
        }
        Serial.println(".");
        break;

      case 'P': {
          Serial.print("i2c Provision: ");
          Wire.beginTransmission(DEFAULT_SLAVE_ADDR);
          byte error2 = Wire.endTransmission();
          if (error2 == 0)
          {
            Serial.print("Found module");

            for (uint8_t address = DEFAULT_SLAVE_ADDR_START_RANGE; address <= DEFAULT_SLAVE_ADDR_END_RANGE; address++ )
            {
              Wire.beginTransmission(address);
              byte error1 = Wire.endTransmission();
              if (error1 != 0) {
                //We have found a gap
                command_set_slave_address(DEFAULT_SLAVE_ADDR, (uint8_t)address);
                Serial.print("new address=");
                Serial.println(address);
                cell_id = address;
                break;
              }
            }
          } else {
            Serial.println("No module");
          }
        }
        break;


      case 'S':
        Serial.print("Select module Id ");
        cell_id = Serial.parseInt();
        Serial.println(cell_id);
        //cell_id = DEFAULT_SLAVE_ADDR_START_RANGE;
        break;

      case 'F':
        Serial.print("Factory reset ");
        Serial.println(cell_id);
        command_factory_reset(cell_id);
        break;

      case 'V':
        Serial.print("Volt calib ");
        v1 = Serial.parseFloat();
        Serial.println(v1);
        command_set_voltage_calibration(cell_id, v1);
        break;

      case 'T':
        Serial.println("Temp calib");
        v1 = Serial.parseFloat();
        Serial.println(v1);
        command_set_temperature_calibration(cell_id, v1);
        break;

      case 'A':
        Serial.print("Set module address ");
        v2 = Serial.parseInt();
        Serial.println(v2);
        if (v2 >= DEFAULT_SLAVE_ADDR_START_RANGE && v2 <= DEFAULT_SLAVE_ADDR_END_RANGE) {
          command_set_slave_address(cell_id, (uint8_t)v2);
        }
        break;


      case 'L':
        Serial.println("Default green led pattern");
        cell_green_led_default(cell_id);
        break;

      case 'D':
        Serial.println("Go dark");
        cell_led_off(cell_id);
        break;

      case 'Q':
        Serial.println("Raw voltage ADC");
        data16 = cell_read_raw_voltage(cell_id);
        Serial.print("  V=");
        Serial.print(data16, HEX);
        Serial.print('=');
        Serial.println(data16);
        break;

      case 'E':
        Serial.println("read_error_counter=");
        data16 = cell_read_error_counter(cell_id);
        Serial.print(data16, HEX);
        Serial.print('=');
        Serial.println(data16);
        break;

      case 'B': {
          Serial.println("Set bypass voltage ");
          uint16_t v5 = Serial.parseInt();
          Serial.println(v5);
          command_set_bypass_voltage(cell_id, v5);
        }
        break;

      case 'Y':
        Serial.println("Bypass state ");
        Serial.println(cell_read_bypass_enabled_state(cell_id));
        break;

      case 'M':
        Serial.println("Bypass volt measurement ");
        Serial.println(cell_read_bypass_voltage_measurement(cell_id));
        break;

      case 'R':
        for (int i = 0; i < 5; i++) {
          Serial.print("Reading ");
          Serial.print(i);

          data16 = cell_read_voltage(cell_id);
          Serial.print("  V=");
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
          dtostrf(f, 5, 2, buffer);

          Serial.print(" VC=");
          Serial.print(buffer);

          f = cell_read_temperature_calibration(cell_id);
          dtostrf(f, 5, 2, buffer);
          Serial.print(" TC=");
          Serial.print(buffer);
          Serial.println("");
          yield();
          delay(500);
        }
        break;

      default:
        Serial.print('*');
        break;
    }
  }

  //digitalWrite(D4, HIGH);
  delay(150);
}//end of loop

