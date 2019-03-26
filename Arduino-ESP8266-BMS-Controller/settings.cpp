#include "Arduino.h"

#include <EEPROM.h>

#include "settings.h"
wifi_eeprom_settings myConfig_WIFI;
eeprom_settings myConfig;



uint32_t calculateCRC32(const uint8_t *data, size_t length)
{
  //This calculates a CRC32 the same as used in MPEG2 streams
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


void FactoryResetSettings() {
  const char emoncms_host[] = "192.168.0.26";
  const char emoncms_apikey[]="1234567890abcdef1234567890abcdef";
  const char emoncms_url[]="/emoncms/input/bulk?data=";

  const char influxdb_host[] = "192.168.0.129";
 
  const char influxdb_database[] = "powerwall";
  const char influxdb_user[] = "root";
  const char influxdb_password[] = "root";

  strcpy(myConfig.influxdb_host, influxdb_host );
  strcpy(myConfig.influxdb_database, influxdb_database );
  strcpy(myConfig.influxdb_user, influxdb_user );
  strcpy(myConfig.influxdb_password, influxdb_password );

  myConfig.influxdb_enabled=false;
  myConfig.influxdb_httpPort=8086;

  strcpy(myConfig.emoncms_host, emoncms_host );
  strcpy(myConfig.emoncms_apikey, emoncms_apikey);
  strcpy(myConfig.emoncms_url, emoncms_url);

  myConfig.emoncms_enabled=false;
  myConfig.emoncms_node_offset = 50 - 24;//DEFAULT_SLAVE_ADDR_START_RANGE;
  myConfig.emoncms_httpPort = 80;

  WriteConfigToEEPROM();
}

void WriteConfigToEEPROM() {
  uint32_t checksum = calculateCRC32((uint8_t*)&myConfig, sizeof(eeprom_settings));
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
  uint32_t checksum = calculateCRC32((uint8_t*)&restoredConfig, sizeof(eeprom_settings));

  Serial.println(checksum, HEX);
  Serial.println(existingChecksum, HEX);

  if (checksum == existingChecksum) {
    //Clone the config into our global variable and return all OK
    memcpy(&myConfig, &restoredConfig, sizeof(eeprom_settings));   
    return true;
  }

  
  //Config is not configured or gone bad, return FALSE
  Serial.println("Config is not configured or gone bad");
  return false;
}





void WriteWIFIConfigToEEPROM() {
  uint32_t checksum = calculateCRC32((uint8_t*)&myConfig_WIFI, sizeof(wifi_eeprom_settings));
  EEPROM.begin(EEPROM_storageSize);
  EEPROM.put(EEPROM_WIFI_CONFIG_ADDRESS, myConfig_WIFI);
  EEPROM.put(EEPROM_WIFI_CHECKSUM_ADDRESS, checksum);
  EEPROM.end();
  Serial.print("WriteWIFIConfigToEEPROM Checksum:");
  Serial.println(checksum);
}



bool LoadWIFIConfigFromEEPROM() {
  wifi_eeprom_settings restoredConfig;
  uint32_t existingChecksum;
  memset(&restoredConfig, 0, sizeof(wifi_eeprom_settings));
  EEPROM.begin(EEPROM_storageSize);
  EEPROM.get(EEPROM_WIFI_CONFIG_ADDRESS, restoredConfig);
  EEPROM.get(EEPROM_WIFI_CHECKSUM_ADDRESS, existingChecksum);
  EEPROM.end();

  // Calculate the checksum of an entire buffer at once.
  uint32_t checksum = calculateCRC32((uint8_t*)&restoredConfig, sizeof(wifi_eeprom_settings));
  if (checksum == existingChecksum) {
    //Clone the config into our global variable and return all OK
    memcpy(&myConfig_WIFI, &restoredConfig, sizeof(wifi_eeprom_settings));
    return true;
  }

  //Config is not configured or gone bad, return FALSE
  return false;
}

