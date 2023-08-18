#include <Arduino.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include <SimpleTimer.h>
#include "MQTTManager.h"
#include "WiFiManager.h"
#include "Ticker.h"

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

void mainLoopCallback();
void masterSwitchCallback();
void delayedPumpControlCallback();

bool masterSwitchOn = false;
bool motorDirectionSwitch = false;
bool initalRun = true;

// Deklaration der Funktionen
void compareTemperatures(float poolTemperature, float solarTemperature);
void switchRelay();
void sendMQTT(float poolTemperature, float solarTemperature);

// Timer
Ticker mainTimer(mainLoopCallback, 10000);
Ticker masterSwitchTimer(masterSwitchCallback, 25000, 1);
Ticker delayedPumpControlTimer(delayedPumpControlCallback, 5000, 1);

//const String baseSensorTopic = "homeassistant/sensor/debug/";
//const String baseSwitchTopic = "homeassistant/switch/debug/";
const String baseSensorTopic = "homeassistant/sensor/poolmcu/";
const String baseSwitchTopic = "homeassistant/switch/poolmcu/";

WiFiManager wifiManager;
MQTTManager mqttManager(baseSensorTopic, baseSwitchTopic);

void delayedPumpControlCallback()
{
    if (mqttManager.getAutomaticState())
    {
        mqttManager.switchOutlet("Pool", "0");
        Serial.println("Pump off");
    }
}

void masterSwitchCallback()
{
    masterSwitchOn = false;
    digitalWrite(relayMasterPin, masterSwitchOn); // Turn off the relay
    Serial.println("Pump off delay started");
    delayedPumpControlTimer.start();
}

void mainLoopCallback()
{
    // Request temperature readings from both sensors
    sensors.requestTemperaturesByAddress(poolSensorAddress);
    sensors.requestTemperaturesByAddress(solarSensorAddress);

    // Get the temperature readings
    float poolTemperature = sensors.getTempC(poolSensorAddress);
    float solarTemperature = sensors.getTempC(solarSensorAddress);

    // Print the temperature readings
    Serial.printf("\nTemperatures pool: %.2f °C solar: %.2f °C\n\n", poolTemperature, solarTemperature);

    if (!masterSwitchOn)
    {
        compareTemperatures(poolTemperature, solarTemperature);
    }
    switchRelay();
    sendMQTT(poolTemperature, solarTemperature);

    Serial.printf("mainTimerState: %d\n", mainTimer.state());
    Serial.printf("masterSwitchTimerState: %d\n", masterSwitchTimer.state());
    Serial.printf("delayedPumpControlTimerState: %d\n", delayedPumpControlTimer.state());
}

void sendMQTT(float poolTemperature, float solarTemperature)
{
    mqttManager.sendTempDiscovery(SENSORPOOL, poolTemperature);
    mqttManager.sendTemp(SENSORPOOL, poolTemperature);
    mqttManager.sendTempDiscovery(SENSORSOLAR, solarTemperature);
    mqttManager.sendTemp(SENSORSOLAR, solarTemperature);
    mqttManager.sendMotorDiscovery(MOTORDIRECTION);
    mqttManager.sendMotorDirection(MOTORDIRECTION, motorDirectionSwitch);
}

void compareTemperatures(float poolTemperature, float solarTemperature)
{
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

void switchRelay()
{
    if (masterSwitchOn && masterSwitchTimer.state() == STOPPED)
    {
        digitalWrite(relayMotorPin, motorDirectionSwitch);
        digitalWrite(relayMasterPin, masterSwitchOn); // Turn on the relay

        if (mqttManager.getAutomaticState())
        {
            mqttManager.switchOutlet("Pool", "1");
        }

        masterSwitchTimer.start();
    }
}

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
        switchRelay();
        mainTimer.start();
    }

    mainTimer.update();
    masterSwitchTimer.update();
    delayedPumpControlTimer.update();

    if (wifiManager.isConnected())
    {
        if (mqttManager.connect())
        {
            mqttManager.loop();
        }
    }
}