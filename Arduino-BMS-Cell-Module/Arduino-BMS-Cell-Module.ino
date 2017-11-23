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

  ATTINY data sheet
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

//#define SWITCH_OFF_LEDS

//LED light patterns
#define GREEN_LED_PATTERN_STANDARD B00000101
#define GREEN_LED_PATTERN_WAITREADY B11110111
#define GREEN_LED_PATTERN_UNCONFIGURED B11110000
#define RED_LED_OFF 0
#define RED_LED_PANIC B01010101

//Where in EEPROM do we store the configuration
#define EEPROM_CHECKSUM_ADDRESS 0
#define EEPROM_CONFIG_ADDRESS 20

//Default i2c SLAVE address (used for auto provision of address)
#define DEFAULT_SLAVE_ADDR 0x15

//Number of times we sample and average the ADC for voltage
#define OVERSAMPLE_LOOP 16

//If we receive a cmdByte with BIT 5 set its a command byte so there is another byte waiting for us to process
#define COMMAND_BIT 5
#define command_green_led_pattern   1
#define command_led_off   2
#define command_factory_default 3

//Read values
#define read_voltage 10
#define read_temperature 11

//#define wdt_reset()  __asm__ __volatile__ ("wdr") 
//#define adc_disable() (ADCSRA &= ~(1<<ADEN)) // disable ADC (before power-off)
//#define adc_enable()  (ADCSRA |=  (1<<ADEN)) // re-enable ADC

  
volatile bool skipNextADC = false;
volatile uint8_t green_pattern = B10100000;
volatile uint8_t red_pattern = 0;
volatile uint16_t analogVal[OVERSAMPLE_LOOP];
volatile uint16_t temperature_probe = 0;
volatile uint8_t analogValIndex;
volatile uint8_t buffer_ready = 0;
volatile uint8_t reading_count = 0;
volatile uint8_t cmdByte = 0;
volatile uint8_t ledOffCount = 0;
volatile uint8_t last_i2c_request = 255;
volatile uint16_t VCCMillivolts = 0;

uint16_t error_counter = 0;

//Number of voltage readings to take before we take a temperature reading
#define TEMP_READING_LOOP_FREQ 16

struct cell_module_config {
  // 7 bit slave I2C address
  uint8_t SLAVE_ADDR = DEFAULT_SLAVE_ADDR;
  // Calibration factor for voltage readings
  float VCCCalibration = 1.630;
};

static cell_module_config myConfig;

