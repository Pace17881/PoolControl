#include <Arduino.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include <SimpleTimer.h>
#include "MQTTManager.h"
#include "WiFiManager.h"

// Pins for sensors
#define ONE_WIRE_BUS 2 // 13

// Pins for the relays
#define relayMasterPin 4
#define relayMotorPin 5

// Setup a OneWire instance to communicate with any OneWire devices
OneWire oneWire(ONE_WIRE_BUS);

// Pass OneWire reference to Dallas Temperature
DallasTemperature sensors(&oneWire);

// Address of the first DS18B20 sensor (Pool)
DeviceAddress poolSensorAddress;

// Address of the second DS18B20 sensor (Heat source)
DeviceAddress solarSensorAddress;

SimpleTimer timer;

// Threshold for temperature difference
const float tempTreshold = 2.0;

const float minTemp = 25.0;
const float maxTemp = 40.0;

unsigned long relayStartTime = 0;
const unsigned long relayDuration = 20000; // 20 seconds

bool masterSwitchOn = false;
bool motorDirectionSwitch = false;
bool cleanPosition = false;

WiFiManager wifiManager;
MQTTManager mqttManager;

void measureDiffTemp(float poolTemperature, float solarTemperature);
void manageRelay();
void startDeepSleep();
void mainLogic();
void sendTempMqtt(float poolTemperature, float solarTemperature);

#define SENSORPOOL "pool"
#define SENSORSOLAR "solar"

void setup(void)
{
    Serial.begin(9600);

    sensors.begin(); // Start up the library
    pinMode(relayMasterPin, OUTPUT);
    pinMode(relayMotorPin, OUTPUT);

    while (!sensors.getAddress(poolSensorAddress, 0) || !sensors.getAddress(solarSensorAddress, 1))
    {
        Serial.println("No temp sensor found");
    }

    // Set the resolution of the sensors to 12 bits (0.0625°C)
    sensors.setResolution(poolSensorAddress, 12);
    sensors.setResolution(solarSensorAddress, 12);

    timer.setInterval(10000, mainLogic);
}

void loop(void)
{
    timer.run();
    yield();
}

void mainLogic()
{
    // Request temperature readings from both sensors
    sensors.requestTemperaturesByAddress(poolSensorAddress);
    sensors.requestTemperaturesByAddress(solarSensorAddress);

    // Get the temperature readings
    float poolTemperature = sensors.getTempC(poolSensorAddress);
    float solarTemperature = sensors.getTempC(solarSensorAddress);

    poolTemperature = round(poolTemperature * 100) / 100;
    solarTemperature = round(solarTemperature * 100) / 100;

    sendTempMqtt(poolTemperature, solarTemperature);

    measureDiffTemp(poolTemperature, solarTemperature);
    manageRelay();
}

void sendTempMqtt(float poolTemperature, float solarTemperature)
{
    if (wifiManager.isConnected() && mqttManager.isConnected())
    {
        Serial.println("Send temperatures via mqtt");
        mqttManager.sendDiscovery(SENSORPOOL, poolTemperature);
        mqttManager.sendTemp(SENSORPOOL, poolTemperature);
        mqttManager.sendDiscovery(SENSORSOLAR, solarTemperature);
        mqttManager.sendTemp(SENSORSOLAR, solarTemperature);
    }

    mqttManager.disconnect();
    wifiManager.disconnect();
}

void measureDiffTemp(float poolTemperature, float solarTemperature)
{

    // Print the temperature readings
    Serial.printf("\nTemperatures pool: %.2f °C solar: %.2f °C\n\n", poolTemperature, solarTemperature);

    // Check the temperature difference and control the relay
    if (solarTemperature > minTemp && poolTemperature < maxTemp)
    {
        if (!motorDirectionSwitch && solarTemperature - poolTemperature > tempTreshold)
        {
            motorDirectionSwitch = true;
            masterSwitchOn = true;
        }
        else if (motorDirectionSwitch && solarTemperature - poolTemperature < tempTreshold)
        {
            motorDirectionSwitch = false;
            masterSwitchOn = true;
        }
    }
    else if (!cleanPosition)
    {
        cleanPosition = true;
        motorDirectionSwitch = false;
        masterSwitchOn = true;
    }
}

void manageRelay()
{
    if (masterSwitchOn && relayStartTime == 0)
    {
        Serial.printf("\nDirection: %d\n", motorDirectionSwitch);
        digitalWrite(relayMotorPin, motorDirectionSwitch);

        Serial.printf("\nMaster: %d\n", masterSwitchOn);
        digitalWrite(relayMasterPin, masterSwitchOn); // Turn on the relay

        relayStartTime = millis(); // Record the start time
    }

    // Check if the relay has been on for the specified duration
    if (masterSwitchOn && millis() - relayStartTime >= relayDuration)
    {
        masterSwitchOn = false;

        Serial.printf("\nMaster: %d\n", masterSwitchOn);
        digitalWrite(relayMasterPin, masterSwitchOn); // Turn off the relay

        relayStartTime = 0;
    }
}

void startDeepSleep()
{
    Serial.println("Going to deep sleep...");
    ESP.deepSleep(2 * 60 * 1e6);
    yield();
}
