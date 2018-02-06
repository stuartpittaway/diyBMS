/*
   ____  ____  _  _  ____  __  __  ___
  (  _ \(_  _)( \/ )(  _ \(  \/  )/ __)
   )(_) )_)(_  \  /  ) _ < )    ( \__ \
  (____/(____) (__) (____/(_/\/\_)(___/

  (c) 2017 Stuart Pittaway

  This is the code for the cell module (one is needed for each series cell in a modular battery array (pack))

  This code runs on ATTINY85 processors and compiles with Arduino 1.8.5 environment

  You will need a seperate programmer to program the ATTINY chips - another Arduino can be used

  Settings ATTINY85, 8MHZ INTERNAL CLOCK, LTO enabled, BOD disabled, Timer1=CPU


  Use this board manager for ATTINY85
  http://drazzy.com/package_drazzy.com_index.json
  not this one https://raw.githubusercontent.com/damellis/attiny/ide-1.6.x-boards-manager/package_damellis_attiny_index.json

  ATTINY85/V-10PU data sheet
  http://www.atmel.com/images/atmel-2586-avr-8-bit-microcontroller-attiny25-attiny45-attiny85_datasheet.pdf

  To manually configure ATTINY fuses use the command....
  .\bin\avrdude -p attiny85 -C etc\avrdude.conf -c avrisp -P COM5 -b 19200 -B2 -e -Uefuse:w:0xff:m -Uhfuse:w:0xdf:m -Ulfuse:w:0xe2:m
  If you burn incorrect fuses to ATTINY85 you may need to connect a crystal over the pins to make it work again!
*/


//Show error is not targetting ATTINY85
#if !(defined(__AVR_ATtiny85__))
#error Written for ATTINY85/V chip
#endif

#if !(F_CPU == 8000000)
#error Processor speed should be 8Mhz internal
#endif

#include <USIWire.h>
#include <EEPROM.h>
#include <avr/sleep.h>
#include <avr/wdt.h>

// __DATE__

//LED light patterns
#define GREEN_LED_PATTERN_STANDARD 0
#define GREEN_LED_PATTERN_BYPASS B01101100
#define GREEN_LED_PANIC B01010101
#define GREEN_LED_PATTERN_UNCONFIGURED B11101111

//Where in EEPROM do we store the configuration
#define EEPROM_CHECKSUM_ADDRESS 0
#define EEPROM_CONFIG_ADDRESS 20

#define MIN_BYPASS_VOLTAGE 3000U
#define MAX_BYPASS_VOLTAGE 4200U

//Number of TIMER1 cycles between voltage reading checks (240 = approx 30 seconds)
#define BYPASS_COUNTER_MAX 240

//Default i2c SLAVE address (used for auto provision of address)
#define DEFAULT_SLAVE_ADDR 21

//Configured cell modules use i2c addresses 24 to 48 (24S)
#define DEFAULT_SLAVE_ADDR_START_RANGE 24
#define DEFAULT_SLAVE_ADDR_END_RANGE DEFAULT_SLAVE_ADDR_START_RANGE + 24

//Number of times we sample and average the ADC for voltage
#define OVERSAMPLE_LOOP 16

//If we receive a cmdByte with BIT 6 set its a command byte so there is another byte waiting for us to process
#define COMMAND_BIT 6

#define COMMAND_green_led_pattern   1
//unused #define COMMAND_led_off   2
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

volatile bool skipNextADC = false;
volatile uint8_t green_pattern = GREEN_LED_PATTERN_STANDARD;
volatile uint16_t analogVal[OVERSAMPLE_LOOP];
volatile uint16_t temperature_probe = 0;
volatile uint8_t analogValIndex;
volatile uint8_t buffer_ready = 0;
volatile uint8_t reading_count = 0;
volatile uint8_t cmdByte = 0;

volatile uint8_t last_i2c_request = 255;
volatile uint16_t VCCMillivolts = 0;
volatile uint16_t ByPassVCCMillivolts = 0;
volatile uint16_t last_raw_adc = 0;
volatile uint16_t targetByPassVoltage = 0;
volatile uint8_t ByPassCounter = 0;
volatile boolean ByPassEnabled = false;
volatile boolean flash_green = false;