void setup() {
  //Must be first line of setup()
  MCUSR &= ~(1<<WDRF); // reset status flag
  wdt_disable();

  //Pins PB0 (SDA) and PB2 (SCLOCK) are for the i2c comms with master
  //PINS https://github.com/SpenceKonde/ATTinyCore/blob/master/avr/extras/ATtiny_x5.md

  //pinMode(A3, INPUT);  //A3 pin 3 (PB3)
  //pinMode(A0, INPUT);  //reset pin A0 pin 5 (PB5)

  DDRB &= ~(1 << DDB3);
  PORTB &= ~(1 << PB3);

  DDRB &= ~(1 << DDB5);
  PORTB &= ~(1 << PB5);

  ledOff();

  //Load our EEPROM configuration
  if (!LoadConfigFromEEPROM()) {
    //If bad configuration, flash RED led for 2 seconds

    for (byte a = 0; a < 6; a++) {
      ledRed();
      delay(250);
      ledOff();
      delay(250);
    }
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

  if (myConfig.SLAVE_ADDR == DEFAULT_SLAVE_ADDR) {
    green_pattern = GREEN_LED_PATTERN_UNCONFIGURED;
  }
}



void loop() {
  //We have had at least one i2c request and not currently in PANIC mode
  if (red_pattern == 0 && last_i2c_request == 0) {
    //Panic after 8 seconds of not receiving i2c requests...

    red_pattern = RED_LED_PANIC;

    //Try resetting the i2c bus
    Wire.end();
    init_i2c();
    error_counter++;

  } else if (red_pattern != 0 && last_i2c_request > 0) {
    //Just come back from a PANIC situation
    LEDReset();
  }

  delay(250);

  if (last_i2c_request != 0) {
    //Count down loop for requests to see if i2c bus hangs or controller stops talking
    last_i2c_request--;
  }

  wdt_reset();
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
  red_pattern = RED_LED_OFF;
  green_pattern = GREEN_LED_PATTERN_STANDARD;
}

void factory_default() { 
  EEPROM.put(EEPROM_CHECKSUM_ADDRESS, 0);

  TCCR1 = 0;
  TIMSK |= (1 << OCIE1A); //Disable timer1

  //Now power down loop until the watchdog timer kicks a reset
  ledRed();

/*
  set_sleep_mode(SLEEP_MODE_PWR_DOWN);
  adc_disable();
  sleep_enable();
  sleep_cpu();
*/
  //Infinity
  while(1) {}
}

void wait_for_buffer_ready() {
  //Just delay here so the buffers are all ready before we service i2c
  green_pattern = GREEN_LED_PATTERN_WAITREADY;
  while (!buffer_ready) {
    delay(100);
    wdt_reset();
  }
}

// function that executes whenever data is received from master
void receiveEvent(int howMany) {
  if (howMany <= 0) return;

  //If cmdByte is not zero then something went wrong in the i2c comms,
  //master failed to request any data after command
  if (cmdByte != 0) error_counter++;

  cmdByte = Wire.read();
  howMany--;

  //Is it a command byte (there are other bytes to process) or not?
  if (bitRead(cmdByte, COMMAND_BIT)) {

    bitClear(cmdByte, COMMAND_BIT);

    switch (cmdByte) {
      case command_green_led_pattern:
        if (howMany == 1) {
          green_pattern = Wire.read();
        }
        break;

      case command_led_off:
        //Switch green LED off for a short while
        ledOffCount = 100;
        break;

      case command_factory_default:
        factory_default();
        break;

    }

    cmdByte = 0;
  } else {
    //Its a READ request

    switch (cmdByte) {
      case read_voltage:
        //TODO: PERHAPS THIS SHOULD BE IN THE LOOP()
        //Prepare the VCCMillivolts
        //Oversampling and take average of ADC samples use an unsigned integer or the bit shifting goes wonky
        uint32_t extraBits = 0;
        for (int k = 0; k < OVERSAMPLE_LOOP; k++) {
          extraBits = extraBits + analogVal[k];
        }
        //Shift the bits to match OVERSAMPLE_LOOP size (buffer size of 8=3 shifts, 16=4 shifts)
        //Assume perfect reference of 2560mV for reference - we will correct for this with VCCCalibration

        //TODO: DONT THINK WE NEED THIS ANY LONGER!
        unsigned int raw = map((extraBits >> 4), 0, 1023, 0, 2560);

        //TODO: Get rid of the need for float variables....
        VCCMillivolts = (int)((float)raw * myConfig.VCCCalibration);

        break;
    }
  }

  // clear rx buffer
  while (Wire.available()) Wire.read();
}

void sendUnsignedInt(uint16_t number) {
  byte ret1 = Wire.write((byte)((number >> 8) & 0xFF));
  byte ret2 = Wire.write((byte)(number & 0xFF));
}

// function that executes whenever data is requested by master (this answers requestFrom command)
void requestEvent() {
  //  if (!buffer_ready) return;

  switch (cmdByte) {
    case read_voltage:
      sendUnsignedInt(VCCMillivolts);
      break;

    case read_temperature:
      sendUnsignedInt(temperature_probe);
      break;

    default:
      //Dont do anything - timeout
      break;
  }

  //Clear cmdByte
  cmdByte = 0;

  //Reset when we last processed a request
  last_i2c_request = 60;
}

inline void ledGreen() {
  DDRB |= (1 << DDB1);
  PORTB |=  (1 << PB1);
}

inline void ledRed() {
  DDRB |= (1 << DDB1);
  PORTB &= ~(1 << PB1);
}

inline void ledOff() {
  //PB1 as input
  DDRB &= ~(1 << DDB1);
  //PB1 pull up OFF
  PORTB &= ~(1 << PB1);
}


ISR(TIMER1_COMPA_vect)
{
  //Flash LED in sync with bit pattern
  if (ledOffCount > 0) {
    ledOffCount--;
    ledOff();
  } else if (red_pattern == 0 ) {
    ///Rotate pattern
    green_pattern = (byte)(green_pattern << 1) | (green_pattern >> 7);
    if (green_pattern & 0x01) {
      ledGreen();
    } else {
      ledOff();
    }
  } else {
    red_pattern = (byte)(red_pattern << 1) | (red_pattern >> 7);
    if (red_pattern & 0x01) {
      ledRed();
    } else {
      ledOff();
    }
  }


  //If we skip this ADC reading, quit ISR here
  if (skipNextADC) {
    skipNextADC = false;
  } else {
    //trigger ADC reading
    ADCSRA |= (1 << ADSC);
  }
}

ISR(ADC_vect) {
  // Interrupt service routine for the ADC completion
  unsigned int  value = ADCL | (ADCH << 8);

  if (reading_count == TEMP_READING_LOOP_FREQ ) {
    //Use A0 (RESET PIN) to act as an analogue input
    //note that we cannot take the pin below 1.4V or the CPU resets
    //so we use the top half between 1.6V and 2.56V (voltage reference)
    //we avoid switching references (VCC vs 2.56V) so the capacitors dont have to keep draining and recharging
    reading_count = 0;

    temperature_probe = value;

    // use ADC3 for input for next reading (voltage)
    ADMUX = B10010011;
    /*
      ADMUX = (INTERNAL2V56 << 4) |
             (0 << ADLAR)  |
             (0 << MUX3)  |
             (0 << MUX2)  |
             (1 << MUX1)  |
             (1 << MUX0);
    */

    //Set skipNextADC to delay the next TIMER1 call to ADC reading to allow the
    //ADC to settle after changing MUX
    skipNextADC = true;
  } else {
    //Populate the rolling buffer with values from the ADC

    analogVal[analogValIndex] = value;

    analogValIndex++;

    if (analogValIndex == OVERSAMPLE_LOOP) {
      analogValIndex = 0;
      buffer_ready = 1;
    }

    reading_count++;

    if (reading_count == TEMP_READING_LOOP_FREQ) {
      // use ADC0 for temp probe input on next ADC loop

      //We have to set the registers directly because the ATTINYCORE appears broken for internal 2v56 register without bypass capacitor
      ADMUX = B10010000;

      /*
        ADMUX = (INTERNAL2V56 << 4) |
               (0 << ADLAR)  |
               (0 << MUX3)  |
               (0 << MUX2)  |
               (0 << MUX1)  |
               (0 << MUX0);
      */

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
  //ADLAR =0 and PB3 for INPUT (A3)
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

#if (F_CPU == 1000000)
  //Assume 1MHZ clock so prescaler to 8 (B011)
  ADCSRA =
    (1 << ADEN)  |     // Enable ADC
    (0 << ADPS2) |     // set prescaler bit 2
    (1 << ADPS1) |     // set prescaler bit 1
    (1 << ADPS0);      // set prescaler bit 0
#endif

#if (F_CPU == 8000000)
  //8MHZ clock so set prescaler to 64 (B110)
  ADCSRA =
    (1 << ADEN)  |     // Enable ADC
    (1 << ADPS2) |     // set prescaler bit 2
    (1 << ADPS1) |     // set prescaler bit 1
    (0 << ADPS0);       // set prescaler bit 0
#endif

  ADCSRA |= (1 << ADIE);     //enable the ADC interrupt.
}

static inline void initTimer1(void)
{
  TCCR1 |= (1 << CTC1);  // clear timer on compare match
  TCCR1 |= (1 << CS13) | (1 << CS12) | (1 << CS11) | (1 << CS10); //clock prescaler 16384
  OCR1C = 128;  //About quarter second trigger Timer1  (there are 488 counts per second @ 8mhz)
  TIMSK |= (1 << OCIE1A); // enable compare match interrupt
}



