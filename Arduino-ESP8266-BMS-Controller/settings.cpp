#include "Arduino.h"
#include <CRC32.h>
#include <EEPROM.h>

#include "settings.h"
wifi_eeprom_settings myConfig_WIFI;
eeprom_settings myConfig;


void WriteConfigToEEPROM() {
  uint32_t checksum = CRC32::calculate(&myConfig, sizeof(eeprom_settings));
  EEPROM.begin(EEPROM_storageSize);
  EEPROM.put(EEPROM_CONFIG_ADDRESS, myConfig);
  EEPROM.put(EEPROM_CHECKSUM_ADDRESS, checksum);
  EEPROM.end();
}

bool LoadConfigFromEEPROM() {
  eeprom_settings restoredConfig;
  uint32_t existingChecksum;

  EEPROM.begin(EEPROM_storageSize);
  EEPROM.get(EEPROM_CONFIG_ADDRESS, restoredConfig);
  EEPROM.get(EEPROM_CHECKSUM_ADDRESS, existingChecksum);
  EEPROM.end();

  // Calculate the checksum of an entire buffer at once.
  uint32_t checksum = CRC32::calculate(&restoredConfig, sizeof(eeprom_settings));

  Serial.println(checksum);
  Serial.println(existingChecksum);

  if (checksum == existingChecksum) {
    //Clone the config into our global variable and return all OK
    memcpy(&myConfig, &restoredConfig, sizeof(eeprom_settings));
    return true;
  }

  //Config is not configured or gone bad, return FALSE
  return false;
}

void WriteWIFIConfigToEEPROM() {
  uint32_t checksum = CRC32::calculate(&myConfig_WIFI, sizeof(wifi_eeprom_settings));

  EEPROM.begin(EEPROM_storageSize);
  EEPROM.put(EEPROM_WIFI_CONFIG_ADDRESS, myConfig_WIFI);
  EEPROM.put(EEPROM_WIFI_CHECKSUM_ADDRESS, checksum);
  EEPROM.end();

  Serial.println(checksum);
}

bool LoadWIFIConfigFromEEPROM() {
  wifi_eeprom_settings restoredConfig;
  uint32_t existingChecksum;

  memset(&restoredConfig,0,sizeof(wifi_eeprom_settings));

  EEPROM.begin(EEPROM_storageSize);
  EEPROM.get(EEPROM_WIFI_CONFIG_ADDRESS, restoredConfig);
  EEPROM.get(EEPROM_WIFI_CHECKSUM_ADDRESS, existingChecksum);
  EEPROM.end();

  // Calculate the checksum of an entire buffer at once.
  uint32_t checksum = CRC32::calculate(&restoredConfig, sizeof(wifi_eeprom_settings));

  Serial.println(restoredConfig.wifi_ssid);
  Serial.println(restoredConfig.wifi_passphrase);

  Serial.println(checksum);
  Serial.println(existingChecksum);

  if (checksum == existingChecksum) {
    //Clone the config into our global variable and return all OK
    memcpy(&myConfig_WIFI, &restoredConfig, sizeof(wifi_eeprom_settings));
    return true;
  }

  //Config is not configured or gone bad, return FALSE
  return false;
}

