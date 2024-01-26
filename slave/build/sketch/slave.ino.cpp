#include <Arduino.h>
#line 1 "/home/esarandon/Arduino/esp32/espNow/slave/slave.ino"
// PinOut for ESP8266
// SDA D2
// SCL D1
// RST D4
// BUSY D3

#include <ESP8266WiFi.h>
#include <espnow.h>
#include "EPD_1in9.h"
#include <iostream>
#include <stdio.h>
#include <Wire.h>
#include <string>

#define CHANNEL 1

// Eink config
char digit_left[] = {0xbf, 0x00, 0xfd, 0xf5, 0x47, 0xf7, 0xff, 0x21, 0xff, 0xf7, 0x00};  // individual segments for the left part od the digit, index 10 is empty
char digit_right[] = {0x1f, 0x1f, 0x17, 0x1f, 0x1f, 0x1d, 0x1d, 0x1f, 0x1f, 0x1f, 0x00}; // individual segments for the right part od the digit, index 10 is empty
unsigned char eink_segments[] = {
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
}; // all white, but will be updated later with proper digits

char temperature_digits[] = {1, 2, 3, 4}; // temperature digits > 1, 2, 3, 4 = 123.4°C
char humidity_digits[] = {5, 6, 7};       // humidity digits > 5, 6, 7 = 56.7%

// Define incoming data stricture
typedef struct struct_message
{
    float t;
    float h;
} struct_message;

// Create structured data object
struct_message incomingData;

float temperature_last_measure = 0;
float humidity_last_measure = 0;

#line 54 "/home/esarandon/Arduino/esp32/espNow/slave/slave.ino"
void setup();
#line 70 "/home/esarandon/Arduino/esp32/espNow/slave/slave.ino"
void loop();
#line 76 "/home/esarandon/Arduino/esp32/espNow/slave/slave.ino"
void OnDataRecv(uint8_t *mac_addr, uint8_t *data, uint8_t data_len);
#line 88 "/home/esarandon/Arduino/esp32/espNow/slave/slave.ino"
void printEink(float temperature, float humidity);
#line 54 "/home/esarandon/Arduino/esp32/espNow/slave/slave.ino"
void setup()
{
    Serial.begin(115200);
    WiFi.mode(WIFI_AP);
    WiFi.softAP("RX_DHT22", "RX_DHT22_Password", CHANNEL, 0);

    Wire.begin();
    Serial.println("e-Paper init and clear");
    GPIOInit();
    EPD_1in9_init();

    esp_now_init();
    esp_now_set_self_role(ESP_NOW_ROLE_SLAVE);
    esp_now_register_recv_cb(OnDataRecv);
}

void loop()
{
    // Just chill
}

// callback when data is recv from Master
void OnDataRecv(uint8_t *mac_addr, uint8_t *data, uint8_t data_len)
{
    memcpy(&incomingData, data, sizeof(incomingData));
    float temperature_value = incomingData.t;
    float humidity_value = incomingData.h;
    if(temperature_value != temperature_last_measure || humidity_value != humidity_last_measure){
        printEink(temperature_value, humidity_value);
        temperature_last_measure = temperature_value;
        humidity_last_measure = humidity_value;
    }    
}

void printEink(float temperature, float humidity){
    // some updates for the e-ink display, I don´t understand most of it, so I´m keepin it here
    EPD_1in9_lut_5S();
    EPD_1in9_Write_Screen(DSPNUM_1in9_off);
    delay(500);
    EPD_1in9_lut_GC();
    EPD_1in9_lut_DU_WB();

    // set correct digits values based on the temperature
    temperature_digits[0] = int(temperature / 100) % 10;
    temperature_digits[1] = int(temperature / 10) % 10;
    temperature_digits[2] = int(temperature) % 10;
    temperature_digits[3] = int(temperature * 10) % 10;

    // set correct digits values based on the humidity
    humidity_digits[0] = int(humidity / 10) % 10;
    humidity_digits[1] = int(humidity) % 10;
    humidity_digits[2] = int(humidity * 10) % 10;

    // do not show leading zeros for values <100 and <10 both temperature and humidity
    if (temperature < 100)
    {
        temperature_digits[0] = 10;
    }
    if (temperature < 10)
    {
        temperature_digits[1] = 10;
    }

    if (humidity < 10)
    {
        humidity_digits[0] = 10;
    }

    // temperature digits
    eink_segments[0] = digit_right[temperature_digits[0]];
    eink_segments[1] = digit_left[temperature_digits[1]];
    eink_segments[2] = digit_right[temperature_digits[1]];
    eink_segments[3] = digit_left[temperature_digits[2]];
    eink_segments[4] = digit_right[temperature_digits[2]] | B00100000 /* decimal point */;
    eink_segments[11] = digit_left[temperature_digits[3]];
    eink_segments[12] = digit_right[temperature_digits[3]];

    // humidity digits
    eink_segments[5] = digit_left[humidity_digits[0]];
    eink_segments[6] = digit_right[humidity_digits[0]];
    eink_segments[7] = digit_left[humidity_digits[1]];
    eink_segments[8] = digit_right[humidity_digits[1]] | B00100000 /* decimal point */;
    eink_segments[9] = digit_left[humidity_digits[2]];
    eink_segments[10] = digit_right[humidity_digits[2]] | B00100000 /* percentage sign */;

    // special symbols - °C / °F, bluetooth, battery
    eink_segments[13] = 0x05 /* °C */;
    //| B00001000 /* bluetooth */ | B00010000 /* battery icon */;

    // write segments to the e-ink screen
    EPD_1in9_Write_Screen(eink_segments);

    EPD_1in9_sleep();
}

