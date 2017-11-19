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

#define delay_ms 10

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
  uint8_t ret = Wire.endTransmission();  // stop transmitting
  delay(delay_ms);
  return ret;
}


uint8_t cmdByte(uint8_t cmd) {
  bitSet(cmd, COMMAND_BIT);
  return cmd;
}

uint8_t  cell_green_led_on(uint8_t cell_id) {
  return send_single_command(cell_id, cmdByte( command_green_led_on ));
}

uint8_t  cell_green_led_off(uint8_t cell_id) {
  return send_single_command(cell_id, cmdByte( command_green_led_off ));
}

uint8_t  cell_red_led_on(uint8_t cell_id) {
  return send_single_command(cell_id, cmdByte( command_red_led_on ));
}

uint8_t  cell_red_led_off(uint8_t cell_id) {
  return send_single_command(cell_id, cmdByte( command_red_led_off ));
}

uint16_t read_uint16_from_cell(uint8_t cell_id) {
  //Read the 2 byte data from slave
  //sizeof(uint16_t)
  uint8_t status = Wire.requestFrom((uint8_t)cell_id, (uint8_t)2,true);

  uint8_t attempts = 1000;


  //TODO: Need a timeout here and to check status
  while (Wire.available() != 2 && attempts > 0)
  {
    yield();
    attempts--;
    delayMicroseconds(20);
  }

  if (attempts == 0) {
    clear_buffer();
    return 0xFFFF;
  }


  uint8_t left = (uint8_t)Wire.read();
  uint8_t right = (uint8_t)Wire.read();

  clear_buffer();

  //TODO: THIS SHOULDNT BE NEEDED!
  //Wire.endTransmission(); 
  return word(left, right);
}

void clear_buffer() {
  while (Wire.available())  {
    Wire.read();
  }
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
  byte status = send_single_command(cell_id, read_voltage);
  if (status > 0) return 0xFFFF;

  return read_uint16_from_cell(cell_id);
}

uint16_t cell_read_board_temp(uint8_t cell_id) {
  //Tell slave we are going to request the temperature
  byte status = send_single_command(cell_id, read_temperature);
  if (status > 0) return 0xFFFF;

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
         next++ = Wire.receive();
        ++bytes_received;
    }

    return bytes_received;
  }
*/

void setup() {
  Serial.begin(115200);           // start serial for output

  //while (!Serial);             // Leonardo: wait for serial monitor
  Serial.println("\n\nI2C Controller");

  Wire.setTimeout(1000);
  Wire.setClock(100000UL);  //100khz
  
  Wire.setClockStretchLimit(255);

  // join i2c bus
  // SDA=GPIO4/D2   SCL=GPIO5/D1
  Wire.begin(D2, D1); //SDA/SCL
}

void print_status(uint8_t status) {
  switch (status) {
    case 0: Serial.print(" * "); break;
    case 1: Serial.print(" LONG "); break;
    case 2: Serial.print(" ANACK "); break;
    case 3: Serial.print(" DNACK "); break;
    default: Serial.print(" ERR ");    break;
  }

}

void loop() {
  Serial.print("Loop... ");

  uint8_t status;
  uint8_t cell_id = 0x38;
  uint16_t data16;
  uint32_t data32;

  //cell_configure(cell_id);

  status = cell_green_led_on(cell_id);
  print_status(status);
/*
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

  status = cell_red_led_on(cell_id);
  print_status(status);
*/
  status = cell_green_led_off(cell_id);
  print_status(status);

  Serial.println();

  for (int i = 0; i < 10; i++) {
    yield();
    delay(100);
  }
}
