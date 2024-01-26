#include <espnow.h>
#include <ESP8266WiFi.h>
#include "DHT.h"

// Create DHT22 parameters
#define DHTTYPE DHT22
#define DHTPIN 13

#define CHANNEL 1

// Create DHT Object
DHT dht(DHTPIN, DHTTYPE);
float temperature_value;
float humidity_value;

// Define data stricture
typedef struct struct_message
{
    float t;
    float h;
} struct_message;

// Create structured data object
struct_message myData;

// All the data about the peer
struct SlaveInfo
{
    uint8_t peer_addr[6];
    uint8_t channel;
    bool encrypt;
};

SlaveInfo slave;

void setup()
{
    Serial.begin(115200);
    dht.begin();
    WiFi.mode(WIFI_STA); // Set device as a Wi-Fi Station
    esp_now_init();
    esp_now_set_self_role(ESP_NOW_ROLE_CONTROLLER);
    esp_now_register_send_cb(OnDataSent);
    ScanForSlave(); // WiFi.macAddress()
    esp_now_add_peer(slave.peer_addr, ESP_NOW_ROLE_SLAVE, slave.channel, NULL, 0);
}

void loop()
{
    // Read DHT22
    temperature_value = dht.readTemperature();
    delay(10);
    humidity_value = dht.readHumidity();
    delay(10);

    // Add to structure data object
    myData.t = temperature_value;
    myData.h = humidity_value;
    esp_now_send(slave.peer_addr, (uint8_t *)&myData, sizeof(myData));
    delay(300000);
}

// Scan for slaves in AP mode and ad as peer if found
void ScanForSlave()
{
    int8_t scanResults = WiFi.scanNetworks();

    for (int i = 0; i < scanResults; i++)
    {
        String SSID = WiFi.SSID(i);
        String BSSIDstr = WiFi.BSSIDstr(i);

        if (SSID.indexOf("RX") == 0)
        {
            int mac[6];
            if (6 == sscanf(BSSIDstr.c_str(), "%x:%x:%x:%x:%x:%x", &mac[0], &mac[1], &mac[2], &mac[3], &mac[4], &mac[5]))
            {
                for (int ii = 0; ii < 6; ++ii)
                {
                    slave.peer_addr[ii] = (uint8_t)mac[ii];
                }
            }

            slave.channel = CHANNEL; // pick a channel
            slave.encrypt = 0;
            break;
        }
    }
}

// callback when data is sent from Master to Slave
void OnDataSent(uint8_t *mac_addr, uint8_t sendStatus)
{
    Serial.print("I sent my data -> ");
    Serial.print(myData.t);
    Serial.print(", ");
    Serial.println(myData.h);
}
