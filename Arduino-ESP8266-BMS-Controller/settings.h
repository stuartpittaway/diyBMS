#include "Arduino.h"

#ifndef config_settings_H_
#define config_settings_H_


//Where in EEPROM do we store the configuration
#define EEPROM_storageSize 2048
#define EEPROM_WIFI_CHECKSUM_ADDRESS 0
#define EEPROM_WIFI_CONFIG_ADDRESS EEPROM_WIFI_CHECKSUM_ADDRESS+sizeof(uint32_t)

#define EEPROM_CHECKSUM_ADDRESS 512
#define EEPROM_CONFIG_ADDRESS EEPROM_CHECKSUM_ADDRESS+sizeof(uint32_t)

struct wifi_eeprom_settings {
  char wifi_ssid[32 + 1];
  char wifi_passphrase[63 + 1];
};

//We have allowed space for 2048-512 bytes of EEPROM for settings (1536 bytes)
struct eeprom_settings { 
  bool emoncms_enabled;
  uint8_t emoncms_node_offset;
  int emoncms_httpPort;
  char emoncms_host[64 + 1];
  char emoncms_apikey[32 + 1];
  char emoncms_url[64 + 1];
  bool influxdb_enabled;
  char influxdb_host[64 +1 ];
  int influxdb_httpPort;
  char influxdb_database[32 + 1];
  char influxdb_user[32 + 1];
  char influxdb_password[32 + 1];
  bool autobalance_enabled;
  bool invertermon_enabled;
  float max_voltage;
  float balance_voltage;
  float balance_dev;
};

extern wifi_eeprom_settings myConfig_WIFI;
extern eeprom_settings myConfig;

void WriteConfigToEEPROM();
bool LoadConfigFromEEPROM();
void WriteWIFIConfigToEEPROM();
bool LoadWIFIConfigFromEEPROM();
void FactoryResetSettings();

#endif

