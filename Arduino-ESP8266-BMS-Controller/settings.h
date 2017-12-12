#include "Arduino.h"

#ifndef config_settings_H_
#define config_settings_H_


//Where in EEPROM do we store the configuration
#define EEPROM_storageSize 1024
#define EEPROM_WIFI_CHECKSUM_ADDRESS 0
#define EEPROM_WIFI_CONFIG_ADDRESS EEPROM_WIFI_CHECKSUM_ADDRESS+sizeof(uint32_t)

#define EEPROM_CHECKSUM_ADDRESS 256
#define EEPROM_CONFIG_ADDRESS EEPROM_CHECKSUM_ADDRESS+sizeof(uint32_t)

struct wifi_eeprom_settings {
  char wifi_ssid[32 + 1];
  char wifi_passphrase[63 + 1];
};

struct eeprom_settings {
  //Allow up to 24 modules
  uint8_t address_list[24];
};



extern wifi_eeprom_settings myConfig_WIFI;
extern eeprom_settings myConfig;

void WriteConfigToEEPROM();
bool LoadConfigFromEEPROM();
void WriteWIFIConfigToEEPROM();
bool LoadWIFIConfigFromEEPROM();

#endif

