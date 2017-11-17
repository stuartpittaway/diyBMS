#include <Wire.h>

//If we send a cmdByte with BIT 5 set its a command byte which instructs the cell to do something (not for reading)
#define COMMAND_BIT 5  

#define command_green_led_on   1
#define command_green_led_off   2
#define command_red_led_on   3
#define command_red_led_off   4

#define read_voltage 10
#define read_temperature 11

byte x = 0;


uint8_t  cell_configure(uint8_t new_cell_id) {
  Wire.beginTransmission(0x4); // transmit to device #4   
  Wire.write((uint8_t)0x20);  //Command configure device address
  Wire.write((uint8_t)new_cell_id);
  return Wire.endTransmission();    // stop transmitting
}


uint8_t  send_single_command(uint8_t cell_id, uint8_t cmd) {
  Wire.beginTransmission(cell_id); // transmit to device  
  Wire.write(cmd);  //Command configure device address
  return Wire.endTransmission();    // stop transmitting 
}


uint8_t cmdByte(uint8_t cmd) {
  bitSet(cmd, COMMAND_BIT);
  return cmd;
}


uint8_t  cell_green_led_on(uint8_t cell_id) {
  return send_single_command(cell_id,cmdByte( command_green_led_on ));
}

uint8_t  cell_green_led_off(uint8_t cell_id) {
  return send_single_command(cell_id,cmdByte( command_green_led_off ));
}

uint8_t  cell_red_led_on(uint8_t cell_id) {
  return send_single_command(cell_id,cmdByte( command_red_led_on ));
}
uint8_t  cell_red_led_off(uint8_t cell_id) {
  return send_single_command(cell_id,cmdByte( command_red_led_off ));
}


uint16_t read_uint16_from_cell(uint8_t cell_id) {
  
  //Read the 2 byte data from slave
  uint8_t status=Wire.requestFrom(cell_id,sizeof(uint16_t));
  
  //TODO: Need a timeout here and to check status
  while (Wire.available()!=sizeof(uint16_t))
  {
  }

  uint8_t left = (uint8_t)Wire.read();
  uint8_t right = (uint8_t)Wire.read();
  
  clear_buffer();
  return word(left,right); 
}

void clear_buffer() {
  while (Wire.available())  { Wire.read();  }
}

/*
uint32_t read_uint32_from_cell(uint8_t cell_id) {
  
  //Read the 4 byte data from slave
  uint8_t status=Wire.requestFrom((uint8_t)cell_id,(uint8_t)4);
  
  //TODO: Need a timeout here and to check status
  while (Wire.available()!=4)
  {
  }

uint8_t data[4];
data[0]=(uint8_t)Wire.read();
data[1]=(uint8_t)Wire.read();
data[2]=(uint8_t)Wire.read();
data[3]=(uint8_t)Wire.read();

clear_buffer();
return *(unsigned long*)(&data);
}
*/


uint16_t cell_read_voltage(uint8_t cell_id) {
  //Tell slave we are going to request the voltage
  byte status=send_single_command(cell_id, read_voltage);
  if (status>0) return 0xFFFF;

  return read_uint16_from_cell(cell_id);
}

uint16_t cell_read_board_temp(uint8_t cell_id) {
  //Tell slave we are going to request the temperature
  byte status=send_single_command(cell_id,read_temperature);
  if (status>0) return 0xFFFF;

  return read_uint16_from_cell(cell_id);
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
        *next++ = Wire.receive();
        ++bytes_received;
    }

    return bytes_received;
}
*/

void setup() {
  Serial.begin(19200);           // start serial for output

  Wire.setTimeout(1000);
  Wire.setClock(100000);  //100khz

  Wire.begin(1); // join i2c bus (address optional for master)

  //cell_read_voltage(0x70);
}

void loop() {  

  uint8_t cell_id=0x38;
  uint16_t data16;
  uint32_t data32;

  //cell_configure(cell_id);
  
  cell_green_led_on(cell_id); 

  data16=cell_read_voltage(cell_id);
  Serial.print("15 Volt= ");
  Serial.print(data16,HEX);
  Serial.print('=');
  Serial.println(data16);

  
  //cell_red_led_on(cell_id); 
  data16=cell_read_board_temp(cell_id);
  Serial.print("16 Temp= ");
  Serial.print(data16,HEX);
  Serial.print('=');
  Serial.println(data16);
  //cell_red_led_off(cell_id); 

  cell_green_led_off(cell_id);
  //Serial.println(status);

  delay(1000);
}
