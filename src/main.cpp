#include <Arduino.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include <SimpleTimer.h>
#include "MQTTManager.h"
#include "WiFiManager.h"

// PIN Definition
#define ONE_WIRE_BUS 13  // Temp sensors
#define relayMasterPin 4 // Master relay
#define relayMotorPin 5  // Direction Relay

// Definitions
#define SENSORPOOL "pool"
#define SENSORSOLAR "solar"
#define MOTORDIRECTION "mode"
#define AUTOMATIC "automatic"

// Temp Sensors
OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature sensors(&oneWire);
DeviceAddress poolSensorAddress;
DeviceAddress solarSensorAddress;

// Thresholds
const float tempTreshold = 2.0;
const float minTemp = 24.0;
const float maxTemp = 40.0;

// Timers
unsigned long relayStartTime = 0;
const unsigned long relayDuration = 20000; // 20 seconds
unsigned long previousMillis = 0;
const long interval = 10000;

bool masterSwitchOn = false;
bool motorDirectionSwitch = false;
bool initalRun = true;
bool automaticMode = false;

WiFiManager wifiManager;
MQTTManager mqttManager;

void measureDiffTemp(float poolTemperature, float solarTemperature);
void manageRelay();
void startDeepSleep();
void mainLogic();
void sendMQTT(float poolTemperature, float solarTemperature);

void setup(void)
{
    Serial.begin(9600);

    sensors.begin(); // Start up the library
    pinMode(relayMasterPin, OUTPUT);
    pinMode(relayMotorPin, OUTPUT);

    while (!sensors.getAddress(poolSensorAddress, 0) || !sensors.getAddress(solarSensorAddress, 1))
    {
        Serial.println("No temp sensor found");
        delay(5000);
    }

    // Set the resolution of the sensors to 12 bits (0.0625°C)
    sensors.setResolution(poolSensorAddress, 12);
    sensors.setResolution(solarSensorAddress, 12);

    // mainLogicTimer.setInterval(10000, mainLogic);
    mqttManager.setup();
}

void loop(void)
{
    

    if (initalRun)
    {
        Serial.printf("\nInitial: motorDirectionSwitch: %d\n", motorDirectionSwitch);
        motorDirectionSwitch = false;
        masterSwitchOn = true;
        initalRun = false;
        manageRelay();
    }

    unsigned long currentMillis = millis();
    if (currentMillis - previousMillis >= interval)
    {
        // Request temperature readings from both sensors
        sensors.requestTemperaturesByAddress(poolSensorAddress);
        sensors.requestTemperaturesByAddress(solarSensorAddress);

        // Get the temperature readings
        float poolTemperature = sensors.getTempC(poolSensorAddress);
        float solarTemperature = sensors.getTempC(solarSensorAddress);

        // Print the temperature readings
        Serial.printf("\nTemperatures pool: %.2f °C solar: %.2f °C\n\n", poolTemperature, solarTemperature);

        previousMillis = currentMillis;
        if (!masterSwitchOn)
        {
            measureDiffTemp(poolTemperature, solarTemperature);
        }
        manageRelay();
        sendMQTT(poolTemperature, solarTemperature);
    }
    
    if (wifiManager.isConnected())
    {
        if (mqttManager.connect())
        {
            mqttManager.loop(); // Handle MQTT communication
            automaticMode = mqttManager.getAutomaticState();
        }
    }
}

void sendMQTT(float poolTemperature, float solarTemperature)
{
    mqttManager.sendTempDiscovery(SENSORPOOL, poolTemperature);
    //mqttManager.sendTemp(SENSORPOOL, poolTemperature);
    mqttManager.sendTempDiscovery(SENSORSOLAR, solarTemperature);
    //mqttManager.sendTemp(SENSORSOLAR, solarTemperature);
    mqttManager.sendMotorDiscovery(MOTORDIRECTION);
    mqttManager.sendMotorDirection(MOTORDIRECTION, motorDirectionSwitch);
}

void measureDiffTemp(float poolTemperature, float solarTemperature)
{
    // Check the temperature difference and control the relay
    if (solarTemperature >= minTemp && poolTemperature <= maxTemp)
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
}

void manageRelay()
{
    if (masterSwitchOn && relayStartTime == 0)
    {
        digitalWrite(relayMotorPin, motorDirectionSwitch);
        digitalWrite(relayMasterPin, masterSwitchOn); // Turn on the relay

        relayStartTime = millis(); // Record the start time

        if (automaticMode)
        {
            mqttManager.switchOutlet("Pool", "1");
        }
    }

    // Check if the relay has been on for the specified duration
    if (masterSwitchOn && millis() - relayStartTime >= relayDuration)
    {
        masterSwitchOn = false;
        digitalWrite(relayMasterPin, masterSwitchOn); // Turn off the relay

        relayStartTime = 0;
        if (automaticMode)
        {
            mqttManager.switchOutlet("Pool", "0");
        }
    }
}
