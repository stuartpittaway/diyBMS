#include "WebServiceSubmit.h"
#include <ESP8266WiFi.h>
#include <WiFiClient.h>

extern uint8_t DEFAULT_SLAVE_ADDR_START_RANGE;

const char* emoncms_host = "192.168.0.26";
const char* emoncms_apikey = "yourkeygoeshere";
const uint8_t emoncms_node_offset = 50 - DEFAULT_SLAVE_ADDR_START_RANGE;
const int emoncms_httpPort = 80;

//Implements WebServiceSubmit abstract/interface class
void EmonCMS::postData(cell_module (&cell_array)[24], int cell_array_max) {

  String url = "/emoncms/input/bulk?data=[";

  for (int a = 0; a < cell_array_max; a++) {

    //Ensure its a sensible value to avoid filling emonCMS graph with high values
      url += "[0," + String(emoncms_node_offset + cell_array[a].address) + ",{\"V\":";
      url += String(cell_array[a].voltage < 5000 ? cell_array[a].voltage : 0);
      url += "},{\"T\":";
      //Ensure temperature is in a sensible range
      url += String(cell_array[a].temperature < 1024 ? cell_array[a].temperature : 0 );
      url += "}]";

      if (a < cell_array_max - 1) {
        url += ",";
      }
  }

  url += "]&offset=0&apikey=" + String(emoncms_apikey);

  //Serial.println(url);

  WiFiClient client;

  if (!client.connect(emoncms_host, emoncms_httpPort)) {
    Serial.println("connection failed");

  } else {
    // This will send the request to the server
    client.print(String("GET ") + url + " HTTP/1.1\r\nHost: " + emoncms_host + "\r\nConnection: close\r\n\r\n");

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