uint16_t error_counter = 0;

bool flashLed;

//Number of voltage readings to take before we take a temperature reading
#define TEMP_READING_LOOP_FREQ 16

//Default values
struct cell_module_config {
  // 7 bit slave I2C address
  uint8_t SLAVE_ADDR = DEFAULT_SLAVE_ADDR;
  // Calibration factor for voltage readings
  float VCCCalibration = 4.430;
  // Calibration factor for temp readings
  float TemperatureCalibration = 1.080;
  // Resistance of bypass load
  float LoadResistance = 10.0;
};

static cell_module_config myConfig;

void setup() {
  //Must be first line of setup()
  MCUSR &= ~(1 << WDRF); // reset status flag
  wdt_disable();

  //Pins PB0 (SDA) and PB2 (SCLOCK) are for the i2c comms with master
  //PINS https://github.com/SpenceKonde/ATTinyCore/blob/master/avr/extras/ATtiny_x5.md

  //pinMode(A3, INPUT);  //A3 pin 3 (PB3)
  //pinMode(A0, INPUT);  //reset pin A0 pin 5 (PB5)

  /*
    DDRB &= ~(1 << DDB3);
    PORTB &= ~(1 << PB3);

    DDRB &= ~(1 << DDB5);
    PORTB &= ~(1 << PB5);
  */

  pinMode(PB4, OUTPUT); //PB4 = PIN 3
  digitalWrite(PB4, LOW);

  ledGreen();
  delay(500);
  ledOff();

  //Load our EEPROM configuration
  if (!LoadConfigFromEEPROM()) {
    //Do something here for bad configuration??
  }

  cli();//stop interrupts

  analogValIndex = 0;

  initTimer1();
  initADC();

  // WDTCSR configuration:     WDIE = 1: Interrupt Enable     WDE = 1 :Reset Enable
  // Enter Watchdog Configuration mode:
  WDTCR |= (1 << WDCE) | (1 << WDE);

  // Set Watchdog settings - 4000ms timeout
  WDTCR = (1 << WDIE) | (1 << WDE) | (1 << WDP3) | (0 << WDP2) | (0 << WDP1) | (0 << WDP0);

  // Enable Global Interrupts
  sei();

  LEDReset();
  wait_for_buffer_ready();
  LEDReset();

  init_i2c();
}

boolean inPanicMode = false;

void panic() {
  green_pattern = GREEN_LED_PANIC;
  inPanicMode = true;
}

uint8_t previousLedState = 0;
void loop() {
  wdt_reset();

  if (last_i2c_request > 0) {
    //Count down loop for requests to see if i2c bus hangs or controller stops talking
    last_i2c_request--;
  }

  //If we are on the default SLAVE address then use different LED pattern to indicate this
  if (myConfig.SLAVE_ADDR == DEFAULT_SLAVE_ADDR) {
    green_pattern = GREEN_LED_PATTERN_UNCONFIGURED;
  } else {
    //We have had at least one i2c request and not currently in PANIC mode
    if (last_i2c_request == 0 && inPanicMode == false) {

      previousLedState = green_pattern ;

      //Panic after a few seconds of not receiving i2c requests...
      panic();

      //Try resetting the i2c bus
      Wire.end();
      init_i2c();

      error_counter++;
    }

    if (last_i2c_request > 0 && inPanicMode == true) {
      //Just come back from a PANIC situation
      LEDReset();
      green_pattern = previousLedState;
      inPanicMode = false;
    }

  }

  //Dont make this very large or watchdog will reset
  delay(250);
}

uint32_t calculateCRC32(const uint8_t *data, size_t length)
{
  uint32_t crc = 0xffffffff;
  while (length--) {
    uint8_t c = *data++;
    for (uint32_t i = 0x80; i > 0; i >>= 1) {
      bool bit = crc & 0x80000000;
      if (c & i) {
        bit = !bit;
      }
      crc <<= 1;
      if (bit) {
        crc ^= 0x04c11db7;
      }
    }
  }
  return crc;
}

