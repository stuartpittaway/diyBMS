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
#define delay_ms 30

//If we send a cmdByte with BIT 5 set its a command byte which instructs the cell to do something (not for reading)
#define COMMAND_BIT 5

#define command_green_led_pattern   1
#define command_led_off   2
#define command_factory_default 3

#define read_voltage 10
#define read_temperature 11

/*
  uint8_t  cell_configure(uint8_t new_cell_id) {
  Wire.beginTransmission(0x4); // transmit to device #4
  Wire.write((uint8_t)0x20);  //Command configure device address
  Wire.write((uint8_t)new_cell_id);
  return Wire.endTransmission();    // stop transmitting
  }
*/

uint8_t  send_single_command(uint8_t cell_id, uint8_t cmd) {
  Wire.beginTransmission(cell_id); // transmit to device
  Wire.write(cmd);  //Command configure device address
  uint8_t ret = Wire.endTransmission();  // stop transmitting
  delay(delay_ms);
  return ret;
}

uint8_t  send_single_command(uint8_t cell_id, uint8_t cmd, uint8_t byteValue) {
  Wire.beginTransmission(cell_id); // transmit to device
  Wire.write(cmd);  //Command configure device address
  Wire.write(byteValue);  //Value
  uint8_t ret = Wire.endTransmission();  // stop transmitting
  delay(delay_ms);
  return ret;
}


uint8_t cmdByte(uint8_t cmd) {
  bitSet(cmd, COMMAND_BIT);
  return cmd;
}

uint8_t  cell_green_led_pattern(uint8_t cell_id) {
  return send_single_command(cell_id, cmdByte( command_green_led_pattern ),B11001010);
}

uint8_t  cell_led_off(uint8_t cell_id) {
  return send_single_command(cell_id, cmdByte( command_led_off ));
}

uint8_t  command_factory_reset(uint8_t cell_id) {
  return send_single_command(cell_id, cmdByte( command_factory_default ));
}


uint16_t read_uint16_from_cell(uint8_t cell_id, uint8_t cmd) {
  send_single_command(cell_id, cmd);

  delay(2);
  uint8_t status = Wire.requestFrom((uint8_t)cell_id, (uint8_t)2);

  uint8_t buffer[4];
  buffer[0] = (uint8_t)Wire.read();
  buffer[1] = (uint8_t)Wire.read();

  delay(delay_ms);
  return word(buffer[0], buffer[1]);
}


void clear_buffer() {
  while (Wire.available())  {
    Wire.read();
  }
}



/*
  uint32_t read_uint32_from_cell(uint8_t cell_id) {

  //Read the 4 byte data from slave
  uint8_t status = Wire.requestFrom((uint8_t)cell_id, (uint8_t)4);

  //TODO: Need a timeout here and to check status
  while (Wire.available() != 4)
  {
  }

  uint8_t data[4];
  data[0] = (uint8_t)Wire.read();
  data[1] = (uint8_t)Wire.read();
  data[2] = (uint8_t)Wire.read();
  data[3] = (uint8_t)Wire.read();

  clear_buffer();
  return *(unsigned long*)(&data);
  }
*/


uint16_t cell_read_voltage(uint8_t cell_id) {
  //Tell slave we are going to request the voltage
  //byte status = send_single_command(cell_id, read_voltage);
  //if (status > 0) return 0xFFFF;

  return read_uint16_from_cell(cell_id, read_voltage);
}

uint16_t cell_read_board_temp(uint8_t cell_id) {
  //Tell slave we are going to request the temperature
  //byte status = send_single_command(cell_id, read_temperature);
  //if (status > 0) return 0xFFFF;

  return read_uint16_from_cell(cell_id, read_temperature);
}

/*
  uint16_t M24LC256::ReadChunk(uint16_t location, uint16_t length, uint8_t* data)
  {
    uint16_t bytes_received = 0;
    uint16_t bytes_requested = min(length,16);

    Wire.requestFrom(i2c_address,bytes_requested);
    uint16_t remaining = bytes_requested;
    uint8_t* next = data;

    while (Wire.available() && remaining--)
    {
         next++ = Wire.receive();
        ++bytes_received;
    }

    return bytes_received;
  }
*/

void setup() {
  Serial.begin(19200);           // start serial for output

  //while (!Serial);             // Leonardo: wait for serial monitor
  Serial.println("\n\nI2C Controller");

  Wire.setTimeout(1000);  //1000ms timeout
  Wire.setClock(100000);  //100khz
  Wire.setClockStretchLimit(1000);

  // join i2c bus
  // DATA=GPIO4/D2, CLOCK=GPIO5/D1
  Wire.begin(4, 5); //SDA/SCL

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
  uint8_t cell_id = 0x15;
  uint16_t data16;
  uint32_t data32;

  //cell_configure(cell_id);

  //status = cell_green_led_pattern(cell_id);
  //print_status(status);

  data16 = cell_read_voltage(cell_id);
  Serial.print("V=");
  Serial.print(data16, HEX);
  Serial.print('=');
  Serial.print(data16);

  data16 = cell_read_board_temp(cell_id);
  Serial.print("T=");
  Serial.print(data16, HEX);
  Serial.print('=');
  Serial.print(data16);

  //status = cell_led_off(cell_id);
  //print_status(status);

  Serial.println("");

  digitalWrite(D4, HIGH);

  for (int i = 0; i < 10; i++) {
    //ESP8266 function
    yield();
    delay(250);
  }

  //Serial.println("FACTORY RESET:");
  //command_factory_reset(cell_id);

}
