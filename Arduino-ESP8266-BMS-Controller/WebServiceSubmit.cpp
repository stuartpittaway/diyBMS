#include "WebServiceSubmit.h"
#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ESP8266HTTPClient.h>

extern uint8_t DEFAULT_SLAVE_ADDR_START_RANGE;
extern int balance_status;
extern bool InverterMon;
extern double AmpsRMS;

//Implements EmonCMS WebServiceSubmit abstract/interface class
void EmonCMS::postData(eeprom_settings myConfig, cell_module (&cell_array)[24], int cell_array_max) {
  if (!myConfig.emoncms_enabled) return;

  String url = myConfig.emoncms_url;// "/emoncms/input/bulk?data=";

  url += "[";

  for (int a = 0; a < cell_array_max; a++) {

    //Ensure its a sensible value to avoid filling emonCMS graph with high values
    url += "[0," + String(myConfig.emoncms_node_offset + cell_array[a].address) + ",{\"V\":"
           + String(cell_array[a].valid_values ? cell_array[a].voltage : 0)
           + "},{\"T\":"
           //Ensure temperature is in a sensible range
           + String(cell_array[a].valid_values ? cell_array[a].temperature : 0)
           + "},{\"B\":"
           //Ensure Bypass is in a sensible range
           + String(cell_array[a].valid_values ? cell_array[a].bypass_status : 0)
           + "}]";

    if (a < cell_array_max - 1) {
      url += ",";
    }
  }

  url += "]&offset=0&apikey=" + String(myConfig.emoncms_apikey);

  WiFiClient client;

  if (!client.connect(myConfig.emoncms_host, myConfig.emoncms_httpPort)) {
    Serial.println("connection failed");

  } else {
    // This will send the request to the server
    client.print(String("GET ") + url + " HTTP/1.1\r\nHost: " + myConfig.emoncms_host + "\r\nConnection: close\r\n\r\n");

    unsigned long timeout = millis() + 2500;
    // Read all the lines of the reply from server and print them to Serial
    while (client.connected())
    {
      yield();

      if (millis() > timeout) {
        Serial.println(">>> Client Timeout !");
        client.stop();
        return;
      }

      if (client.available())
      {
        String line = client.readStringUntil('\n');
        Serial.println(line);
      }
    }
    client.stop();
  }
}

//Implements Influxdb WebServiceSubmit abstract/interface class
void Influxdb::postData(eeprom_settings myConfig, cell_module (&cell_array)[24], int cell_array_max) {
  if (!myConfig.influxdb_enabled) return;
  String poststring;
  String poststring1;
  HTTPClient http;
  
  //Construct URL for the influxdb
  String url = "http://" + String(myConfig.influxdb_host) + ":" + myConfig.influxdb_httpPort + "/write?db=" + String(myConfig.influxdb_database) ;

  //Cycle through each module and push the voltage to grafana using a http post.

  for (int a = 0; a < cell_array_max; a++) {
    //Ensure its a sensible value to avoid filling influxdb db with high values
    if (cell_array[a].valid_values==true) {
      poststring = poststring + ("Cells,Cell=" + String(a+1) + " " + "Voltage=" + String(cell_array[a].voltage) + ",Temp=" + String(cell_array[a].temperature) + ",Bypass=" + int(cell_array[a].bypass_status) + "\n");
     }
  }
  poststring = poststring + ("Cells Battery-Balancing=" + String(balance_status) + "\n");
  if (myConfig.invertermon_enabled == true) poststring = poststring + ("Cells Inverter-Usage=" + String(AmpsRMS) + "\n");

  http.begin(url);
  http.addHeader("Content-Type", "data-binary");
  int httpCode = http.POST(poststring);
  String payload = http.getString();
  Serial.println (payload);
  
  WiFiClient client;

  if (!client.connect(myConfig.influxdb_host, myConfig.influxdb_httpPort)) {
    Serial.println("connection failed");

  } else {
    // This will send the request to the server
    client.print(String("GET ") + url + " HTTP/1.1\r\nHost: " + myConfig.influxdb_host + "\r\nConnection: close\r\n\r\n");
    
    unsigned long timeout = millis() + 2500;
    // Read all the lines of the reply from server and print them to Serial
    while (client.connected())
    {
      yield();

      if (millis() > timeout) {
        Serial.println(">>> Client Timeout !");
        client.stop();
        return;
      }

      if (client.available())
      {
        String line = client.readStringUntil('\n');
        //Serial.println(line);
      }
    }
    client.stop();
  }
}