void WriteConfigToEEPROM() {
  EEPROM.put(EEPROM_CONFIG_ADDRESS, myConfig);
  EEPROM.put(EEPROM_CHECKSUM_ADDRESS, calculateCRC32((uint8_t*)&myConfig, sizeof(cell_module_config)));
}

bool LoadConfigFromEEPROM() {
  cell_module_config restoredConfig;
  uint32_t existingChecksum;

  EEPROM.get(EEPROM_CONFIG_ADDRESS, restoredConfig);
  EEPROM.get(EEPROM_CHECKSUM_ADDRESS, existingChecksum);

  // Calculate the checksum of an entire buffer at once.
  uint32_t checksum = calculateCRC32((uint8_t*)&restoredConfig, sizeof(cell_module_config));

  if (checksum == existingChecksum) {
    //Clone the config into our global variable and return all OK
    memcpy(&myConfig, &restoredConfig, sizeof(cell_module_config));
    return true;
  }

  //Config is not configured or gone bad, return FALSE
  return false;
}

void init_i2c() {
  Wire.begin(myConfig.SLAVE_ADDR);
  Wire.onRequest(requestEvent);
  Wire.onReceive(receiveEvent);
}

void LEDReset() {
  green_pattern = GREEN_LED_PATTERN_STANDARD;
}

void factory_default() {
  EEPROM.put(EEPROM_CHECKSUM_ADDRESS, 0);
}

void Reboot() {
  TCCR1 = 0;
  TIMSK |= (1 << OCIE1A); //Disable timer1

  //Now power down loop until the watchdog timer kicks a reset
  ledGreen();

  /*
    set_sleep_mode(SLEEP_MODE_PWR_DOWN);
    adc_disable();
    sleep_enable();
    sleep_cpu();
  */
  //Infinity
  while (1) {}
}

void wait_for_buffer_ready() {
  //Just delay here so the buffers are all ready before we service i2c
  while (!buffer_ready) {
    delay(100);
    wdt_reset();
  }
}


void sendUnsignedInt(uint16_t number) {
  Wire.write((byte)((number >> 8) & 0xFF));
  Wire.write((byte)(number & 0xFF));
}

void sendByte(uint8_t number) {
  Wire.write(number);
}


union {
  float val;
  uint8_t b[4];
} float_to_bytes;

void sendFloat(float number) {
  float_to_bytes.val = number;

  Wire.write(float_to_bytes.b[0]);
  Wire.write(float_to_bytes.b[1]);
  Wire.write(float_to_bytes.b[2]);
  Wire.write(float_to_bytes.b[3]);
}

float readFloat() {
  float_to_bytes.b[0] = Wire.read();
  float_to_bytes.b[1] = Wire.read();
  float_to_bytes.b[2] = Wire.read();
  float_to_bytes.b[3] = Wire.read();

  return float_to_bytes.val;
}

void bypass_off() {
  targetByPassVoltage = 0;
  ByPassCounter = 0;
  ByPassEnabled = false;
  green_pattern = GREEN_LED_PATTERN_STANDARD;
}

union {
  uint16_t val;
  uint8_t b[2];
} uint16_t_to_bytes;

uint16_t readUINT16() {
  uint16_t_to_bytes.b[0] = Wire.read();
  uint16_t_to_bytes.b[1] = Wire.read();
  return uint16_t_to_bytes.val;
}

