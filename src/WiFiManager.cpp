#include <Arduino.h>
#include <ESP8266WiFi.h>
#include "WiFiManager.h"

// WiFi credentials
const char *ssid = "Internet";
const char *password = "(/DDD)HD)988793hdHhd39hd987887";
bool wifiConnected = false;

bool WiFiManager::isConnected()
{
    int retries = 0;
    while (retries <= 3 && !wifiConnected)
    {
        Serial.println("WiFi not connected");
        Serial.printf("Retry %d from 3\n", retries++);
        Serial.println("Retrying in one second");

        if (!WiFi.isConnected())
        {
            Serial.println("Attempting Wifi  connection...");
            WiFi.begin(ssid, password);
            delay(5000);
            if (WiFi.isConnected())
            {
                Serial.println("");
                Serial.println("WiFi connected");
                Serial.println("IP address: " + WiFi.localIP().toString());
                wifiConnected = true;
            }
        }
        else
        {
            wifiConnected = true; // Aktualisiere den Wi-Fi-Status
        }
    }

    return wifiConnected;
}

void WiFiManager::disconnect()
{
    WiFi.disconnect();
}