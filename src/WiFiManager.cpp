#include <Arduino.h>
#include <ESP8266WiFi.h>
#include "WiFiManager.h"

// WiFi credentials
const char *ssid = "Internet";
const char *password = "(/DDD)HD)988793hdHhd39hd987887";

bool WiFiManager::isConnected()
{
    int retries = 0;
    bool connected = true;
    Serial.println("Checking Wifi connection...");
    while (retries <= 3 && !WiFi.isConnected())
    {
        Serial.println("WiFi not connected");
        Serial.printf("Retry %d from 3\n", retries++);
        Serial.println("Retrying in one second");
        connected = false;

        Serial.println("Attempting Wifi  connection...");
        WiFi.begin(ssid, password);
        delay(5000);

        if (WiFi.isConnected())
        {
            Serial.println("");
            Serial.println("WiFi connected");
            Serial.println("IP address: " + WiFi.localIP().toString());
            connected = true;
        }
    }

    return connected;
}