// function that executes whenever data is received from master
void receiveEvent(int howMany) {
  if (howMany <= 0) return;

  //If cmdByte is not zero then something went wrong in the i2c comms,
  //master failed to request any data after command
  if (cmdByte != 0) error_counter++;

  flashLed = true;

  cmdByte = Wire.read();
  howMany--;

  //Is it a command byte (there are other bytes to process) or not?
  if (bitRead(cmdByte, COMMAND_BIT)) {

    bitClear(cmdByte, COMMAND_BIT);

    switch (cmdByte) {
      case COMMAND_green_led_pattern:
        if (howMany == 1) {
          green_pattern = Wire.read();
        }
        break;

      case COMMAND_green_led_default:
        green_pattern = GREEN_LED_PATTERN_STANDARD;
        break;

      case COMMAND_factory_default:
        factory_default();
        Reboot();
        break;

      case COMMAND_set_voltage_calibration:
        if (howMany == sizeof(float)) {
          float newValue = readFloat();
          //Only accept if its different
          if (newValue != myConfig.VCCCalibration) {
            myConfig.VCCCalibration = newValue;
            WriteConfigToEEPROM();
          }
        }
        break;

      case COMMAND_set_temperature_calibration:
        if (howMany == sizeof(float)) {
          float newValue = readFloat();
          //Only accept if its different
          if (newValue != myConfig.TemperatureCalibration) {
            myConfig.TemperatureCalibration = newValue;
            WriteConfigToEEPROM();
          }
        }
        break;

      case COMMAND_set_load_resistance:
        if (howMany == sizeof(float)) {
          float newValue = readFloat();
          //Only accept if its different
          if (newValue != myConfig.LoadResistance) {
            myConfig.LoadResistance = newValue;
            WriteConfigToEEPROM();
          }
        }
        break;

      case COMMAND_set_bypass_voltage:
        if (howMany == sizeof(uint16_t)) {
          uint16_t newValue = readUINT16();
          //Only accept if its a realistic value and the value is LESS than the last voltage reading
          if (newValue >= MIN_BYPASS_VOLTAGE && newValue <= MAX_BYPASS_VOLTAGE && newValue < VCCMillivolts) {
            //TODO: Are we sure we really need all these variables??
            targetByPassVoltage = newValue;
            ByPassVCCMillivolts = VCCMillivolts;
            ByPassCounter = BYPASS_COUNTER_MAX;
            green_pattern = GREEN_LED_PATTERN_BYPASS;
            ByPassEnabled = true;
          } else {
            //Disable
            bypass_off();
          }
        }
        break;


      case COMMAND_set_slave_address:
        //Set i2c slave address and write to EEPROM, then reboot
        if (howMany == 1 ) {
          uint8_t newAddress = Wire.read();
          //Only accept if its a different address
          if (newAddress != myConfig.SLAVE_ADDR && newAddress >= DEFAULT_SLAVE_ADDR_START_RANGE && newAddress <= DEFAULT_SLAVE_ADDR_END_RANGE) {
            myConfig.SLAVE_ADDR = newAddress;
            WriteConfigToEEPROM();
            Reboot();
          }
        }
        break;

    }

    cmdByte = 0;
  } else {
    //Its a READ request

    switch (cmdByte) {
      case read_voltage:
        VCCMillivolts = Update_VCCMillivolts();
        flash_green = true;
        break;
    }
  }

  // clear rx buffer
  while (Wire.available()) Wire.read();
}


float Update_VCCMillivolts() {
  //Oversampling and take average of ADC samples use an unsigned integer or the bit shifting goes wonky
  uint32_t extraBits = 0;
  for (int k = 0; k < OVERSAMPLE_LOOP; k++) {
    extraBits = extraBits + analogVal[k];
  }
  //Shift the bits to match OVERSAMPLE_LOOP size (buffer size of 8=3 shifts, 16=4 shifts)
  //Assume perfect reference of 2560mV for reference - we will correct for this with VCCCalibration

  uint16_t raw = (extraBits >> 4);
  //TODO: DONT THINK WE NEED THIS ANY LONGER!
  //unsigned int raw = map((extraBits >> 4), 0, 1023, 0, 2560);

  //TODO: Get rid of the need for float variables....
  return (int)((float)raw * myConfig.VCCCalibration);
}

// function that executes whenever data is requested by master (this answers requestFrom command)
void requestEvent() {
  switch (cmdByte) {
    case read_voltage:
      //If we are in bypass then only return the voltage
      //thats taken outside of the resistor load being switched on
      if (ByPassEnabled) {
        sendUnsignedInt(ByPassVCCMillivolts);
      } else {
        sendUnsignedInt(VCCMillivolts);
      }

      break;

    case read_raw_voltage:
      sendUnsignedInt(last_raw_adc);
      break;

    case read_error_counter:
      sendUnsignedInt(error_counter);
      break;

    case read_bypass_voltage_measurement:
      sendUnsignedInt(ByPassVCCMillivolts);
      break;

    case read_bypass_enabled_state:
      sendByte(ByPassEnabled);
      break;

    case read_temperature:
      sendUnsignedInt((uint16_t)((float)temperature_probe * myConfig.TemperatureCalibration));
      break;

    case read_voltage_calibration:
      sendFloat(myConfig.VCCCalibration);
      break;

    case read_temperature_calibration:
      sendFloat(myConfig.TemperatureCalibration);
      break;

    case read_load_resistance:
      sendFloat(myConfig.LoadResistance);
      break;

    default:
      //Dont do anything - timeout
      break;
  }

  //Clear cmdByte
  cmdByte = 0;

  //Reset when we last processed a request, if this times out master has stopped communicating with module
  last_i2c_request = 150;
}

inline void ledGreen() {
  DDRB |= (1 << DDB1);
  PORTB |=  (1 << PB1);
}

/*
  inline void ledRed() {
  DDRB |= (1 << DDB1);
  PORTB &= ~(1 << PB1);
  }
*/
inline void ledOff() {
  //Low
  DDRB |= (1 << DDB1);
  PORTB &= ~(1 << PB1);
  /*
    //PB1 as input
    DDRB &= ~(1 << DDB1);
    //PB1 pull up OFF
    PORTB &= ~(1 << PB1);
  */
}

ISR(TIMER1_COMPA_vect)
{
  //Flash LED in sync with bit pattern


  if (green_pattern == 0) {
    if (flash_green)  {
      ledGreen();
    }
    flash_green = false;
  } else {
    ///Rotate pattern
    green_pattern = (byte)(green_pattern << 1) | (green_pattern >> 7);

    if (green_pattern & 0x01) {
      ledGreen();
    } else {
      ledOff();
    }
  }

  if (ByPassEnabled) {

    //TODO: We need to add in code here to check we don't overheat

    //This must go above the following "if (ByPassCounter > 0)" statement...
    if (ByPassCounter == 0 && analogValIndex == 0) {
      //We are in bypass and just finished an in-cycle voltage measurement
      ByPassVCCMillivolts = Update_VCCMillivolts();

      if (targetByPassVoltage >= ByPassVCCMillivolts) {
        //We reached the goal
        bypass_off();
      } else {
        //Try again
        ByPassCounter = BYPASS_COUNTER_MAX;
      }
    }

    if (ByPassCounter > 0)
    {
      //We are in ACTIVE BYPASS mode - the RESISTOR will be ACTIVE + BURNING ENERGY
      ByPassCounter--;
      digitalWrite(PB4, HIGH);

      if (ByPassCounter == 0)
      {
        //We have just finished this timed ACTIVE BYPASS mode, disable resistor
        //and measure resting voltage now before possible re-enable.
        digitalWrite(PB4, LOW);

        //Reset voltage ADC buffer
        analogValIndex = 0;
      }
    }

  } else {
    //Safety check we ensure bypass is always off if not enabled
    digitalWrite(PB4, LOW);
  }

  if (green_pattern == 0) {
    ledOff();
  }


  //trigger ADC reading
  ADCSRA |= (1 << ADSC);
}

ISR(ADC_vect) {
  // Interrupt service routine for the ADC completion
  //uint8_t adcl = ADCL;
  //uint16_t value = ADCH << 8 | adcl;

  uint16_t value = ADCW;

  //If we skip this ADC reading, quit ISR here
  if (skipNextADC) {
    skipNextADC = false;
    return;
  }

  if (reading_count == TEMP_READING_LOOP_FREQ ) {
    //Use A0 (RESET PIN) to act as an analogue input
    //note that we cannot take the pin below 1.4V or the CPU resets
    //so we use the top half between 1.6V and 2.56V (voltage reference)
    //we avoid switching references (VCC vs 2.56V) so the capacitors dont have to keep draining and recharging
    reading_count = 0;

    //We reduce the value by 512 as we have a DC offset we need to remove
    temperature_probe = value;

    // use ADC3 for input for next reading (voltage)
    ADMUX = B10010011;

    //Set skipNextADC to delay the next TIMER1 call to ADC reading to allow the ADC to settle after changing MUX
    skipNextADC = true;

  } else {

    //Populate the rolling buffer with values from the ADC
    last_raw_adc = value;
    analogVal[analogValIndex] = value;

    analogValIndex++;

    if (analogValIndex == OVERSAMPLE_LOOP) {
      analogValIndex = 0;
      buffer_ready = 1;
    }

    reading_count++;

    if (reading_count == TEMP_READING_LOOP_FREQ) {
      //use ADC0 for temp probe input on next ADC loop

      //We have to set the registers directly because the ATTINYCORE appears broken for internal 2v56 register without bypass capacitor
      ADMUX = B10010000;

      //Set skipNextADC to delay the next TIMER1 call to ADC reading to allow the ADC to settle after changing MUX
      skipNextADC = true;
    }
  }

}

void initADC()
{
  /* this function initialises the ADC

        ADC Prescaler Notes:
    --------------------

     ADC Prescaler needs to be set so that the ADC input frequency is between 50 - 200kHz.

           For more information, see table 17.5 "ADC Prescaler Selections" in
           chapter 17.13.2 "ADCSRA – ADC Control and Status Register A"
          (pages 140 and 141 on the complete ATtiny25/45/85 datasheet, Rev. 2586M–AVR–07/10)

           Valid prescaler values for various clock speeds

       Clock   Available prescaler values
           ---------------------------------------
             1 MHz   8 (125kHz), 16 (62.5kHz)
             4 MHz   32 (125kHz), 64 (62.5kHz)
             8 MHz   64 (125kHz), 128 (62.5kHz)
            16 MHz   128 (125kHz)


    ADPS2 ADPS1 ADPS0 Division Factor
    000 2
    001 2
    010 4
    011 8
    100 16
    101 32
    110 64
    111 128

  */

  //NOTE The device requries a supply voltage of 3V+ in order to generate 2.56V reference voltage.
  // http://openenergymonitor.blogspot.co.uk/2012/08/low-level-adc-control-admux.html

  //REFS1 REFS0 ADLAR REFS2 MUX3 MUX2 MUX1 MUX0
  //Internal 2.56V Voltage Reference without external bypass capacitor, disconnected from PB0 (AREF)
  //ADLAR =0 and PB3 (B0011) for INPUT (A3)
  //We have to set the registers directly because the ATTINYCORE appears broken for internal 2v56 register without bypass capacitor
  ADMUX = B10010011;

  /*
    ADMUX = (INTERNAL2V56_NO_CAP << 4) |
          (0 << ADLAR)  |     // dont left shift ADLAR
          (0 << MUX3)  |     // use ADC3 for input (PB4), MUX bit 3
          (0 << MUX2)  |     // use ADC3 for input (PB4), MUX bit 2
          (1 << MUX1)  |     // use ADC3 for input (PB4), MUX bit 1
          (1 << MUX0);       // use ADC3 for input (PB4), MUX bit 0
  */

  /*
    #if (F_CPU == 1000000)
    //Assume 1MHZ clock so prescaler to 8 (B011)
    ADCSRA =
      (1 << ADEN)  |     // Enable ADC
      (0 << ADPS2) |     // set prescaler bit 2
      (1 << ADPS1) |     // set prescaler bit 1
      (1 << ADPS0);      // set prescaler bit 0
    #endif
  */
  //#if (F_CPU == 8000000)
  //8MHZ clock so set prescaler to 64 (B110)
  ADCSRA =
    (1 << ADEN)  |     // Enable ADC
    (1 << ADPS2) |     // set prescaler bit 2
    (1 << ADPS1) |     // set prescaler bit 1
    (0 << ADPS0) |       // set prescaler bit 0
    (1 << ADIE);     //enable the ADC interrupt.
  //#endif
}

static inline void initTimer1(void)
{
  TCCR1 |= (1 << CTC1);  // clear timer on compare match
  TCCR1 |= (1 << CS13) | (1 << CS12) | (1 << CS11) | (1 << CS10); //clock prescaler 16384
  OCR1C = 64;  //About eighth of a second trigger Timer1  (there are 488 counts per second @ 8mhz)
  TIMSK |= (1 << OCIE1A); // enable compare match interrupt
}

